#include "runtime.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void deadline(const pets::Runtime &r, double expected, const char *message) {
    check(r.nextDeadline() && std::abs(*r.nextDeadline() - expected) < 1e-8, message);
}

template<class Fn> void rejects(Fn fn, const char *message) {
    try { fn(); }
    catch (const std::invalid_argument &) { return; }
    throw std::runtime_error(message);
}

// Catches control handlers clearing the independent visibility/pause preference.
void independentPreferences() {
    pets::Runtime r;
    r.control(pets::Control::Pause, 1);
    r.control(pets::Control::Hide, 2);
    r.control(pets::Control::Show, 3);
    check(r.state().visible && r.state().paused, "show preserves pause");
    check(!r.nextDeadline(), "paused runtime has no animation timer");
    check(r.advance(1000).steps == 0, "paused time produces no steps");
    r.control(pets::Control::Resume, 1000);
    check(r.advance(1000).steps == 0, "resume never catches up");
    deadline(r, 1030, "resume starts fresh quiet time");
    r.control(pets::Control::Hide, 1001);
    r.control(pets::Control::Pause, 1002);
    r.control(pets::Control::Resume, 1003);
    check(!r.state().visible && !r.state().paused, "resume preserves hidden state");
    check(!r.nextDeadline(), "hidden resumed pet has no deadline");
    r.control(pets::Control::Show, 1004);
    deadline(r, 1034, "show schedules fresh quiet time");
}

// Catches continuous idle ticking, overlong gestures, or too-short cooldowns.
void quietWindows() {
    pets::Runtime r;
    deadline(r, 30, "initial quiet deadline is thirty seconds");
    check(r.advance(29.99).steps == 0, "quiet time does not advance motion");
    check(r.advance(30).steps == 0, "gesture starts without elapsed tick");
    deadline(r, 30 + 1.0 / 60, "gesture schedules first tick at configured rate");
    check(r.advance(30 + 1.0 / 60).steps == 1, "first active tick advances once");
    check(r.advance(32).steps == 0, "gesture cannot animate after two seconds");
    deadline(r, 62, "next gesture is thirty seconds after prior window ends");
    check(r.advance(61.99).steps == 0, "quiet spacing forbids early animation");
    r.advance(62);
    check(r.advance(62 + 1.0 / 60).steps == 1, "next quiet gesture animates");
}

// Catches backlog replay, unlimited catch-up, and lost dropped-step diagnostics.
void lateCallbacks() {
    pets::Runtime r;
    r.advance(30);
    check(r.advance(31).steps == 4, "catch-up is bounded to four steps");
    check(r.diagnostics().steps == 4, "executed steps counted");
    check(r.diagnostics().droppedSteps == 56, "excess elapsed ticks counted as dropped");
    check(r.advance(31).steps == 0, "equal-time callbacks do not replay dropped ticks");
    check(r.advance(1000000).steps == 0, "expired active window is never replayed");
    deadline(r, 1000030, "late window termination schedules fresh quiet time");
    pets::Runtime quiet;
    check(quiet.advance(1000000).steps == 0, "missed quiet gesture is skipped");
    deadline(quiet, 1000030, "missed gesture schedules fresh quiet time");
    pets::Runtime slightlyLate;
    check(slightlyLate.advance(31).steps == 4, "late start inside gesture window is bounded");
    check(slightlyLate.advance(32).steps == 0, "late start does not extend the gesture");
}

// Catches drag/sleep retaining active motion or clearing visibility/pause.
void suppressionAndQuit() {
    pets::Runtime r;
    r.advance(30);
    r.control(pets::Control::BeginDrag, 30.5);
    check(r.state().dragging && !r.nextDeadline(), "dragging stops scheduling");
    check(r.advance(100).steps == 0, "dragging drops elapsed motion");
    r.control(pets::Control::EndDrag, 100);
    deadline(r, 130, "drag release starts fresh quiet time");
    r.control(pets::Control::Hide, 101);
    r.control(pets::Control::Pause, 102);
    r.control(pets::Control::Suspend, 103);
    check(r.state().suspended && !r.nextDeadline(), "suspension stops scheduling");
    check(r.advance(10000).steps == 0, "suspended time never animates");
    r.control(pets::Control::Wake, 10000);
    check(!r.state().suspended && !r.state().visible && r.state().paused,
          "wake retains visibility and pause");
    check(!r.nextDeadline(), "wake keeps suppressed pet timer-free");
    r.control(pets::Control::Show, 10000);
    r.control(pets::Control::Resume, 10000);
    deadline(r, 10030, "final suppression release starts fresh quiet time");
    r.advance(10030);
    r.control(pets::Control::Suspend, 10030.1);
    r.control(pets::Control::Wake, 20000);
    check(r.advance(20000).steps == 0, "wake discards active backlog");
    deadline(r, 20030, "wake schedules fresh quiet time");
    r.control(pets::Control::Quit, 20000);
    r.control(pets::Control::Resume, 20001);
    r.control(pets::Control::Show, 20001);
    check(r.state().quitting && !r.nextDeadline(), "quit is terminal and cancels scheduling");
    check(r.advance(30000).steps == 0, "quit never animates");
}

// Catches duplicate commands resetting deadlines or cancelling live gestures.
void duplicateControls() {
    pets::Runtime r;
    r.control(pets::Control::Show, 1);
    r.control(pets::Control::Resume, 2);
    r.control(pets::Control::EndDrag, 3);
    r.control(pets::Control::Wake, 4);
    deadline(r, 30, "inactive commands preserve quiet deadline");
    r.advance(30);
    const auto active = r.nextDeadline();
    r.control(pets::Control::Show, 30);
    r.control(pets::Control::Resume, 30);
    check(r.nextDeadline() == active, "duplicate controls preserve active deadline");
    r.control(pets::Control::Pause, 31);
    r.control(pets::Control::Pause, 32);
    check(!r.nextDeadline(), "duplicate suppression has no timer");
    r.control(pets::Control::Resume, 33);
    r.control(pets::Control::Resume, 34);
    deadline(r, 63, "duplicate resume does not move quiet deadline");
}

// Catches rate acceptance beyond the ceiling and clock errors mutating state.
void validation() {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    for (double bad : {nan, inf, -inf}) {
        rejects([&] { pets::Runtime r(bad); }, "nonfinite initial clock rejected");
    }
    for (double bad : {0., -1., 60.1, nan, inf}) {
        rejects([&] { pets::Runtime r(0, bad); }, "invalid rate rejected");
    }
    pets::Runtime r(10);
    r.advance(11);
    for (double bad : {10., nan, inf, -inf}) {
        const auto prior = r.nextDeadline();
        const auto callbacks = r.diagnostics().callbacks;
        rejects([&] { r.control(pets::Control::Hide, bad); }, "bad control clock rejected");
        check(r.state().visible && r.nextDeadline() == prior,
              "rejected control leaves state and deadline untouched");
        rejects([&] { r.advance(bad); }, "bad advance clock rejected");
        check(r.diagnostics().callbacks == callbacks && r.nextDeadline() == prior,
              "rejected advance leaves diagnostics and deadline untouched");
    }
    r.control(pets::Control::Pause, 11);
    r.control(pets::Control::Resume, 11);
    deadline(r, 41, "equal-time controls accepted without elapsed animation");
}

// Catches hard-coded 60Hz scheduling and per-callback step inflation.
void configurableRate() {
    pets::Runtime r(0, 20);
    r.advance(30);
    deadline(r, 30.05, "20Hz uses fifty-millisecond interval");
    check(r.advance(30.049).steps == 0, "20Hz forbids early step");
    check(r.advance(30.05).steps == 1, "20Hz steps at deadline");
    check(r.advance(30.25).steps == 4, "20Hz catches up four due steps");
    check(r.diagnostics().droppedSteps == 0, "four due ticks do not count drops");
    pets::Runtime slow(0, 0.1);
    slow.advance(30);
    deadline(slow, 32, "gesture end wakes before a slow tick");
    check(slow.advance(32).steps == 0, "low rate never extends gesture");
}

// Catches timers spinning when finite clock magnitudes cannot represent tick intervals.
void finiteClockLimits() {
    pets::Runtime r(1e300);
    const auto start = r.nextDeadline();
    check(!start || *start > 1e300, "large finite clocks never expose a past deadline");
    if (start) {
        check(r.advance(*start).steps == 0, "unrepresentable animation interval is skipped");
        check(!r.nextDeadline() || *r.nextDeadline() > *start,
              "large finite clocks do not spin on the same animation deadline");
    }
    pets::Runtime maximum(std::numeric_limits<double>::max());
    check(!maximum.nextDeadline(), "no timer exists beyond largest finite clock");
    check(maximum.advance(std::numeric_limits<double>::max()).steps == 0,
          "largest finite clock is accepted without animation");
}

// Catches loss of cumulative counters and accidental retained callback history.
void boundedDiagnostics() {
    static_assert(sizeof(pets::Diagnostics) == 3 * sizeof(std::uint64_t),
                  "diagnostics must contain only fixed counters");
    pets::Runtime r;
    for (unsigned i = 0; i < 100000; ++i) check(r.advance(0).steps == 0, "quiet callback produces zero steps");
    check(r.diagnostics().callbacks == 100000, "callbacks counted without history");
    check(r.diagnostics().steps == 0 && r.diagnostics().droppedSteps == 0,
          "quiet callbacks do not count animation");
    r.advance(30);
    r.advance(30.5);
    check(r.diagnostics().callbacks == 100002 && r.diagnostics().steps == 4
          && r.diagnostics().droppedSteps == 26, "fixed diagnostics accumulate bounded catch-up");
}

// Catches notification gestures bypassing the shared step cap or extending past
// their short window, and ambient timers stealing their priority.
void notificationScheduling() {
    pets::Runtime r;
    check(r.requestReaction(1), "visible notification starts immediately");
    check(r.reactionActive(), "reaction kind exposed to the shell");
    deadline(r, 1 + 1.0 / 60, "notification uses the shared active tick rate");
    check(r.advance(2).steps == 4 && r.diagnostics().droppedSteps == 56,
          "notification catch-up shares four-step ceiling");
    check(!r.requestReaction(2), "duplicate reaction request does not restart live window");
    check(r.advance(4).steps == 0 && !r.reactionActive(), "notification ends after three seconds");
    deadline(r, 34, "notification end starts fresh ambient rest");
    r.advance(34);
    check(r.requestReaction(34.5, 1), "notification preempts ambient gesture");
    deadline(r, 34.5 + 1.0 / 60, "preempted gesture starts with fresh elapsed time");
    check(r.advance(35.5).steps == 0 && !r.reactionActive(), "explicit shorter notification window honored");
    deadline(r, 65.5, "short reaction is followed by ambient quiet time");
    check(r.requestReaction(100), "late request starts a fresh reaction without ambient backlog");
    check(r.advance(100).steps == 0, "request never catches up old animation");
}

// Catches suppression retaining notification state or cancel restarting ambient
// work when there is no notification to cancel.
void notificationCancellation() {
    pets::Runtime r;
    r.cancelReaction(1);
    deadline(r, 30, "cancel with no reaction preserves ambient deadline");
    r.requestReaction(2);
    r.cancelReaction(2.5);
    check(!r.reactionActive() && r.advance(2.5).steps == 0, "cancel discards active notification");
    deadline(r, 32.5, "cancel starts fresh quiet time");
    r.control(pets::Control::Pause, 3);
    check(!r.requestReaction(4) && !r.nextDeadline(), "paused notification refused without timer");
    r.control(pets::Control::Resume, 5);
    deadline(r, 35, "resume never replays suppressed request");
    r.requestReaction(6);
    r.control(pets::Control::BeginDrag, 6.1);
    check(!r.reactionActive() && !r.nextDeadline(), "drag cancels notification immediately");
    r.control(pets::Control::EndDrag, 7);
    deadline(r, 37, "drag release never resumes canceled notification");
    r.requestReaction(8);
    r.control(pets::Control::Suspend, 8.1);
    r.control(pets::Control::Wake, 1000);
    check(!r.reactionActive(), "wake loses pre-sleep reaction");
    deadline(r, 1030, "wake discards elapsed notification time");
    r.control(pets::Control::Hide, 1001);
    check(!r.requestReaction(1002), "hidden notification refused");
    r.control(pets::Control::Show, 1003);
    r.control(pets::Control::Quit, 1004);
    check(!r.requestReaction(1005), "quitting notification refused");
}

// Catches bad notification inputs mutating scheduling, and timer spin at
// unrepresentable finite clocks.
void notificationValidation() {
    pets::Runtime r(10);
    for (double bad : {0., -1., 3.1, std::numeric_limits<double>::quiet_NaN(),
                       std::numeric_limits<double>::infinity()}) {
        rejects([&] { r.requestReaction(10, bad); }, "invalid reaction duration rejected");
        deadline(r, 40, "invalid duration leaves scheduling unchanged");
    }
    r.requestReaction(11);
    const auto prior = r.nextDeadline();
    for (double bad : {10., std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()}) {
        rejects([&] { r.requestReaction(bad); }, "invalid request clock rejected");
        rejects([&] { r.cancelReaction(bad); }, "invalid cancel clock rejected");
        check(r.reactionActive() && r.nextDeadline() == prior, "bad reaction clock changes nothing");
    }
    pets::Runtime large(1e300);
    check(!large.requestReaction(1e300) && !large.reactionActive(), "unrepresentable reaction window skipped");
    check(!large.nextDeadline() || *large.nextDeadline() > 1e300, "large reaction input never spins timer");
}
void replaceAmbientScheduler() {
    pets::Runtime r;
    r.setAmbientEnabled(false, 0);
    check(!r.nextDeadline(), "external chooser removes legacy idle deadline");
    check(r.advance(30).steps == 0, "legacy ambient episode stays disabled");
    check(r.requestReaction(31), "notifications retain active scheduling");
    r.advance(34);
    check(!r.nextDeadline(), "notification completion stays quiet");
    r.setAmbientEnabled(true, 35);
    deadline(r, 65, "fixture can restore legacy idle scheduling");
}
} // namespace

int main() {
    unsigned failures = 0;
    const struct { const char *name; void (*run)(); } tests[] = {
        {"external ambient scheduler", replaceAmbientScheduler}, {"independent preferences", independentPreferences}, {"quiet windows", quietWindows},
        {"late callbacks", lateCallbacks}, {"suppression and quit", suppressionAndQuit},
        {"duplicate controls", duplicateControls}, {"clock/rate validation", validation},
        {"configurable rate", configurableRate}, {"bounded diagnostics", boundedDiagnostics},
        {"finite clock limits", finiteClockLimits}, {"notification scheduling", notificationScheduling},
        {"notification cancellation", notificationCancellation}, {"notification validation", notificationValidation}
    };
    for (const auto &test : tests) {
        try { test.run(); std::cout << "PASS " << test.name << '\n'; }
        catch (const std::exception &error) {
            ++failures;
            std::cerr << "FAIL " << test.name << ": " << error.what() << '\n';
        }
    }
    return failures ? 1 : 0;
}
