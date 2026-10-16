#include "runtime.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace {
void addCounter(std::uint64_t &counter, std::uint64_t amount) {
    const auto maximum = std::numeric_limits<std::uint64_t>::max();
    counter += std::min(amount, maximum - counter);
}

// A finite clock at the limit of double's range has no representable future.
// Omit its deadline instead of returning infinity or a timer that spins at now.
std::optional<double> future(double now, double delay) {
    double deadline = now + delay;
    if (deadline <= now) deadline = std::nextafter(now, std::numeric_limits<double>::infinity());
    return std::isfinite(deadline) ? std::optional<double>(deadline) : std::nullopt;
}
} // namespace

namespace pets {
bool Runtime::requestReaction(double now, double durationSeconds) {
    validateTime(now);
    if (!std::isfinite(durationSeconds) || durationSeconds <= 0 || durationSeconds > 3)
        throw std::invalid_argument("reaction duration must be finite and in (0, 3]");
    lastNow_ = now;
    if (suppressed() || (reactionActive_ && activeEnd_ && now < *activeEnd_)) return false;
    activeStart_ = now;
    double end = now + durationSeconds;
    if (end - now > durationSeconds) end = std::nextafter(end, now);
    if (!std::isfinite(end) || end <= now || tickDeadline(1) <= now) {
        restFrom(now);
        return false;
    }
    activeEnd_ = end;
    reactionActive_ = true;
    quietDeadline_.reset();
    nextStep_ = 1;
    return true;
}

void Runtime::cancelReaction(double now) {
    validateTime(now);
    lastNow_ = now;
    if (reactionActive_) restFrom(now);
}

void Runtime::setAmbientEnabled(bool enabled, double now) {
    validateTime(now);
    lastNow_ = now;
    if (ambientEnabled_ == enabled) return;
    ambientEnabled_ = enabled;
    if (!reactionActive_) restFrom(now);
}

Runtime::Runtime(double now, double activeHz) : lastNow_(now), activeHz_(activeHz) {
    if (!std::isfinite(now)) throw std::invalid_argument("runtime clock must be finite");
    if (!std::isfinite(activeHz) || activeHz <= 0 || activeHz > 60)
        throw std::invalid_argument("runtime rate must be finite and in (0, 60]");
    restFrom(now);
}

bool Runtime::suppressed() const {
    return !state_.visible || state_.paused || state_.dragging || state_.suspended || state_.quitting;
}

void Runtime::validateTime(double now) const {
    if (!std::isfinite(now) || now < lastNow_)
        throw std::invalid_argument("runtime clock must be finite and nondecreasing");
}

void Runtime::restFrom(double now) {
    activeEnd_.reset();
    reactionActive_ = false;
    nextStep_ = 1;
    quietDeadline_ = suppressed() || !ambientEnabled_ ? std::nullopt : future(now, 30);
}

void Runtime::control(Control command, double now) {
    validateTime(now);
    lastNow_ = now;
    if (state_.quitting) return;
    const bool wasSuppressed = suppressed();
    switch (command) {
    case Control::Show: state_.visible = true; break;
    case Control::Hide: state_.visible = false; break;
    case Control::Pause: state_.paused = true; break;
    case Control::Resume: state_.paused = false; break;
    case Control::BeginDrag: state_.dragging = true; break;
    case Control::EndDrag: state_.dragging = false; break;
    case Control::Suspend: state_.suspended = true; break;
    case Control::Wake: state_.suspended = false; break;
    case Control::Quit: state_.quitting = true; break;
    }
    if (suppressed() || wasSuppressed) restFrom(now);
}

double Runtime::tickDeadline(std::uint64_t index) const {
    // Derive every tick from the gesture origin, avoiding accumulated rounding drift.
    return static_cast<double>(static_cast<long double>(activeStart_)
                               + static_cast<long double>(index) / activeHz_);
}

std::uint64_t Runtime::dueTicks(double now) const {
    const auto elapsed = static_cast<long double>(now) - activeStart_;
    auto ticks = static_cast<std::uint64_t>(std::floor(elapsed * activeHz_));
    // The host sees double deadlines: honor that rounded boundary exactly, without
    // adding a tolerance that would permit ticks before the advertised deadline.
    if (tickDeadline(ticks + 1) <= now) ++ticks;
    return ticks;
}

Tick Runtime::advance(double now) {
    validateTime(now);
    lastNow_ = now;
    addCounter(diagnostics_.callbacks, 1);
    if (suppressed()) return {};

    if (!activeEnd_) {
        if (!quietDeadline_ || now < *quietDeadline_) return {};
        activeStart_ = *quietDeadline_;
        activeEnd_ = activeStart_ + 2;
        quietDeadline_.reset();
        nextStep_ = 1;
        if (!std::isfinite(*activeEnd_) || *activeEnd_ <= activeStart_
            || tickDeadline(1) <= activeStart_ || now >= *activeEnd_) {
            restFrom(now); // A missed ambient gesture never becomes a backlog.
            return {};
        }
    }

    if (now >= *activeEnd_) {
        // Count discarded ticks from the existing gesture only, not imaginary
        // gestures during a long clock jump. The end boundary itself is inactive.
        const auto total = dueTicks(std::nextafter(*activeEnd_, activeStart_));
        if (total >= nextStep_) addCounter(diagnostics_.droppedSteps, total - nextStep_ + 1);
        restFrom(now);
        return {};
    }

    const auto due = dueTicks(now);
    if (due < nextStep_) return {};
    const auto elapsedSteps = due - nextStep_ + 1;
    const auto steps = static_cast<unsigned>(std::min<std::uint64_t>(elapsedSteps, 4));
    addCounter(diagnostics_.steps, steps);
    addCounter(diagnostics_.droppedSteps, elapsedSteps - steps);
    nextStep_ = due + 1; // Drop excess time permanently instead of replaying it.
    return {steps};
}

std::optional<double> Runtime::nextDeadline() const {
    if (suppressed()) return {};
    if (!activeEnd_) return quietDeadline_;
    return std::min(tickDeadline(nextStep_), *activeEnd_);
}
} // namespace pets
