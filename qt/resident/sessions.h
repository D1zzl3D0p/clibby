#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pets {

enum class SessionStatus { Working, NeedsInput, Completed, Failed };
const char *sessionStatusName(SessionStatus status);

struct SessionKey {
    std::string sourceId;
    std::string sessionId;
    bool operator==(const SessionKey &other) const;
    bool operator<(const SessionKey &other) const;
};

struct SessionUpdate {
    SessionKey key;
    std::string label;
    std::uint64_t sequence = 0;
    SessionStatus status = SessionStatus::Working;
    unsigned ttlSeconds = 120;
    std::string sourceUrl; // Optional action metadata; never interpreted by the core.
};

struct SessionRecord {
    SessionKey key;
    std::string label;
    std::uint64_t sequence = 0;
    SessionStatus status = SessionStatus::Working;
    double expiresAt = 0;
    double transitionedAt = 0;
    bool acknowledged = false;
    bool cleared = false;
    std::string sourceUrl;
};

enum class SessionResultCode { Accepted, InvalidParameters, StaleSequence, Capacity };
struct SessionResult {
    SessionResultCode code = SessionResultCode::Accepted;
    std::uint64_t sequence = 0;
    unsigned ttlSeconds = 0; // Effective TTL for accepted updates; zero for clears.
    bool accepted() const { return code == SessionResultCode::Accepted; }
};

struct SessionReaction {
    SessionKey key;
    SessionStatus status = SessionStatus::Working;
    double startedAt = 0;
    double endsAt = 0;
};

struct SessionSnapshot {
    std::vector<SessionRecord> active; // Lexical key order; excludes clear tombstones.
    std::optional<SessionRecord> foreground;
    std::optional<SessionReaction> reaction;
};

class Sessions {
public:
    static constexpr std::size_t MaximumRecords = 32;
    static constexpr std::uint64_t MaximumSequence = 9007199254740991ULL;
    explicit Sessions(double now = 0);
    SessionResult update(const SessionUpdate &update, double now);
    SessionResult clear(const SessionKey &key, std::uint64_t sequence, double now);
    bool acknowledge(const SessionKey &key, double now);
    void advance(double now);
    void setSuppressed(bool suppressed, double now);
    // Wake removes records, sequence history, reactions and cooldown. Suppression
    // stays under the shell's control; reset itself never schedules a reaction.
    void reset(double now);
    SessionSnapshot snapshot(double now);
    std::optional<double> nextDeadline() const;
    std::size_t recordCount() const { return records_.size(); }
    static bool validId(const std::string &id);
    static bool validLabel(const std::string &label);

private:
    void validateTime(double now) const;
    const SessionRecord *foreground() const;
    void reconcile();
    std::vector<SessionRecord> records_;
    std::optional<SessionReaction> reaction_;
    std::optional<double> lastReactionStart_;
    double lastNow_;
    bool suppressed_ = false;
};

} // namespace pets
