#include "../Insta360Remote/PowerPolicy.h"
#include "../Insta360Remote/RemoteLogic.h"
#include "../Insta360Remote/Config.h"
#include <cassert>
#include <iostream>
#include <limits>

using remote::Action;
using remote::DisplayLevel;

int main() {
    remote::DisplayIdle power(REMOTE_SCREEN_DIM_AFTER_MS, REMOTE_SCREEN_OFF_AFTER_MS);
    power.activity(1000);
    assert(power.level(6999) == DisplayLevel::Normal);
    assert(power.level(7000) == DisplayLevel::Dim);
    assert(power.level(10999) == DisplayLevel::Dim);
    assert(power.level(11000) == DisplayLevel::Off);
    // Camera RX never calls activity: the display stays off indefinitely.
    assert(power.level(120000) == DisplayLevel::Off);

    // First A press from backlight-off both wakes and triggers one shutter.
    remote::Buttons buttons;
    buttons.update(119990, false, false);
    power.activity(120000);
    assert(power.level(120000) == DisplayLevel::Normal);
    assert(buttons.update(120000, true, false) == Action::Shutter);
    assert(buttons.update(120010, true, false) == Action::None);
    power.activity(120010); // Holding a button keeps the display awake.
    assert(power.level(126009) == DisplayLevel::Normal);
    buttons.update(120020, false, false);

    // First B press wakes without prematurely changing capture mode.
    assert(power.level(180010) == DisplayLevel::Off);
    power.activity(180020);
    assert(buttons.update(180020, false, true) == Action::None);
    assert(power.level(180020) == DisplayLevel::Normal);
    assert(buttons.update(180120, false, false) == Action::Mode);
    // A long B press keeps its original one-shot screen command.
    power.activity(240200);
    buttons.update(240200, false, true);
    assert(buttons.update(241200, false, true) == Action::Screen);
    assert(buttons.update(241210, false, false) == Action::None);

    // Connection changes can wake the display without generating any button.
    assert(power.level(300200) == DisplayLevel::Off);
    power.activity(300210);
    assert(power.level(300210) == DisplayLevel::Normal);
    buttons.reset();
    assert(buttons.update(300210, false, false) == Action::None);

    // Timeouts can be disabled separately for always-on / dim-only use.
    remote::DisplayIdle alwaysOn(0, 0), dimOnly(REMOTE_SCREEN_DIM_AFTER_MS, 0),
        offOnly(0, REMOTE_SCREEN_OFF_AFTER_MS);
    assert(alwaysOn.level(1000000) == DisplayLevel::Normal);
    assert(dimOnly.level(1000000) == DisplayLevel::Dim);
    assert(offOnly.level(9999) == DisplayLevel::Normal);
    assert(offOnly.level(10000) == DisplayLevel::Off);

    const uint32_t max = std::numeric_limits<uint32_t>::max();
    power.activity(max - 2999);
    assert(power.level(2999) == DisplayLevel::Normal);
    assert(power.level(3000) == DisplayLevel::Dim);
    assert(power.level(6999) == DisplayLevel::Dim);
    assert(power.level(7000) == DisplayLevel::Off);
    power.activity(7001);
    assert(power.level(7001) == DisplayLevel::Normal);
    std::cout << "PASS: idle dim/off, first-press commands, hold, connection wake, disabled stages, rollover\n";
}
