#pragma once
#include <cstdint>

namespace remote {
enum class DisplayLevel : uint8_t { Normal, Dim, Off };

// The caller reports local button activity or connection changes. BLE status
// traffic must not refresh this timer, otherwise a paired camera prevents dimming.
class DisplayIdle {
public:
    DisplayIdle(uint32_t dimAfterMs, uint32_t offAfterMs)
        : dimAfter_(dimAfterMs), offAfter_(offAfterMs) {}

    void activity(uint32_t now) { lastActivity_ = now; }

    DisplayLevel level(uint32_t now) const {
        const uint32_t elapsed = now - lastActivity_;
        if (offAfter_ && elapsed >= offAfter_) return DisplayLevel::Off;
        if (dimAfter_ && elapsed >= dimAfter_) return DisplayLevel::Dim;
        return DisplayLevel::Normal;
    }
private:
    uint32_t dimAfter_, offAfter_, lastActivity_ = 0;
};
} // namespace remote
