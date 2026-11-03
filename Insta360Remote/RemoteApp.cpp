// StickS3 -> Insta360 X4 Air experimental BLE remote.
// Protocol references and third-party attribution: ../THIRD_PARTY_NOTICES.md
#include <Arduino.h>
#include <M5Unified.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <BLEAdvertising.h>
#include <atomic>
#include <algorithm>
#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <esp32-hal-cpu.h>
#include "Config.h"
#include "RemoteLogic.h"
#include "PowerPolicy.h"

#if !defined(CONFIG_IDF_TARGET_ESP32S3)
#error "This project targets M5Stack StickS3 (ESP32-S3), not StickC/Plus/Plus2."
#endif

static_assert(REMOTE_CPU_MHZ == 80 || REMOTE_CPU_MHZ == 160 || REMOTE_CPU_MHZ == 240,
              "Use a supported ESP32-S3 CPU frequency");
static_assert(REMOTE_BRIGHTNESS > 0 && REMOTE_BRIGHTNESS <= 255 &&
              REMOTE_DIM_BRIGHTNESS > 0 && REMOTE_DIM_BRIGHTNESS <= REMOTE_BRIGHTNESS,
              "Brightness must be 1..255; dim must not exceed normal brightness");
static_assert(REMOTE_SCREEN_DIM_AFTER_MS == 0 || REMOTE_SCREEN_OFF_AFTER_MS == 0 ||
              REMOTE_SCREEN_DIM_AFTER_MS < REMOTE_SCREEN_OFF_AFTER_MS,
              "Dimming must happen before backlight off");

namespace {
std::atomic<bool> connected{false}, subscribed{false}, restartAdvertising{false};
std::atomic<unsigned> rxCount{0}, droppedLogs{0};
std::atomic<int> txStatus{-1};
std::atomic<bool> loggingEnabled{false};
BLEServer* server = nullptr;
BLECharacteristic* tx = nullptr;
BLE2902* cccd = nullptr;
QueueHandle_t rxQueue = nullptr;
struct RxLog { uint16_t uuid; uint16_t total; uint8_t length; uint8_t data[64]; };
remote::Buttons buttons;
remote::SendGate sendGate;
unsigned sentCount = 0;
const char* notice = "Select remote on camera";
bool redraw = true;
M5Canvas screen(&M5.Display);
bool screenBuffered = false;
remote::DisplayIdle displayIdle(REMOTE_SCREEN_DIM_AFTER_MS, REMOTE_SCREEN_OFF_AFTER_MS);
uint8_t appliedBrightness = REMOTE_BRIGHTNESS;

class LinkCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer*) override {
        subscribed.store(false);
        connected.store(true);
        // Advertising stops at connection: one camera at a time.
    }
    void onDisconnect(BLEServer*) override {
        connected.store(false);
        subscribed.store(false);
        cccd->setNotifications(false);
        restartAdvertising.store(true);
    }
};

class SubscriptionCallbacks : public BLEDescriptorCallbacks {
    void onWrite(BLEDescriptor* descriptor) override {
        const bool enabled = descriptor->getLength() == 2 && (descriptor->getValue()[0] & 1);
        subscribed.store(enabled);
    }
};

class WriteCallbacks : public BLECharacteristicCallbacks {
public:
    explicit WriteCallbacks(uint16_t uuid) : uuid_(uuid) {}
    void onWrite(BLECharacteristic* characteristic) override {
        if (uuid_ == 0xce81) rxCount.fetch_add(1);
        // On battery without a USB monitor, do not copy/queue debug packets.
        if (!loggingEnabled.load()) return;
        const auto value = characteristic->getValue();
        RxLog entry{};
        entry.uuid = uuid_;
        entry.total = value.size();
        entry.length = std::min(value.size(), sizeof(entry.data));
        memcpy(entry.data, value.data(), entry.length);
        // BLE callbacks never draw or block on USB logging.
        if (xQueueSend(rxQueue, &entry, 0) != pdTRUE) droppedLogs.fetch_add(1);
    }
private:
    uint16_t uuid_;
};

class TransmitCallbacks : public BLECharacteristicCallbacks {
    void onStatus(BLECharacteristic*, Status status, uint32_t) override {
        txStatus.store(static_cast<int>(status));
    }
};

void makeRead(BLEService* service, uint16_t uuid, const uint8_t* data, size_t length) {
    auto* characteristic = service->createCharacteristic(BLEUUID(uuid), BLECharacteristic::PROPERTY_READ);
    characteristic->setValue(std::string(reinterpret_cast<const char*>(data), length));
}

void makeWrite(BLEService* service, uint16_t uuid) {
    auto* characteristic = service->createCharacteristic(BLEUUID(uuid), BLECharacteristic::PROPERTY_WRITE);
    characteristic->setCallbacks(new WriteCallbacks(uuid));
}

void beginBluetooth() {
    BLEDevice::init(remote::DeviceName);
    server = BLEDevice::createServer();
    server->setCallbacks(new LinkCallbacks());

    auto* primary = server->createService(BLEUUID(uint16_t(0xce80)), 12);
    makeWrite(primary, 0xce81);
    tx = primary->createCharacteristic(BLEUUID(uint16_t(0xce82)), BLECharacteristic::PROPERTY_NOTIFY);
    tx->setCallbacks(new TransmitCallbacks());
    cccd = new BLE2902();
    // The camera must explicitly enable notifications. Do not fake readiness.
    cccd->setNotifications(false);
    cccd->setCallbacks(new SubscriptionCallbacks());
    tx->addDescriptor(cccd);
    uint8_t empty = 0;
    tx->setValue(&empty, 1);
    const uint8_t version[] = {0x01, 0x02}; // uint16 0x0201, little endian
    makeRead(primary, 0xce83, version, sizeof(version));

    // Nine characteristics need more than the library's default 15 handles.
    auto* secondary = server->createService(BLEUUID(remote::SecondaryService), 40);
    const uint8_t model[] = {0x01, 0x90, 0x1e, 0x30};
    const uint8_t hardware[] = {0x01, 0x20, 0x00, 0x18};
    makeWrite(secondary, 0xffd1);
    makeRead(secondary, 0xffd2, &empty, 0);
    makeRead(secondary, 0xffd3, model, sizeof(model));
    makeRead(secondary, 0xffd4, hardware, sizeof(hardware));
    makeRead(secondary, 0xffd5, &empty, 0);
    makeWrite(secondary, 0xffd8);
    makeRead(secondary, 0xfff1, &empty, 0);
    makeWrite(secondary, 0xfff2);
    makeRead(secondary, 0xffe0, &empty, 0);
    primary->start();
    secondary->start();

    BLEAdvertisementData advertisement, scanResponse;
#if REMOTE_ORBIT_ADVERTISING
    advertisement.addData(std::string(reinterpret_cast<const char*>(remote::OrbitAdvertisement),
                                      sizeof(remote::OrbitAdvertisement)));
    scanResponse.setCompleteServices(BLEUUID(uint16_t(0xce80)));
#else
    advertisement.setFlags(0x06);
    advertisement.setCompleteServices(BLEUUID(uint16_t(0xce80)));
    advertisement.setCompleteServices(BLEUUID(remote::SecondaryService));
#endif
    scanResponse.setName(remote::DeviceName);
    // Refuse invalid packets instead of silently truncating the device name.
    if (advertisement.getPayload().size() > 31 || scanResponse.getPayload().size() > 31) {
        notice = "BLE packet too large";
        return;
    }
    auto* advertising = BLEDevice::getAdvertising();
    advertising->setScanResponse(true);
    advertising->setAdvertisementData(advertisement);
    advertising->setScanResponseData(scanResponse);
    advertising->setMinInterval(160); // 100 ms
    advertising->setMaxInterval(240); // 150 ms
    advertising->start();
}

void sendAction(remote::Action action) {
    if (action == remote::Action::None) return;
    redraw = true;
    if (!connected.load()) { notice = "Not connected"; return; }
    if (!subscribed.load()) { notice = "Wait for subscription"; return; }
    if (!sendGate.allow(millis(), connected.load(), subscribed.load())) {
        notice = "Wait 0.4s, then press";
        return;
    }
    auto packet = remote::command(action);
    txStatus.store(-1);
    tx->setValue(packet.data(), packet.size());
    tx->notify();
    const int status = txStatus.load();
    if (status == BLECharacteristicCallbacks::SUCCESS_NOTIFY) {
        ++sentCount;
        notice = action == remote::Action::Shutter ? "SHUTTER sent" :
                 action == remote::Action::Mode ? "MODE sent" : "SCREEN sent";
    } else {
        notice = "Send failed - check BLE";
    }
    // SUCCESS_NOTIFY means accepted by the local stack, not camera confirmation.
    if (Serial) {
        Serial.printf("TX status=%d bytes=", status);
        for (const auto byte : packet) Serial.printf("%02X ", byte);
        Serial.println();
    }
}

void drainLogs() {
    RxLog entry{};
    for (int n = 0; n < 4 && xQueueReceive(rxQueue, &entry, 0) == pdTRUE; ++n) {
        if (!Serial) continue;
        Serial.printf("RX %04X len=%u: ", entry.uuid, entry.total);
        for (uint8_t i = 0; i < entry.length; ++i) Serial.printf("%02X ", entry.data[i]);
        if (entry.length < entry.total) Serial.print("[truncated]");
        Serial.println();
    }
    const unsigned dropped = droppedLogs.exchange(0);
    if (dropped && Serial) Serial.printf("LOG dropped=%u\n", dropped);
}

void drawScreen() {
    const bool link = connected.load(), ready = link && subscribed.load();
    // Clear/redraw in RAM, then copy the completed frame to the LCD. A display
    // transaction alone does not hide intermediate fillScreen/print operations.
    lgfx::LGFXBase& display = screenBuffered
        ? static_cast<lgfx::LGFXBase&>(screen)
        : static_cast<lgfx::LGFXBase&>(M5.Display);
    display.startWrite();
    display.fillScreen(TFT_BLACK);
    display.setTextSize(1);
    display.setTextColor(TFT_WHITE, TFT_BLACK);
    display.setCursor(6, 4);
    display.print("INSTA360 X4 AIR / TEST");
    display.setTextSize(2);
    display.setTextColor(ready ? TFT_GREEN : TFT_YELLOW, TFT_BLACK);
    display.setCursor(6, 21);
    display.print(ready ? "READY" : link ? "CONNECTING" : "PAIR ON CAMERA");
    display.setTextSize(1);
    display.setTextColor(TFT_WHITE, TFT_BLACK);
    display.setCursor(6, 47);
    display.println("A: shutter / rec toggle");
    display.setCursor(6, 61);
    display.println("B: mode   Hold B: screen");
    display.setCursor(6, 80);
    display.print(notice);
    display.setCursor(6, 96);
    display.printf("TX %u  RX %u", sentCount, rxCount.load());
    display.setCursor(6, 115);
    display.print("Check recording on camera");
    display.endWrite();
    if (screenBuffered) screen.pushSprite(0, 0);
}
} // namespace

void setup() {
    // Set the CPU clock once, before display/BLE peripheral initialization.
    const bool cpuConfigured = setCpuFrequencyMhz(REMOTE_CPU_MHZ);
    auto config = M5.config();
    config.serial_baudrate = 115200;
    config.fallback_board = m5::board_t::board_M5StickS3;
    config.internal_imu = false;
    config.internal_rtc = false;
    config.internal_mic = false;
    config.internal_spk = false;
    config.output_power = false;
    M5.begin(config); // Includes StickS3 display, buttons and power management.
    M5.Display.setRotation(REMOTE_DISPLAY_ROTATION);
    M5.Display.setBrightness(REMOTE_BRIGHTNESS);
    appliedBrightness = REMOTE_BRIGHTNESS;
    displayIdle.activity(millis());
    M5.Display.setFont(&fonts::Font0);
    screen.setColorDepth(16);
    screenBuffered = screen.createSprite(M5.Display.width(), M5.Display.height()) != nullptr;
    if (!screenBuffered) {
        // Keep the remote usable if external RAM is unavailable.
        screen.setPsram(false);
        screenBuffered = screen.createSprite(M5.Display.width(), M5.Display.height()) != nullptr;
    }
    screen.setFont(&fonts::Font0);
    drawScreen();
    rxQueue = xQueueCreate(8, sizeof(RxLog));
    if (!rxQueue) {
        notice = "Memory allocation failed";
        drawScreen();
        while (true) delay(100);
    }
    beginBluetooth();
    if (Serial) {
        Serial.printf("POWER CPU=%u MHz (requested=%u, ok=%u) dim=%lu ms off=%lu ms\n",
                      getCpuFrequencyMhz(), REMOTE_CPU_MHZ, unsigned(cpuConfigured),
                      REMOTE_SCREEN_DIM_AFTER_MS, REMOTE_SCREEN_OFF_AFTER_MS);
    }
    drawScreen();
}

void loop() {
    M5.update();
    const uint32_t now = millis();
    const bool buttonA = M5.BtnA.isPressed(), buttonB = M5.BtnB.isPressed();
    if (buttonA || buttonB) displayIdle.activity(now);
    loggingEnabled.store(static_cast<bool>(Serial));
    static bool oldLink = false, oldReady = false;
    static unsigned oldRx = 0;
    static bool pendingRestart = false;
    static uint32_t restartSince = 0, lastDraw = 0, lastCounterDraw = 0;

    if (restartAdvertising.exchange(false)) {
        pendingRestart = true;
        restartSince = now;
        buttons.reset();
        notice = "Disconnected - reconnect";
        redraw = true;
    }
    if (pendingRestart && uint32_t(now - restartSince) >= 300) {
        pendingRestart = false;
        if (!connected.load()) BLEDevice::startAdvertising();
    }
    const bool link = connected.load(), ready = link && subscribed.load();
    if (link != oldLink || ready != oldReady) {
        displayIdle.activity(now);
        buttons.reset(); // Do not turn a button held during pairing into a shot.
        notice = ready ? "Connected; check camera" : link ? "Waiting for camera" : "Select remote on camera";
        oldLink = link;
        oldReady = ready;
        redraw = true;
        if (Serial) Serial.printf("LINK %s SUBSCRIBE %s\n", link ? "UP" : "DOWN", ready ? "ON" : "OFF");
    }
    // Wake the screen and process the SAME press as a normal camera command.
    // Never consume the first press just to turn on the display.
    sendAction(buttons.update(now, buttonA, buttonB));
    drainLogs();
    const auto displayLevel = displayIdle.level(now);
    const uint8_t targetBrightness = displayLevel == remote::DisplayLevel::Off ? 0 :
        displayLevel == remote::DisplayLevel::Dim ? REMOTE_DIM_BRIGHTNESS : REMOTE_BRIGHTNESS;
    const bool displayVisible = targetBrightness != 0;
    const bool waking = displayVisible && appliedBrightness == 0;
    if (waking) redraw = true;
    const unsigned currentRx = rxCount.load();
    // Incoming camera status can be frequent. Counter-only redraws are limited
    // to 1 Hz; button and connection changes still request an immediate redraw.
    // If both buffer allocations failed, do not flicker the LCD for RX alone.
    if (displayVisible && screenBuffered && currentRx != oldRx && uint32_t(now - lastCounterDraw) >= 1000) {
        redraw = true;
    }
    if (displayVisible && redraw && (waking || uint32_t(now - lastDraw) >= 100)) {
        drawScreen();
        redraw = false;
        lastDraw = now;
        lastCounterDraw = now;
        oldRx = currentRx;
    }
    // On wake, copy the latest frame before enabling the backlight.
    if (targetBrightness != appliedBrightness) {
        M5.Display.setBrightness(targetBrightness);
        appliedBrightness = targetBrightness;
    }
    delay(5);
}
