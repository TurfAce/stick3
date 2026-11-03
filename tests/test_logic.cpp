#include "../Insta360Remote/RemoteLogic.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <limits>

using remote::Action;

int main() {
    const uint8_t shutter[] = {0xfc,0xef,0xfe,0x86,0,3,1,2,0};
    const uint8_t mode[] = {0xfc,0xef,0xfe,0x86,0,3,1,1,0};
    const uint8_t screen[] = {0xfc,0xef,0xfe,0x86,0,3,1,0,0};
    assert(memcmp(remote::command(Action::Shutter).data(), shutter, 9) == 0);
    assert(memcmp(remote::command(Action::Mode).data(), mode, 9) == 0);
    assert(memcmp(remote::command(Action::Screen).data(), screen, 9) == 0);

    const auto& adv = remote::OrbitAdvertisement;
    size_t offset = 0;
    while (offset < sizeof(adv)) {
        assert(adv[offset] > 0);
        assert(offset + 1 + adv[offset] <= sizeof(adv));
        offset += 1 + adv[offset];
    }
    assert(offset == 31);
    assert(adv[4] == 0xff && adv[5] == 0x4c && adv[6] == 0);
    assert(4 + 2 + strlen(remote::DeviceName) <= 31);

    remote::Buttons buttons;
    assert(buttons.update(0, true, false) == Action::None);
    assert(buttons.update(100, false, false) == Action::None);
    assert(buttons.update(200, true, false) == Action::Shutter);
    assert(buttons.update(2200, true, false) == Action::None);
    assert(buttons.update(2300, false, false) == Action::None);
    assert(buttons.update(2400, false, true) == Action::None);
    assert(buttons.update(2500, false, false) == Action::Mode);
    assert(buttons.update(2600, false, true) == Action::None);
    assert(buttons.update(3599, false, true) == Action::None);
    assert(buttons.update(3600, false, true) == Action::Screen);
    assert(buttons.update(4600, false, true) == Action::None);
    assert(buttons.update(4700, false, false) == Action::None);
    assert(buttons.update(4800, true, true) == Action::None);
    assert(buttons.update(4900, true, false) == Action::None);
    assert(buttons.update(5000, false, false) == Action::None);
    assert(buttons.update(5100, true, false) == Action::Shutter);
    buttons.reset();
    assert(buttons.update(5200, true, false) == Action::None);
    assert(buttons.update(5300, false, false) == Action::None);
    assert(buttons.update(5400, true, false) == Action::Shutter);

    const uint32_t max = std::numeric_limits<uint32_t>::max();
    buttons.reset();
    buttons.update(max - 600, false, false);
    buttons.update(max - 500, false, true);
    assert(buttons.update(498, false, true) == Action::None);
    assert(buttons.update(499, false, true) == Action::Screen);
    assert(buttons.update(510, false, false) == Action::None);

    remote::SendGate gate;
    assert(!gate.allow(0, false, false));
    assert(!gate.allow(0, true, false));
    assert(gate.allow(0, true, true));
    assert(!gate.allow(399, true, true));
    assert(gate.allow(400, true, true));
    assert(!gate.allow(1000, false, true));
    assert(!gate.allow(1000, true, false));
    assert(gate.allow(1000, true, true));
    remote::SendGate rolloverGate;
    assert(rolloverGate.allow(max - 100, true, true));
    assert(!rolloverGate.allow(298, true, true));
    assert(rolloverGate.allow(299, true, true));

    std::cout << "PASS: command bytes, advertising, buttons, reconnect, cooldown, rollover\n";
}
