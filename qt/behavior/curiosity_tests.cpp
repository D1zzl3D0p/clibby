#include "curiosity.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace pets::behavior;
using pets::character::Role;
using pets::character::roleBit;
static Capabilities all() { return {63,true,true}; }
static Observation space() { return {0,-10,10,1}; }
static void personalityTests() {
    assert(!personalityFor("legacy") && !personalityFor("unknown"));
    const char* names[]={"calm","friendly","cautious","reserved","curious"};
    const double minima[]={24,8,14,18,10}, maxima[]={45,18,28,36,22};
    for(int n=0;n<5;++n) {
        const auto tuning=personalityFor(names[n]); assert(tuning);
        for(std::uint32_t seed=0;seed<100;++seed) {
            Curiosity c(seed); c.reset(100,Profile::Fallback,{},tuning);
            double now=100;
            for(int i=0;i<20;++i) {
                const double due=*c.nextDeadline();
                assert(due-now>=minima[n]-1e-9 && due-now<=maxima[n]+1e-9);
                assert(c.advance(due-.001,space()).empty());
                assert(c.advance(due,space()).empty());
                now=due;
            }
        }
        // Anatomy still bounds every personality; no wrists means no reach.
        for(auto role:{Role::Head,Role::EarLeft,Role::Tail}) {
            Curiosity c(41); c.reset(0,Profile::Fallback,{roleBit(role),false,false},tuning);
            for(int i=0;i<300;++i) {
                for(auto command:c.advance(*c.nextDeadline(),space())) {
                    assert(command.kind!=Command::Kind::Navigate && command.kind!=Command::Kind::Reach);
                    assert(command.role==role);
                }
            }
        }
        Curiosity held(2); held.reset(0,Profile::Fallback,all(),tuning);
        auto near=space(); near.cursorOffset=pets::motion::Vec3{1,.5,0};
        assert(!held.advance(.1,near).empty() && !held.nextDeadline());
        for(int i=0;i<100;++i) assert(held.advance(.2+i,near).empty());
        near.suppressed=true; held.advance(101,near);
        assert(!held.nextDeadline() && held.advance(10000,near).empty());
        // Exploring is still sparse and obeys each personality's cooldown.
        Curiosity traveler(4); traveler.reset(0,Profile::Fallback,all(),tuning);
        double last=-1e100; int explores=0;
        for(int i=0;i<3000;++i) {
            const double now=*traveler.nextDeadline();
            for(auto command:traveler.advance(now,space())) if(command.kind==Command::Kind::Navigate) {
                assert(now-last>=tuning->exploreCooldown); last=now; ++explores;
            }
        }
        assert(explores>0);
    }
    const auto friendly=*personalityFor("friendly"), curious=*personalityFor("curious");
    assert(friendly.tailWeight>friendly.surveyWeight);
    assert(curious.surveyWeight>curious.tailWeight);
    assert(personalityFor("cautious")->earWeight>personalityFor("friendly")->earWeight);
    assert(personalityFor("reserved")->surveyWeight>personalityFor("reserved")->tailWeight);
    // Zero weights disable even anatomically available actions.
    auto disabled=friendly;
    disabled.surveyWeight=disabled.earWeight=disabled.tailWeight=disabled.stretchWeight=0;
    disabled.exploreProbability=0;
    Curiosity quiet; quiet.reset(0,Profile::Werewolf,all(),disabled);
    for(int i=0;i<300;++i) {
        assert(quiet.advance(*quiet.nextDeadline(),space()).empty());
        assert(quiet.activity()==Activity::Rest);
    }
    // The biases affect choices across identical seeds, not just metadata.
    int friendlyTails=0, curiousTails=0, friendlyHeads=0, curiousHeads=0;
    for(auto name:{"friendly","curious"}) {
        Curiosity c(91);
        c.reset(0,Profile::Fallback,{roleBit(Role::Head)|roleBit(Role::Tail)|roleBit(Role::EarLeft),false,false},personalityFor(name));
        int tails=0,heads=0;
        for(int i=0;i<10000;++i) {
            const auto before=c.activity(); c.advance(*c.nextDeadline(),space());
            if(before==Activity::Rest) { tails+=c.activity()==Activity::TailSwish; heads+=c.activity()==Activity::Survey; }
        }
        if(std::string_view(name)=="friendly") { friendlyTails=tails; friendlyHeads=heads; }
        else { curiousTails=tails; curiousHeads=heads; }
    }
    assert(friendlyTails>curiousTails && curiousHeads>friendlyHeads);
    // Rejected resets must preserve the ongoing state and random sequence.
    for(int field=0;field<12;++field) {
        Curiosity c(6),reference(6); c.reset(0,Profile::Mallow,all()); reference.reset(0,Profile::Mallow,all());
        auto bad=friendly;
        switch(field) {
        case 0: bad.restMin=0; break;
        case 1: bad.restMax=bad.restMin-1; break;
        case 2: bad.restMin=std::numeric_limits<double>::quiet_NaN(); break;
        case 3: bad.restMax=std::numeric_limits<double>::infinity(); break;
        case 4: bad.surveyWeight=-1; break;
        case 5: bad.earWeight=101; break;
        case 6: bad.tailWeight=-1; break;
        case 7: bad.stretchWeight=-1; break;
        case 8: bad.exploreProbability=-.1; break;
        case 9: bad.exploreProbability=.26; break;
        case 10: bad.exploreCooldown=44; break;
        case 11: bad.exploreCooldown=std::numeric_limits<double>::quiet_NaN(); break;
        }
        bool rejected=false;
        try { c.reset(100,Profile::Werewolf,{},bad); } catch(const std::invalid_argument&) { rejected=true; }
        assert(rejected && c.nextDeadline()==reference.nextDeadline() && c.activity()==reference.activity());
        for(int i=0;i<100;++i) {
            assert(c.nextDeadline()==reference.nextDeadline());
            const auto left=c.advance(*c.nextDeadline(),space()),right=reference.advance(*reference.nextDeadline(),space());
            assert(left.size()==right.size() && c.activity()==reference.activity());
            for(size_t j=0;j<left.size();++j)
                assert(left[j].kind==right[j].kind && left[j].role==right[j].role &&
                       left[j].value.x==right[j].value.x && left[j].value.y==right[j].value.y &&
                       left[j].value.z==right[j].value.z && left[j].targetX==right[j].targetX);
        }
    }
}
int main() {
    personalityTests();
    Curiosity a(123),b(123); a.reset(0,Profile::Werewolf,all()); b.reset(0,Profile::Werewolf,all());
    assert(*a.nextDeadline()>=8 && *a.nextDeadline()<=20);
    Activity previous=Activity::Rest; double lastExplore=-1000; int explores=0, choices=0;
    for(int i=0;i<2000;++i) {
        assert(a.nextDeadline()==b.nextDeadline());
        double now=*a.nextDeadline(); auto ca=a.advance(now,space()),cb=b.advance(now,space());
        assert(ca.size()==cb.size() && ca.size()<=8 && a.activity()==b.activity());
        for(size_t j=0;j<ca.size();++j) {
            assert(ca[j].kind==cb[j].kind && ca[j].role==cb[j].role);
            assert(ca[j].value.x==cb[j].value.x && ca[j].value.y==cb[j].value.y && ca[j].value.z==cb[j].value.z);
            assert(ca[j].targetX==cb[j].targetX);
            if(ca[j].kind==Command::Kind::Navigate) {
                assert(std::abs(ca[j].targetX)>=.5 && std::abs(ca[j].targetX)<=1.5);
                assert(now-lastExplore>=45); lastExplore=now; ++explores;
            }
        }
        if(previous==Activity::Rest && a.activity()!=Activity::Rest) ++choices;
        previous=a.activity();
    }
    assert(explores>0 && explores<choices/3);
    Curiosity head(9); head.reset(0,Profile::Mallow,{roleBit(Role::Head),false,false});
    for(int i=0;i<100;++i) {
        auto commands=head.advance(*head.nextDeadline(),space());
        for(auto c:commands) assert(c.kind==Command::Kind::Look || c.kind==Command::Kind::RoleGoal || c.kind==Command::Kind::Clear);
        assert(head.activity()==Activity::Survey || head.activity()==Activity::Rest);
    }
    Curiosity empty; empty.reset(0,Profile::Fallback,{});
    assert(empty.advance(*empty.nextDeadline(),space()).empty());
    Curiosity preempt(8); preempt.reset(0,Profile::Werewolf,all());
    double t=*preempt.nextDeadline(); preempt.advance(t,space());
    auto busy=space(); busy.manualBusy=true;
    auto cleared=preempt.advance(t+.01,busy); assert(!cleared.empty() && !preempt.nextDeadline());
    assert(preempt.advance(t+100,busy).empty());
    assert(preempt.advance(t+101,space()).empty());
    assert(*preempt.nextDeadline()>=t+109 && *preempt.nextDeadline()<=t+121);
    assert(preempt.advance(t+100000,space()).empty()); assert(preempt.activity()==Activity::Rest);
    auto invalid=space(); invalid.width=std::numeric_limits<double>::quiet_NaN();
    auto deadline=preempt.nextDeadline(); assert(preempt.advance(t+100001,invalid).empty()); assert(deadline==preempt.nextDeadline());
    Curiosity travel(4); travel.reset(0,Profile::Werewolf,all());
    for(int i=0;i<500 && travel.activity()!=Activity::Explore;++i) travel.advance(*travel.nextDeadline(),space());
    assert(travel.activity()==Activity::Explore);
    auto blocked=space(); blocked.blocked=true; auto stop=travel.advance(*travel.nextDeadline()-.1,blocked);
    bool stopped=false; for(auto c:stop) stopped|=c.kind==Command::Kind::StopNavigation;
    assert(stopped && travel.activity()==Activity::Rest);
    Curiosity repeats(13); repeats.reset(0,Profile::Bramble,{roleBit(Role::Head)|roleBit(Role::Tail),false,false});
    Activity last=Activity::Rest, before=Activity::Rest;
    for(int i=0;i<100;++i) { repeats.advance(*repeats.nextDeadline(),space()); if(before==Activity::Rest && repeats.activity()!=Activity::Rest) { assert(last!=repeats.activity()); last=repeats.activity(); } before=repeats.activity(); }
    Curiosity navigationOnly(23); navigationOnly.reset(0,Profile::Fallback,{0,false,true});
    int navStarts=0;
    for(int i=0;i<300;++i) {
        for(auto c:navigationOnly.advance(*navigationOnly.nextDeadline(),space()))
            if(c.kind==Command::Kind::Navigate) ++navStarts;
    }
    assert(navStarts>1); // A sole supported activity may repeat after its cooldown.
    Curiosity tiny(91); tiny.reset(0,Profile::Werewolf,all());
    auto noSpace=space(); noSpace.spanMin=-.1; noSpace.spanMax=.1;
    for(int i=0;i<200;++i) {
        for(auto c:tiny.advance(*tiny.nextDeadline(),noSpace)) assert(c.kind!=Command::Kind::Navigate);
    }
    Curiosity arriving(4); arriving.reset(0,Profile::Werewolf,all());
    for(int i=0;i<500 && arriving.activity()!=Activity::Explore;++i) arriving.advance(*arriving.nextDeadline(),space());
    auto arrival=space(); arrival.arrived=true;
    arriving.advance(*arriving.nextDeadline()-1,arrival); assert(arriving.activity()==Activity::Survey);
    Curiosity timeout(4); timeout.reset(0,Profile::Werewolf,all());
    for(int i=0;i<500 && timeout.activity()!=Activity::Explore;++i) timeout.advance(*timeout.nextDeadline(),space());
    auto timeoutCommands=timeout.advance(*timeout.nextDeadline(),space()); assert(timeout.activity()==Activity::Rest);
    stopped=false; for(auto c:timeoutCommands) stopped|=c.kind==Command::Kind::StopNavigation; assert(stopped);
    for(int mode=0;mode<2;++mode) {
        Curiosity interrupted(2); interrupted.reset(0,Profile::Werewolf,all());
        double start=*interrupted.nextDeadline(); interrupted.advance(start,space());
        auto interruption=space(); interruption.suppressed=mode==0; interruption.reactionBusy=mode==1;
        interrupted.advance(start+.1,interruption); assert(!interrupted.nextDeadline());
        interrupted.advance(start+1,space()); assert(*interrupted.nextDeadline()>=start+9);
    }
    Curiosity mallow(1),bramble(1);
    mallow.reset(0,Profile::Mallow,all()); bramble.reset(0,Profile::Bramble,all());
    bool profilesDiffer=false;
    for(int i=0;i<200;++i) {
        mallow.advance(*mallow.nextDeadline(),space()); bramble.advance(*bramble.nextDeadline(),space());
        profilesDiffer|=mallow.activity()!=bramble.activity();
    }
    assert(profilesDiffer);
    // Invalid geometry must never hide a higher-priority interruption.
    for(bool exploring:{false,true}) {
        Curiosity invalidBusy(4); invalidBusy.reset(0,Profile::Werewolf,all());
        if(exploring) {
            for(int i=0;i<500 && invalidBusy.activity()!=Activity::Explore;++i) invalidBusy.advance(*invalidBusy.nextDeadline(),space());
        } else invalidBusy.advance(*invalidBusy.nextDeadline(),space());
        assert(invalidBusy.activity()!=Activity::Rest);
        auto invalidState=space(); invalidState.rootX=100; invalidState.suppressed=true;
        auto cancelled=invalidBusy.advance(*invalidBusy.nextDeadline(),invalidState);
        assert(!cancelled.empty() && invalidBusy.activity()==Activity::Rest && !invalidBusy.nextDeadline());
        if(exploring) {
            bool stopInvalid=false; for(auto c:cancelled) stopInvalid|=c.kind==Command::Kind::StopNavigation;
            assert(stopInvalid);
        }
        invalidBusy.advance(10000,space()); assert(*invalidBusy.nextDeadline()>10000);
    }
    for(bool exploring:{false,true}) {
        Curiosity invalidActive(4); invalidActive.reset(0,Profile::Werewolf,all());
        if(exploring) {
            for(int i=0;i<500 && invalidActive.activity()!=Activity::Explore;++i) invalidActive.advance(*invalidActive.nextDeadline(),space());
        } else invalidActive.advance(*invalidActive.nextDeadline(),space());
        double due=*invalidActive.nextDeadline();
        auto invalidState=space(); invalidState.width=std::numeric_limits<double>::quiet_NaN();
        auto cancelled=invalidActive.advance(due,invalidState);
        assert(!cancelled.empty() && invalidActive.activity()==Activity::Rest);
        assert(!invalidActive.nextDeadline() || *invalidActive.nextDeadline()>due);
        double next=*invalidActive.nextDeadline();
        assert(invalidActive.advance(due+.1,invalidState).empty());
        assert(*invalidActive.nextDeadline()==next);
        invalidActive.advance(next,invalidState); assert(*invalidActive.nextDeadline()>next);
    }
    Curiosity rolling(71); rolling.reset(0,Profile::Werewolf,all());
    std::vector<Activity> starts;
    for(int i=0;i<5000;++i) {
        const auto beforeChoice=rolling.activity();
        rolling.advance(*rolling.nextDeadline(),space());
        if(beforeChoice==Activity::Rest && rolling.activity()!=Activity::Rest) {
            starts.push_back(rolling.activity());
            int rollingExplores=0;
            for(size_t j=starts.size()>4?starts.size()-4:0;j<starts.size();++j)
                rollingExplores+=starts[j]==Activity::Explore;
            assert(rollingExplores<=1);
        }
    }
    Curiosity rewind(4); rewind.reset(0,Profile::Werewolf,all());
    for(int i=0;i<500 && rewind.activity()!=Activity::Explore;++i) rewind.advance(*rewind.nextDeadline(),space());
    rewind.advance(-1000,space()); assert(rewind.activity()==Activity::Rest && *rewind.nextDeadline()>-1000);
    int afterRewind=0; double firstRewindExplore=0;
    for(int i=0;i<500;++i) {
        double advanceTime=*rewind.nextDeadline();
        for(auto c:rewind.advance(advanceTime,space())) {
            if(c.kind==Command::Kind::Navigate) { if(afterRewind==0) firstRewindExplore=advanceTime; ++afterRewind; }
        }
    }
    assert(afterRewind>0 && firstRewindExplore < -700);
    Curiosity stretching(3); stretching.reset(0,Profile::Werewolf,all());
    bool sawReach=false;
    for(int i=0;i<1000 && !sawReach;++i) {
        for(auto c:stretching.advance(*stretching.nextDeadline(),space())) {
            if(c.kind==Command::Kind::Reach) { assert(c.side==0?c.value.x>0:c.value.x<0); sawReach=true; }
        }
    }
    assert(sawReach);
    Curiosity precision(7); precision.reset(1e20,Profile::Mallow,all());
    assert(precision.nextDeadline() && *precision.nextDeadline()>1e20 && std::isfinite(*precision.nextDeadline()));
    auto badPrecision=space(); badPrecision.rootX=100;
    precision.advance(*precision.nextDeadline(),badPrecision);
    assert(precision.nextDeadline() && *precision.nextDeadline()>1e20);
    const double maxTime=std::numeric_limits<double>::max();
    precision.reset(maxTime,Profile::Mallow,all());
    assert(!precision.nextDeadline());
    precision.advance(maxTime,badPrecision); assert(!precision.nextDeadline());
    // A nearby cursor interrupts idle motion, then settles without repeated commands.
    Curiosity cursor(4); cursor.reset(0,Profile::Mallow,all());
    auto near=space(); near.cursorOffset=pets::motion::Vec3{1,.5,0};
    auto interest=cursor.advance(.1,near);
    assert(cursor.activity()==Activity::Attention && !cursor.nextDeadline());
    bool looked=false, tilted=false;
    for(auto c:interest) {
        looked|=c.kind==Command::Kind::Look && c.value.x>0 && c.value.y>0 && c.value.z>0;
        tilted|=c.kind==Command::Kind::RoleGoal && c.role==Role::Head && c.value.z!=0;
        assert(c.kind!=Command::Kind::Navigate);
    }
    assert(looked && tilted);
    for(int i=0;i<100;++i) assert(cursor.advance(.2+i*.1,near).empty());
    near.cursorOffset=pets::motion::Vec3{1.001,.501,0};
    assert(cursor.advance(11,near).empty());
    near.cursorOffset=pets::motion::Vec3{-1,-.5,0};
    auto following=cursor.advance(11.1,near);
    assert(following.size()==1 && following[0].kind==Command::Kind::Look && following[0].value.x<0 && following[0].value.y<0);
    near.cursorOffset=pets::motion::Vec3{2.8,0,0};
    cursor.advance(11.2,near); assert(cursor.activity()==Activity::Attention);
    near.cursorOffset=pets::motion::Vec3{3.1,0,0};
    auto released=cursor.advance(11.3,near);
    assert(!released.empty() && cursor.activity()==Activity::Rest && *cursor.nextDeadline()>=19.3);
    assert(cursor.advance(11.4,near).empty());
    near.cursorOffset=pets::motion::Vec3{2.8,0,0};
    assert(cursor.advance(11.5,near).empty() && cursor.activity()==Activity::Rest);
    near.cursorOffset=pets::motion::Vec3{0,0,0};
    assert(cursor.advance(11.6,near).empty());
    near.cursorOffset=pets::motion::Vec3{1,0,0}; cursor.advance(11.7,near);
    near.reactionBusy=true;
    auto reactionClear=cursor.advance(11.8,near);
    assert(!reactionClear.empty() && !cursor.nextDeadline());
    assert(cursor.advance(12,near).empty());
    near.reactionBusy=false; cursor.advance(12.1,near);
    assert(cursor.activity()==Activity::Rest && *cursor.nextDeadline()>=20.1);
    cursor.advance(12.2,near); assert(cursor.activity()==Activity::Attention);
    near.cursorOffset=pets::motion::Vec3{std::numeric_limits<double>::infinity(),0,0};
    assert(!cursor.advance(12.3,near).empty() && cursor.activity()==Activity::Rest);
    Curiosity cursorExplore(4); cursorExplore.reset(0,Profile::Werewolf,all());
    for(int i=0;i<500 && cursorExplore.activity()!=Activity::Explore;++i) cursorExplore.advance(*cursorExplore.nextDeadline(),space());
    near=space(); near.cursorOffset=pets::motion::Vec3{1,0,0};
    const double exploreTime=*cursorExplore.nextDeadline()-.1;
    auto exploreInterest=cursorExplore.advance(exploreTime,near);
    int stopCount=0; for(auto c:exploreInterest) stopCount+=c.kind==Command::Kind::StopNavigation;
    assert(stopCount==1 && cursorExplore.activity()==Activity::Attention);
    assert(cursorExplore.advance(exploreTime+.1,near).empty());
    Curiosity cursorUnsupported; cursorUnsupported.reset(0,Profile::Fallback,{roleBit(Role::Tail),false,true});
    assert(cursorUnsupported.advance(.1,near).empty() && cursorUnsupported.activity()==Activity::Rest);
    cursorExplore.advance(-100,near);
    assert(cursorExplore.activity()==Activity::Rest && *cursorExplore.nextDeadline()>-100);
    Curiosity headCursor; headCursor.reset(0,Profile::Fallback,{roleBit(Role::Head),false,false});
    near=space(); near.cursorOffset=pets::motion::Vec3{.5,.5,0};
    auto headInterest=headCursor.advance(.1,near);
    assert(headInterest.size()==2);
    near.cursorOffset.reset();
    auto headRelease=headCursor.advance(.2,near);
    assert(headRelease.size()==1 && headRelease[0].kind==Command::Kind::Clear && headRelease[0].role==Role::Head);
    near.cursorOffset=pets::motion::Vec3{1,0,0}; headCursor.advance(.3,near);
    near.cursorOffset=pets::motion::Vec3{0,0,0};
    assert(headCursor.advance(.4,near).empty() && headCursor.activity()==Activity::Attention);
    near.suppressed=true;
    assert(headCursor.advance(.5,near).size()==1 && !headCursor.nextDeadline());
    assert(headCursor.advance(100000,near).empty());
    std::cout<<"curiosity tests passed\n";
}
