#pragma once
#include "../characters/expression.h"
#include <cstdint>
#include <optional>
#include <random>
#include <string_view>
#include <vector>
namespace pets::behavior {
enum class Profile { Fallback, Mallow, Bramble, Werewolf };
enum class Activity { Rest, Survey, EarTwitch, TailSwish, Stretch, Explore, Attention };
struct Capabilities { std::uint32_t roles=0; bool reach=false, navigation=false; };
// Rest bounds: 8..120 seconds, min <= max. Weights: 0..100 (0 disables).
// Exploration probability: 0..0.25; cooldown: 45..600 seconds.
// reset rejects nonfinite or out-of-range tuning before changing state.
struct Personality {
    double restMin=8, restMax=20;
    int surveyWeight=3, earWeight=1, tailWeight=2, stretchWeight=1;
    double exploreProbability=.25, exploreCooldown=45;
};
std::optional<Personality> personalityFor(std::string_view disposition);
struct Observation {
    double rootX=0, spanMin=0, spanMax=0, width=1;
    bool suppressed=false, manualBusy=false, reactionBusy=false, blocked=false, arrived=false;
    // Cursor relative to the head: x right, y up, in body-width units; z unused.
    std::optional<motion::Vec3> cursorOffset;
};
struct Command {
    enum class Kind { Look, RoleGoal, RoleImpulse, Clear, StopNavigation, Navigate, Reach, ReleaseReach };
    Kind kind;
    character::Role role=character::Role::Head;
    motion::Vec3 value{};
    double targetX=0;
    int side=0; // 0 left, 1 right; reach position is in reference-body-height units.
};
class Curiosity {
public:
    explicit Curiosity(std::uint32_t seed=1);
    void reset(double now, Profile profile, Capabilities capabilities, std::optional<Personality> personality = {});
    std::vector<Command> advance(double now, const Observation& observation);
    std::optional<double> nextDeadline() const { return deadline_; }
    Activity activity() const { return activity_; }
private:
    double random(double low,double high);
    bool supports(character::Role role) const;
    void rest(double now);
    void clear(std::vector<Command>& commands);
    void start(double now,Activity activity,const Observation& observation,std::vector<Command>& commands);
    Activity choose(double now,const Observation& observation);
    bool canExplore(const Observation& observation) const;
    std::uint32_t seed_;
    std::mt19937 rng_;
    Profile profile_=Profile::Fallback;
    Capabilities capabilities_{};
    std::optional<Personality> personality_;
    Activity activity_=Activity::Rest, previous_=Activity::Rest;
    std::optional<double> deadline_;
    double lastNow_=0, lastExplore_=-1e100, activityEnd_=0;
    bool suspended_=false;
    int phase_=0, reachSide_=0, choicesSinceExplore_=3;
    std::uint32_t ownedRoles_=0;
    bool ownsReach_=false;
    motion::Vec3 cursorLook_{};
};
}
