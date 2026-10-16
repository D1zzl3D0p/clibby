#include "curiosity.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
namespace pets::behavior {
using character::Role;
using character::roleBit;
Curiosity::Curiosity(std::uint32_t seed):seed_(seed),rng_(seed) {}
double Curiosity::random(double lo,double hi) {
    // Avoid implementation-dependent standard distribution mappings.
    return lo+(hi-lo)*(static_cast<double>(rng_())/4294967296.0);
}
bool Curiosity::supports(Role role) const { return (capabilities_.roles & roleBit(role))!=0; }
void Curiosity::reset(double now,Profile profile,Capabilities capabilities) {
    if(!std::isfinite(now)) return;
    rng_.seed(seed_); profile_=profile; capabilities_=capabilities;
    previous_=Activity::Rest; lastExplore_=-1e100; lastNow_=now; choicesSinceExplore_=3;
    ownedRoles_=0; ownsReach_=false; suspended_=false;
    rest(now);
}
void Curiosity::rest(double now) {
    activity_=Activity::Rest; phase_=0;
    double next=now+random(8,20);
    if(next<=now) next=std::nextafter(now,std::numeric_limits<double>::infinity());
    if(std::isfinite(next)) deadline_=next; else deadline_.reset();
}
void Curiosity::clear(std::vector<Command>& commands) {
    for(int i=0;i<static_cast<int>(Role::Count);++i) {
        const auto role=static_cast<Role>(i);
        if(ownedRoles_ & roleBit(role)) commands.push_back({Command::Kind::Clear,role});
    }
    ownedRoles_=0;
    if(ownsReach_) { Command c{Command::Kind::ReleaseReach}; c.side=reachSide_; commands.push_back(c); ownsReach_=false; }
    if(activity_==Activity::Explore) commands.push_back({Command::Kind::StopNavigation});
}
bool Curiosity::canExplore(const Observation& o) const {
    return capabilities_.navigation &&
        (o.rootX-o.spanMin>=.5*o.width || o.spanMax-o.rootX>=.5*o.width);
}
Activity Curiosity::choose(double now,const Observation& o) {
    std::vector<std::pair<Activity,int>> candidates;
    if(supports(Role::Head)) candidates.push_back({Activity::Survey,profile_==Profile::Mallow?6:3});
    if(supports(Role::EarLeft)||supports(Role::EarRight)) candidates.push_back({Activity::EarTwitch,profile_==Profile::Mallow?2:1});
    if(supports(Role::Tail)) candidates.push_back({Activity::TailSwish,profile_==Profile::Bramble?5:2});
    if(capabilities_.reach && (supports(Role::WristLeft)||supports(Role::WristRight))) candidates.push_back({Activity::Stretch,profile_==Profile::Werewolf?4:1});
    const bool exploration=canExplore(o) && choicesSinceExplore_>=3 && now-lastExplore_>=45 &&
        (previous_!=Activity::Explore || candidates.empty());
    // One dedicated quarter-probability draw caps exploration independently of anatomy.
    if(exploration && random(0,1)<.25) return Activity::Explore;
    if(candidates.size()>1 || (exploration && !candidates.empty())) {
        candidates.erase(std::remove_if(candidates.begin(),candidates.end(),[this](auto c){return c.first==previous_;}),candidates.end());
    }
    if(candidates.empty()) return Activity::Rest;
    int total=0; for(auto c:candidates) total+=c.second;
    double pick=random(0,total);
    for(auto c:candidates) { pick-=c.second; if(pick<0) return c.first; }
    return candidates.back().first;
}
void Curiosity::start(double now,Activity selected,const Observation& o,std::vector<Command>& out) {
    activity_=selected; if(selected!=Activity::Rest) previous_=selected; phase_=0;
    choicesSinceExplore_=selected==Activity::Explore?0:std::min(3,choicesSinceExplore_+1);
    auto roleCommand=[&](Command::Kind kind,Role role,motion::Vec3 value) {
        if(!supports(role)) return;
        out.push_back({kind,role,value}); ownedRoles_|=roleBit(role);
    };
    switch(selected) {
    case Activity::Rest: rest(now); break;
    case Activity::Attention: break; // Started from an observation, never a random choice.
    case Activity::Survey:
        activityEnd_=now+random(2,5); deadline_=std::min(activityEnd_,now+random(.7,1.6));
        if(profile_==Profile::Bramble)
            roleCommand(Command::Kind::RoleGoal,Role::Head,{random(-.15,-.03),random(-.35,.35),random(-.17,.17)});
        else roleCommand(Command::Kind::Look,Role::Head,{random(-.55,.55),random(.03,.3),1});
        break;
    case Activity::EarTwitch: {
        const Role role=supports(Role::EarLeft)&&(!supports(Role::EarRight)||random(0,1)<.5)?Role::EarLeft:Role::EarRight;
        roleCommand(Command::Kind::RoleImpulse,role,{0,0,random(.8,1.6)*(role==Role::EarLeft?1:-1)});
        deadline_=now+random(.5,2); break;
    }
    case Activity::TailSwish:
        roleCommand(Command::Kind::RoleGoal,Role::Tail,{0,random(-.25,.25),random(-.35,.35)});
        deadline_=now+random(.5,2); break;
    case Activity::Stretch: {
        reachSide_=supports(Role::WristLeft)?0:1;
        Command c{Command::Kind::Reach}; c.side=reachSide_; c.value={reachSide_==0?.36:-.36,.78,.15};
        out.push_back(c); ownsReach_=true;
        roleCommand(Command::Kind::RoleGoal,reachSide_==0?Role::WristLeft:Role::WristRight,{0,0,reachSide_==0?-.2:.2});
        roleCommand(Command::Kind::Look,Role::Head,{0,.3,1});
        roleCommand(Command::Kind::RoleGoal,Role::Tail,{.12,0,0});
        deadline_=now+random(2,4); break;
    }
    case Activity::Explore: {
        lastExplore_=now;
        const double left=o.rootX-o.spanMin, right=o.spanMax-o.rootX;
        bool goRight=left<.5*o.width || (right>=.5*o.width && random(0,1)<.5);
        double distance=random(.5,std::min(1.5,(goRight?right:left)/o.width))*o.width;
        Command c{Command::Kind::Navigate}; c.targetX=o.rootX+(goRight?distance:-distance); out.push_back(c);
        deadline_=now+8; break;
    }
    }
}
std::vector<Command> Curiosity::advance(double now,const Observation& o) {
    std::vector<Command> out;
    if(!std::isfinite(now)) return out;
    if(now<lastNow_ && lastExplore_>-1e90)
        lastExplore_=now-std::clamp(lastNow_-lastExplore_,0.0,45.0);
    const bool busy=o.suppressed||o.manualBusy||o.reactionBusy;
    if(busy) {
        if(!suspended_) { clear(out); activity_=Activity::Rest; deadline_.reset(); suspended_=true; }
        lastNow_=now; return out;
    }
    if(suspended_) { suspended_=false; rest(now); lastNow_=now; return out; }
    if(!std::isfinite(o.rootX)||!std::isfinite(o.spanMin)||!std::isfinite(o.spanMax)||
       !std::isfinite(o.width)||o.width<=0||o.spanMin>o.spanMax||o.rootX<o.spanMin||o.rootX>o.spanMax) {
        if(activity_!=Activity::Rest) { clear(out); rest(now); }
        else if(!deadline_ || *deadline_<=now || now<lastNow_) rest(now);
        lastNow_=now;
        return out;
    }
    // Rebase discontinuous clocks; one call never catches up multiple expired phases.
    if(now<lastNow_ || (deadline_ && now>*deadline_+20)) {
        clear(out); rest(now); lastNow_=now; return out;
    }
    lastNow_=now;
    // Hysteresis keeps boundary motion from continually releasing and acquiring attention.
    // The chooser emits targets only; the procedural follower owns the actual movement.
    const bool following=activity_==Activity::Attention;
    const bool finiteCursor=o.cursorOffset && std::isfinite(o.cursorOffset->x) &&
        std::isfinite(o.cursorOffset->y);
    const double distance=finiteCursor?std::hypot(o.cursorOffset->x,o.cursorOffset->y):
        std::numeric_limits<double>::infinity();
    const bool nearby=supports(Role::Head) && distance<=(following?3.0:2.5) &&
        (following || distance>=.08);
    if(nearby) {
        if(!following) {
            clear(out); activity_=Activity::Attention; deadline_.reset();
            // A small held bias conveys interest without oscillation or repeated impulses.
            out.push_back({Command::Kind::RoleGoal,Role::Head,{0,0,.08}});
            ownedRoles_|=roleBit(Role::Head);
            for(auto ear:{Role::EarLeft,Role::EarRight}) {
                if(!supports(ear)) continue;
                out.push_back({Command::Kind::RoleGoal,ear,{-.08,0,ear==Role::EarLeft?-.06:.06}});
                ownedRoles_|=roleBit(ear);
            }
        }
        // Avoid unstable direction changes when the cursor passes through the head.
        if(distance>=.08) {
            motion::Vec3 look{.55*o.cursorOffset->x,.55*o.cursorOffset->y,1};
            const double length=std::hypot(std::hypot(look.x,look.y),look.z);
            look={look.x/length,look.y/length,look.z/length};
            const double delta=std::hypot(std::hypot(look.x-cursorLook_.x,look.y-cursorLook_.y),look.z-cursorLook_.z);
            // Roughly one degree: stationary cursors and tiny pixel noise do not wake animation.
            if(!following || delta>=.02) {
                out.push_back({Command::Kind::Look,Role::Head,look}); cursorLook_=look;
            }
        }
        return out;
    }
    if(following) { clear(out); rest(now); return out; }
    if(activity_==Activity::Explore && (o.blocked||o.arrived)) {
        clear(out);
        if(o.arrived && !o.blocked && supports(Role::Head)) start(now,Activity::Survey,o,out); else rest(now);
        return out;
    }
    if(!deadline_ || now<*deadline_) return out;
    if(activity_==Activity::Rest) { start(now,choose(now,o),o,out); return out; }
    if(activity_==Activity::Survey && now<activityEnd_ && phase_<2) {
        ++phase_; out.push_back({Command::Kind::Look,Role::Head,{random(-.5,.5),random(0,.25),1}});
        deadline_=phase_==2?activityEnd_:std::min(activityEnd_,now+random(.6,1.4)); return out;
    }
    clear(out); rest(now); return out;
}
}
