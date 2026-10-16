#include "sessions.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
using namespace pets;
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}
template<class Fn> void rejects(Fn fn) {
    try { fn(); } catch (const std::invalid_argument &) { return; }
    throw std::runtime_error("invalid clock must throw before changing state");
}
SessionUpdate item(std::string id, SessionStatus status = SessionStatus::Working,
                   std::uint64_t seq = 0, unsigned ttl = 120) {
    return {{"tool", std::move(id)}, "Task", seq, status, ttl};
}

// Catches expiry after arbitration and stale/clear messages renewing TTL.
void sequenceAndExpiry() {
    Sessions s;
    auto u = item("one", SessionStatus::Working, 1, 10);
    check(s.update(u, 0).accepted(), "initial sequence accepted");
    u.label = "Changed";
    check(s.update(u, 5).code == SessionResultCode::StaleSequence, "repeated sequence rejected");
    check(s.snapshot(5).active.at(0).label == "Task", "stale update cannot change label");
    check(s.nextDeadline() && *s.nextDeadline() == 10, "stale update cannot renew expiry");
    check(s.clear(u.key, 0, 6).code == SessionResultCode::StaleSequence, "older clear rejected");
    check(s.clear(u.key, 2, 7).accepted(), "clear accepts greater sequence");
    check(s.snapshot(7).active.empty() && s.recordCount() == 1, "clear retains hidden history");
    check(s.nextDeadline() && *s.nextDeadline() == 10, "clear retains original expiry");
    u.sequence = 2;
    check(s.update(u, 9).code == SessionResultCode::StaleSequence, "clear sequence blocks stale resurrection");
    check(s.snapshot(10).active.empty() && s.recordCount() == 0, "boundary expiry removes history");
    u.sequence = 0;
    check(s.update(u, 10).accepted(), "expired key becomes a fresh creation");
}

// Catches tombstones excluded from capacity and unknown clears allocating.
void capacity() {
    Sessions s;
    for (unsigned i = 0; i < 32; ++i) check(s.update(item(std::to_string(i)), 0).accepted(), "32 records allowed");
    check(s.clear({"tool", "absent"}, 1, 0).accepted() && s.recordCount() == 32,
          "unknown clear succeeds without allocation even at capacity");
    s.clear({"tool", "0"}, 1, 1);
    check(s.update(item("new"), 1).code == SessionResultCode::Capacity, "tombstone consumes capacity");
    check(s.update(item("1", SessionStatus::Completed, 1), 1).accepted(), "existing update allowed at capacity");
    check(s.update(item("new"), 120).accepted(), "expiry frees capacity before incoming operation");
    check(s.recordCount() == 2, "only renewed record and fresh record survive expiry");
}

// Catches accepting invalid IDs, scalar counts, malformed UTF-8 or numeric bounds.
void validation() {
    check(Sessions::validId("AZaz09._-") && Sessions::validId(std::string(64, 'a')), "ASCII ID alphabet accepted");
    for (const std::string &id : {std::string(), std::string(65, 'a'), std::string("a b"), std::string("a/b"), std::string("é")})
        check(!Sessions::validId(id), "invalid ID rejected");
    std::string unicode;
    for (unsigned i = 0; i < 120; ++i) unicode += "🐈";
    check(Sessions::validLabel(unicode) && Sessions::validLabel(""), "label accepts 120 Unicode scalars and empty plain text");
    check(!Sessions::validLabel(unicode + "a"), "label bound counts scalars");
    for (const std::string &label : {std::string("a\nb"), std::string("\x7f"), std::string("\xc2\x85"),
             std::string("\xc0\xaf"), std::string("\xed\xa0\x80"), std::string("\xf4\x90\x80\x80"),
             std::string("\x80"), std::string("\xe2\x82"), std::string("\xe2(x")})
        check(!Sessions::validLabel(label), "controls and malformed UTF-8 rejected");
    Sessions s;
    for (unsigned ttl : {0u, 3601u}) {
        check(s.update(item("a", SessionStatus::Working, 0, ttl), 0).code == SessionResultCode::InvalidParameters,
              "invalid TTL rejected");
    }
    check(s.update(item("a", SessionStatus::Working, Sessions::MaximumSequence + 1), 0).code
          == SessionResultCode::InvalidParameters, "sequence ceiling enforced");
    check(s.clear({"bad id", "a"}, 0, 0).code == SessionResultCode::InvalidParameters, "clear validates key");
    check(s.clear({"tool", "a"}, Sessions::MaximumSequence + 1, 0).code == SessionResultCode::InvalidParameters,
          "clear validates sequence");
    check(s.update(item("a", static_cast<SessionStatus>(999)), 0).code == SessionResultCode::InvalidParameters,
          "invalid typed status rejected");
    check(s.recordCount() == 0, "invalid updates allocate nothing");
    const auto result = s.update(item("a"), 0);
    check(result.accepted() && result.ttlSeconds == 120 && result.sequence == 0, "default TTL reported");
    check(s.snapshot(0).active.at(0).expiresAt == 120, "default TTL applied");
    Sessions maximum;
    check(maximum.update(item("a", SessionStatus::Working, Sessions::MaximumSequence, 3600), 0).accepted(),
          "maximum sequence and TTL accepted");
}

// Source metadata follows accepted updates without changing status arbitration.
void sourceMetadata() {
    Sessions s;
    auto u = item("source", SessionStatus::Completed, 1, 10);
    u.sourceUrl = "https://example.com/task";
    check(s.update(u, 0).accepted(), "bounded source metadata accepted");
    check(s.snapshot(0).active.at(0).sourceUrl == u.sourceUrl, "source metadata retained");
    s.acknowledge(u.key, 1);
    check(!s.snapshot(1).foreground, "acknowledged completion leaves foreground");
    u.sourceUrl = "file:///tmp/changed";
    check(s.update(u, 2).code == SessionResultCode::StaleSequence, "stale source change rejected");
    check(s.snapshot(2).active.at(0).sourceUrl == "https://example.com/task", "stale source retained");
    u.sequence = 2;
    check(s.update(u, 3).accepted(), "fresh source change accepted");
    check(s.snapshot(3).active.at(0).sourceUrl == u.sourceUrl && !s.snapshot(3).foreground,
          "metadata refresh preserves acknowledgment");
    u.sequence = 3; u.sourceUrl.clear();
    check(s.update(u, 4).accepted() && s.snapshot(4).active.at(0).sourceUrl.empty(),
          "omitted source clears previous metadata");
    u.sequence = 4;
    for (const auto &bad : {std::string(2049, 'a'), std::string("https://example.com/\n"),
                           std::string("\xc2\x85"), std::string("\xc0\xaf")}) {
        u.sourceUrl = bad;
        check(s.update(u, 5).code == SessionResultCode::InvalidParameters, "invalid core source metadata rejected");
        check(s.snapshot(5).active.at(0).sourceUrl.empty(), "invalid metadata leaves accepted state");
    }
    u.sourceUrl = std::string(2048, 'a');
    check(s.update(u, 5).accepted(), "core metadata byte ceiling accepted");
}

// Catches wrong status ordering, refresh timestamps and lexical tie direction.
void arbitration() {
    Sessions s;
    s.update(item("work"), 0);
    s.update(item("done", SessionStatus::Completed), 1);
    s.update(item("fail", SessionStatus::Failed), 2);
    s.update(item("z", SessionStatus::NeedsInput), 3);
    s.update(item("a", SessionStatus::NeedsInput), 3);
    check(s.snapshot(3).foreground.has_value(), "active sessions choose foreground");
    check(s.snapshot(3).foreground->key.sessionId == "a", "equal priority/time ties use lexical key");
    s.update(item("z", SessionStatus::NeedsInput, 1), 4);
    check(s.snapshot(4).foreground->key.sessionId == "a", "refresh cannot change transition ordering");
    s.update(item("new", SessionStatus::NeedsInput), 5);
    check(s.snapshot(5).foreground->key.sessionId == "new", "newer transition wins ties");
    s.acknowledge({"tool", "new"}, 5);
    s.acknowledge({"tool", "a"}, 5);
    s.acknowledge({"tool", "z"}, 5);
    check(s.snapshot(5).foreground->key.sessionId == "fail", "unacknowledged failed follows needs_input");
    s.acknowledge({"tool", "fail"}, 5);
    check(s.snapshot(5).foreground->key.sessionId == "done", "completed follows failed");
    s.clear({"tool", "done"}, 1, 5);
    check(s.snapshot(5).foreground->key.sessionId == "work", "working follows completed");
    s.clear({"tool", "work"}, 1, 5);
    check(!s.snapshot(5).foreground && s.snapshot(5).active.size() == 4, "acknowledged attention remains menu only");
}

// Catches acknowledgment changing expiry/status or failing to reset on transition.
void acknowledgment() {
    Sessions s;
    auto u = item("one", SessionStatus::NeedsInput);
    s.update(u, 0);
    check(s.acknowledge(u.key, 1), "known session acknowledges");
    auto snap = s.snapshot(1);
    check(!snap.foreground && !snap.reaction && snap.active.at(0).acknowledged
          && snap.active.at(0).status == SessionStatus::NeedsInput && snap.active.at(0).expiresAt == 120,
          "acknowledgment suppresses attention without changing status or expiry");
    u.sequence = 1;
    s.update(u, 2);
    check(s.snapshot(2).active.at(0).acknowledged && !s.snapshot(2).reaction, "refresh preserves acknowledgment");
    u.sequence = 2; u.status = SessionStatus::Failed;
    s.update(u, 5);
    check(!s.snapshot(5).active.at(0).acknowledged && s.snapshot(5).reaction, "transition clears acknowledgment");
    check(!s.acknowledge({"tool", "missing"}, 6), "unknown acknowledgment allocates nothing");
}

// Catches long/queued reactions, cooldown bypass and starting from a refresh.
void reactions() {
    Sessions s;
    s.update(item("work"), 0);
    check(s.snapshot(0).reaction && s.snapshot(0).reaction->endsAt == 3, "creation starts at most three seconds");
    s.update(item("work", SessionStatus::Working, 1), 1);
    check(s.snapshot(1).reaction->startedAt == 0, "refresh leaves existing gesture unchanged");
    s.update(item("need", SessionStatus::NeedsInput), 2);
    check(s.snapshot(2).foreground->key.sessionId == "need" && !s.snapshot(2).reaction,
          "higher priority arrival cancels old reaction but cannot bypass cooldown");
    s.advance(5);
    check(!s.snapshot(5).reaction, "cooldown completion never replays discarded reaction");
    s.update(item("need", SessionStatus::NeedsInput, 1), 5);
    check(!s.snapshot(5).reaction, "unchanged refresh cannot start after cooldown");
    s.update(item("need", SessionStatus::Failed, 2), 5);
    check(s.snapshot(5).reaction && s.snapshot(5).reaction->status == SessionStatus::Failed,
          "new foreground transition starts at exact cooldown boundary");
    s.advance(8);
    check(!s.snapshot(8).reaction, "reaction ends at exact three second boundary");
    s.update(item("background"), 10);
    check(!s.snapshot(10).reaction, "background creation does not react");
    s.clear({"tool", "need"}, 3, 10);
    check(s.snapshot(10).foreground && !s.snapshot(10).reaction, "clear foreground reveals state without gesture");
    Sessions expiry;
    expiry.update(item("low"), 0);
    expiry.update(item("high", SessionStatus::NeedsInput, 0, 1), 5);
    expiry.advance(6);
    check(expiry.snapshot(6).foreground->key.sessionId == "low" && !expiry.snapshot(6).reaction,
          "expiry foreground change cancels without a gesture");
}

// Catches suppression queuing or losing state, and wake retaining cooldown/history.
void suppressionAndSleep() {
    Sessions s;
    s.update(item("one"), 0);
    s.setSuppressed(true, 1);
    check(!s.snapshot(1).reaction, "suppression cancels active reaction");
    s.update(item("one", SessionStatus::NeedsInput, 1), 6);
    check(s.snapshot(6).foreground && !s.snapshot(6).reaction, "suppressed update retains display data only");
    s.setSuppressed(false, 7);
    check(!s.snapshot(7).reaction, "suppression release does not replay");
    s.update(item("one", SessionStatus::Failed, 2), 7);
    check(s.snapshot(7).reaction.has_value(), "fresh visible transition can react");
    s.reset(8);
    check(s.recordCount() == 0 && !s.nextDeadline() && !s.snapshot(8).foreground, "wake reset removes all state/deadlines");
    s.update(item("one", SessionStatus::Working, 0), 8);
    check(s.snapshot(8).reaction.has_value(), "wake reset removes sequence history and cooldown");
}

// Catches nonfinite/backward time mutating state or exposing infinite timers.
void clocksAndDeadlines() {
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    rejects([&] { Sessions bad(nan); });
    Sessions s(10);
    s.update(item("one"), 10);
    for (double bad : {9., nan, std::numeric_limits<double>::infinity()}) {
        rejects([&] { s.update(item("two"), bad); });
        rejects([&] { s.clear({"tool", "one"}, 1, bad); });
        rejects([&] { s.acknowledge({"tool", "one"}, bad); });
        rejects([&] { s.advance(bad); });
        rejects([&] { s.setSuppressed(true, bad); });
        rejects([&] { s.reset(bad); });
        rejects([&] { s.snapshot(bad); });
        check(s.recordCount() == 1 && s.snapshot(10).reaction, "clock rejection leaves state unchanged");
    }
    check(s.nextDeadline() && *s.nextDeadline() == 13, "reaction end precedes expiry deadline");
    s.advance(13);
    check(s.nextDeadline() && *s.nextDeadline() == 130, "expiry deadline survives reaction end");
    s.setSuppressed(true, 14);
    check(s.nextDeadline() && *s.nextDeadline() == 130, "suppression retains required expiry deadline");
    Sessions large(1e300);
    check(large.update(item("one"), 1e300).accepted(), "large finite clock accepted");
    check(large.nextDeadline() && std::isfinite(*large.nextDeadline()) && *large.nextDeadline() > 1e300,
          "large finite clock exposes future representable expiry");
    Sessions max(std::numeric_limits<double>::max());
    max.update(item("one"), std::numeric_limits<double>::max());
    check(!max.nextDeadline(), "largest finite clock has no spinning deadline");
}
} // namespace

int main() {
    unsigned failures = 0;
    const struct { const char *name; void (*run)(); } tests[] = {
        {"sequence and expiry", sequenceAndExpiry}, {"bounded capacity", capacity},
        {"parameter validation", validation}, {"source metadata", sourceMetadata}, {"foreground arbitration", arbitration},
        {"acknowledgment", acknowledgment}, {"reaction cooldown", reactions},
        {"suppression and wake reset", suppressionAndSleep}, {"clock/deadline safety", clocksAndDeadlines}
    };
    for (const auto &test : tests) {
        try { test.run(); std::cout << "PASS " << test.name << '\n'; }
        catch (const std::exception &error) { ++failures; std::cerr << "FAIL " << test.name << ": " << error.what() << '\n'; }
    }
    return failures ? 1 : 0;
}
