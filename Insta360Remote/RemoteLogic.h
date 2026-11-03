#pragma once
#include <array>
#include <cstdint>

namespace remote {
enum class Action : uint8_t { None, Shutter, Mode, Screen };

// Observed GPS-remote button frames; compatibility with X4 Air needs testing.
inline std::array<uint8_t, 9> command(Action action) {
    const uint8_t key = action == Action::Shutter ? 2 : action == Action::Mode ? 1 : 0;
    return {{0xfc, 0xef, 0xfe, 0x86, 0x00, 0x03, 0x01, key, 0x00}};
}

static constexpr char DeviceName[] = "Insta360 GPS Remote";
static constexpr char SecondaryService[] = "0000d0ff-3c17-d293-8e48-14fe2e4da212";

// Legacy advertisements have a 31-byte limit. No camera serial is included:
// this is an identity payload, not a wakeup request.
static constexpr uint8_t OrbitAdvertisement[] = {
    0x02, 0x01, 0x06,
    0x1b, 0xff, 0x4c, 0x00, 0x02, 0x15, 0x09, 0x4f, 0x52, 0x42, 0x49,
    0x54, 0x09, 0xff, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xe4, 0x01
};
static_assert(sizeof(OrbitAdvertisement) == 31, "Advertisement must fit legacy BLE");

// Feed debounced button states from M5Unified. A fires once on press.
// B fires Mode on release, or Screen once after a one-second hold.
// Startup/reconnect-held buttons and simultaneous presses are suppressed.
class Buttons {
public:
    void reset() { inhibited_ = true; a_ = b_ = longSent_ = false; }

    Action update(uint32_t now, bool a, bool b) {
        if (inhibited_ || (a && b)) {
            inhibited_ = a || b;
            a_ = a;
            b_ = b;
            longSent_ = false;
            return Action::None;
        }
        Action result = Action::None;
        if (a && !a_) result = Action::Shutter;
        if (b && !b_) { bSince_ = now; longSent_ = false; }
        if (b && !longSent_ && uint32_t(now - bSince_) >= 1000) {
            longSent_ = true;
            result = Action::Screen;
        }
        if (!b && b_ && !longSent_) result = Action::Mode;
        a_ = a;
        b_ = b;
        return result;
    }
private:
    bool inhibited_ = true, a_ = false, b_ = false, longSent_ = false;
    uint32_t bSince_ = 0;
};

// Commands are dropped while disconnected/unsubscribed and are never replayed.
// Failed sends also consume the cooldown; retrying a toggle can undo a shot.
class SendGate {
public:
    bool allow(uint32_t now, bool connected, bool subscribed) {
        if (!connected || !subscribed) return false;
        if (used_ && uint32_t(now - last_) < 400) return false;
        used_ = true;
        last_ = now;
        return true;
    }
private:
    bool used_ = false;
    uint32_t last_ = 0;
};
} // namespace remote
