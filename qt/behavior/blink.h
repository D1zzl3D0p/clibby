#pragma once
#include <cstdint>
#include <optional>
#include <random>

namespace pets::behavior {
// Independent eyelid channel: only a blink's short active interval needs frames.
class Blink {
public:
    explicit Blink(std::uint32_t seed=1);
    void reset(double now);
    bool advance(double now, bool suppressed=false);
    double openness() const { return openness_; }
    std::optional<double> nextDeadline() const { return deadline_; }
private:
    void wait(double now);
    void frame(double now);
    std::uint32_t seed_;
    std::mt19937 rng_;
    std::optional<double> deadline_;
    double lastNow_=0, elapsed_=0, openness_=1;
    bool active_=false, suppressed_=false;
};
}
