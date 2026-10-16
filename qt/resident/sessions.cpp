#include "sessions.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <tuple>

namespace {
std::optional<double> future(double now, double delay) {
    double value = now + delay;
    if (value <= now) value = std::nextafter(now, std::numeric_limits<double>::infinity());
    return std::isfinite(value) ? std::optional<double>(value) : std::nullopt;
}
int priority(const pets::SessionRecord &record) {
    if (record.cleared) return 0;
    switch (record.status) {
    case pets::SessionStatus::NeedsInput: return record.acknowledged ? 0 : 4;
    case pets::SessionStatus::Failed: return record.acknowledged ? 0 : 3;
    case pets::SessionStatus::Completed: return record.acknowledged ? 0 : 2;
    case pets::SessionStatus::Working: return 1;
    }
    return 0;
}
bool validStatus(pets::SessionStatus status) {
    switch (status) {
    case pets::SessionStatus::Working: case pets::SessionStatus::NeedsInput:
    case pets::SessionStatus::Completed: case pets::SessionStatus::Failed: return true;
    }
    return false;
}
} // namespace

namespace pets {
const char *sessionStatusName(SessionStatus status) {
    switch (status) {
    case SessionStatus::Working: return "working";
    case SessionStatus::NeedsInput: return "needs_input";
    case SessionStatus::Completed: return "completed";
    case SessionStatus::Failed: return "failed";
    }
    return "unknown";
}

bool SessionKey::operator==(const SessionKey &other) const {
    return sourceId == other.sourceId && sessionId == other.sessionId;
}
bool SessionKey::operator<(const SessionKey &other) const {
    return std::tie(sourceId, sessionId) < std::tie(other.sourceId, other.sessionId);
}

Sessions::Sessions(double now) : lastNow_(now) {
    if (!std::isfinite(now)) throw std::invalid_argument("session clock must be finite");
    records_.reserve(MaximumRecords);
}

bool Sessions::validId(const std::string &id) {
    if (id.empty() || id.size() > 64) return false;
    for (unsigned char c : id)
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
              || (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-')) return false;
    return true;
}

namespace {
bool validText(const std::string &label, std::size_t byteLimit, unsigned scalarLimit) {
    if (label.size() > byteLimit) return false;
    unsigned count = 0;
    std::size_t i = 0;
    while (i < label.size()) {
        const auto lead = static_cast<unsigned char>(label[i++]);
        std::uint32_t scalar = lead;
        unsigned continuation = 0;
        std::uint32_t minimum = 0;
        if (lead < 0x80) {}
        else if (lead >= 0xc2 && lead <= 0xdf) { scalar = lead & 0x1f; continuation = 1; minimum = 0x80; }
        else if (lead >= 0xe0 && lead <= 0xef) { scalar = lead & 0x0f; continuation = 2; minimum = 0x800; }
        else if (lead >= 0xf0 && lead <= 0xf4) { scalar = lead & 0x07; continuation = 3; minimum = 0x10000; }
        else return false;
        if (continuation > label.size() - i) return false;
        for (unsigned j = 0; j < continuation; ++j) {
            const auto c = static_cast<unsigned char>(label[i++]);
            if ((c & 0xc0) != 0x80) return false;
            scalar = (scalar << 6) | (c & 0x3f);
        }
        if (scalar < minimum || scalar > 0x10ffff || (scalar >= 0xd800 && scalar <= 0xdfff)
            || scalar <= 0x1f || (scalar >= 0x7f && scalar <= 0x9f)) return false;
        if (++count > scalarLimit) return false;
    }
    return true;
}

} // namespace
bool Sessions::validLabel(const std::string &label) { return validText(label, 480, 120); }

void Sessions::validateTime(double now) const {
    if (!std::isfinite(now) || now < lastNow_)
        throw std::invalid_argument("session clock must be finite and nondecreasing");
}

const SessionRecord *Sessions::foreground() const {
    const SessionRecord *best = nullptr;
    for (const auto &record : records_) {
        const auto rank = priority(record);
        if (!rank) continue;
        if (!best || rank > priority(*best)
            || (rank == priority(*best) && (record.transitionedAt > best->transitionedAt
                || (record.transitionedAt == best->transitionedAt && record.key < best->key)))) best = &record;
    }
    return best;
}

void Sessions::reconcile() {
    if (!reaction_) return;
    const auto *current = foreground();
    if (suppressed_ || !current || current->acknowledged || !(current->key == reaction_->key)
        || current->status != reaction_->status) reaction_.reset();
}

void Sessions::advance(double now) {
    validateTime(now);
    lastNow_ = now;
    records_.erase(std::remove_if(records_.begin(), records_.end(),
                                 [now](const SessionRecord &r) { return r.expiresAt <= now; }), records_.end());
    if (reaction_ && reaction_->endsAt <= now) reaction_.reset();
    reconcile();
}

SessionResult Sessions::update(const SessionUpdate &update, double now) {
    advance(now);
    if (!validId(update.key.sourceId) || !validId(update.key.sessionId) || !validLabel(update.label)
        || !validText(update.sourceUrl, 2048, 2048)
        || update.sequence > MaximumSequence || !validStatus(update.status)
        || update.ttlSeconds < 1 || update.ttlSeconds > 3600) return {SessionResultCode::InvalidParameters};
    auto found = std::find_if(records_.begin(), records_.end(),
                              [&](const SessionRecord &r) { return r.key == update.key; });
    if (found != records_.end() && update.sequence <= found->sequence) return {SessionResultCode::StaleSequence};
    if (found == records_.end()) {
        if (records_.size() >= MaximumRecords) return {SessionResultCode::Capacity};
        records_.push_back({});
        found = records_.end() - 1;
        found->cleared = true; // Fresh creation is a status transition.
    }
    const bool transition = found->cleared || found->status != update.status;
    found->key = update.key;
    found->label = update.label;
    found->sourceUrl = update.sourceUrl;
    found->sequence = update.sequence;
    found->status = update.status;
    found->expiresAt = future(now, update.ttlSeconds).value_or(std::numeric_limits<double>::max());
    found->cleared = false;
    if (transition) { found->transitionedAt = now; found->acknowledged = false; }
    reconcile();
    const auto *current = foreground();
    if (transition && current == &*found && !suppressed_ && !found->acknowledged
        && (!lastReactionStart_ || static_cast<long double>(now) - *lastReactionStart_ >= 5)) {
        double end = now + 3;
        // Never enlarge the bounded reaction when a large finite clock rounds up.
        if (end - now > 3) end = std::nextafter(end, now);
        if (std::isfinite(end) && end > now) {
            reaction_ = SessionReaction{found->key, found->status, now, end};
            lastReactionStart_ = now;
        }
    }
    return {SessionResultCode::Accepted, update.sequence, update.ttlSeconds};
}

SessionResult Sessions::clear(const SessionKey &key, std::uint64_t sequence, double now) {
    advance(now);
    if (!validId(key.sourceId) || !validId(key.sessionId) || sequence > MaximumSequence)
        return {SessionResultCode::InvalidParameters};
    auto found = std::find_if(records_.begin(), records_.end(), [&](const SessionRecord &r) { return r.key == key; });
    if (found != records_.end()) {
        if (sequence <= found->sequence) return {SessionResultCode::StaleSequence};
        found->sequence = sequence;
        found->cleared = true;
        reconcile();
    }
    return {SessionResultCode::Accepted, sequence, 0};
}

bool Sessions::acknowledge(const SessionKey &key, double now) {
    advance(now);
    auto found = std::find_if(records_.begin(), records_.end(),
                              [&](const SessionRecord &r) { return !r.cleared && r.key == key; });
    if (found == records_.end()) return false;
    found->acknowledged = true;
    reconcile();
    return true;
}

void Sessions::setSuppressed(bool suppressed, double now) {
    advance(now);
    suppressed_ = suppressed;
    reconcile();
}

void Sessions::reset(double now) {
    validateTime(now);
    lastNow_ = now;
    records_.clear();
    reaction_.reset();
    lastReactionStart_.reset();
}

SessionSnapshot Sessions::snapshot(double now) {
    advance(now);
    SessionSnapshot result;
    result.active.reserve(records_.size());
    for (const auto &record : records_) if (!record.cleared) result.active.push_back(record);
    std::sort(result.active.begin(), result.active.end(), [](const SessionRecord &a, const SessionRecord &b) { return a.key < b.key; });
    if (const auto *current = foreground()) result.foreground = *current;
    result.reaction = reaction_;
    return result;
}

std::optional<double> Sessions::nextDeadline() const {
    std::optional<double> result;
    const auto consider = [&](double value) {
        if (std::isfinite(value) && value > lastNow_ && (!result || value < *result)) result = value;
    };
    for (const auto &record : records_) consider(record.expiresAt);
    if (reaction_) consider(reaction_->endsAt);
    return result;
}
} // namespace pets
