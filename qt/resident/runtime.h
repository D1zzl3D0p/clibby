#pragma once

#include <cstdint>
#include <optional>

namespace pets {

enum class Control { Show, Hide, Pause, Resume, BeginDrag, EndDrag, Suspend, Wake, Quit };

struct RuntimeState {
    bool visible = true;
    bool paused = false;
    bool dragging = false;
    bool suspended = false;
    bool quitting = false;
};

struct Tick { unsigned steps = 0; };

struct Diagnostics {
    std::uint64_t callbacks = 0;
    std::uint64_t steps = 0;
    std::uint64_t droppedSteps = 0;
};

class Runtime {
public:
    // Clock values are finite, monotonic seconds; equal timestamps are allowed.
    // Quiet gestures occupy [scheduled start, start + 2s), then rest >=30s.
    explicit Runtime(double now = 0, double activeHz = 60);
    void control(Control command, double now);
    // Notification gestures preempt ambient work and share its fixed-step budget.
    // Return false when suppressed, already reacting, or no future tick can be
    // represented. Durations must be finite and in (0, 3] seconds.
    bool requestReaction(double now, double durationSeconds = 3);
    void cancelReaction(double now);
    void setAmbientEnabled(bool enabled, double now);
    bool reactionActive() const { return reactionActive_; }
    Tick advance(double now);
    std::optional<double> nextDeadline() const;
    const RuntimeState &state() const { return state_; }
    const Diagnostics &diagnostics() const { return diagnostics_; }

private:
    bool suppressed() const;
    void validateTime(double now) const;
    void restFrom(double now);
    double tickDeadline(std::uint64_t index) const;
    std::uint64_t dueTicks(double now) const;

    RuntimeState state_;
    Diagnostics diagnostics_;
    double lastNow_;
    double activeHz_;
    std::optional<double> quietDeadline_;
    std::optional<double> activeEnd_;
    double activeStart_ = 0;
    std::uint64_t nextStep_ = 1;
    bool reactionActive_ = false;
    bool ambientEnabled_ = true;
};

} // namespace pets
