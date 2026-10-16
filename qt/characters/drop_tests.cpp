#include "grounded.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace pets::character;
static void check(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}
static Definition creature() {
  Definition d;
  d.id = d.name = "drop-test"; d.controller = "soft";
  d.width = d.height = 100; d.origin = {50,60}; d.bindings["body"] = 0;
  d.interaction={{0,0},{100,0},{100,100}};
  d.attachments.push_back({0,{0,0,1,1},{},{},0});
  d.bones = {{"body",-1,{50,60}}, {"lh",0,{-6,0}}, {"lk",1,{0,16}},
             {"la",2,{0,16}}, {"rh",0,{6,0}}, {"rk",4,{0,16}},
             {"ra",5,{0,16}}};
  Locomotion l; l.groundY = 90; l.stride = 12;
  l.legs = {LegChain{1,2,3,{},1},LegChain{4,5,6,{},-1}};
  d.locomotion = l;
  return d;
}
static void tick(GroundedController &g, double dt=1./120) {
  const auto before = g.state(); const auto q = g.propose(dt);
  g.realize(before.rootX + q.displacement, false,
            std::max(0., before.elevation-q.displacementY));
}
static void anatomical(const GroundedController &g) {
  const auto &d = g.definition(); const auto w=worldTransforms(d,g.pose());
  for (auto leg : d.locomotion->legs) {
    for (auto pair : {std::pair<int,int>{leg.hip,leg.knee}, {leg.knee,leg.ankle}}) {
      const auto rest=d.bones[pair.second].rest;
      const double length=std::hypot(w[pair.second].tx-w[pair.first].tx,
                                    w[pair.second].ty-w[pair.first].ty);
      check(std::isfinite(length) && std::abs(length-std::hypot(rest.x,rest.y))<1e-7,
            "detached IK retains anatomical segment lengths");
    }
  }
}
static double legExtension(const GroundedController &g) {
  const auto &d=g.definition(); const auto world=worldTransforms(d,g.pose());
  double total=0;
  for(auto leg:d.locomotion->legs) {
    const auto a=d.bones[leg.knee].rest,b=d.bones[leg.ankle].rest;
    total+=.5*std::hypot(world[leg.ankle].tx-world[leg.hip].tx,
                         world[leg.ankle].ty-world[leg.hip].ty)/
        (std::hypot(a.x,a.y)+std::hypot(b.x,b.y));
  }
  return total;
}
struct DropResult { double time=0, compression=0, pitch=0; int recoverySteps=0; };
static DropResult drop(double height, double speed=1, double dt=1./120) {
  auto d=creature(); d.locomotion->stride = 4*speed;
  GroundedController g(d); g.reset(0,{-1000,1000}); g.setSpeedMultiplier(speed);
  const double rest=g.pose().bones[0].y;
  g.pickup(0,height); g.release({0,0});
  DropResult result; bool landed=false, recovering=false;
  for(int frame=0;frame<6000 && (frame==0 || !g.settled());++frame) {
    const auto before=g.state(); tick(g,dt); const auto after=g.state();
    const auto p=g.pose();
    check(std::isfinite(p.bones[0].y),"drop remains finite");
    check(p.bones[0].y<=d.locomotion->groundY-.3*g.referenceReach()+1e-7,
          "body floor constraint bounds deep impact");
    result.compression=std::max(result.compression,p.bones[0].y-rest);
    result.pitch=std::max(result.pitch,std::abs(p.bones[0].rotation));
    if(after.phase==DropPhase::Airborne) {
      check(after.feet[0].swing && after.feet[1].swing,"airborne feet have no support");
      anatomical(g); result.time += std::min(dt,1./30);
    }
    if(!landed && after.phase==DropPhase::Landing) landed=true;
    if(landed && before.phase!=DropPhase::Airborne) {
      check(!(after.feet[0].swing && after.feet[1].swing),"recovery lifts only one foot at a time");
      for(int side=0;side<2;++side) {
        if(!before.feet[side].swing && !after.feet[side].swing)
          check(after.feet[side].position.x==before.feet[side].position.x &&
                after.feet[side].position.y==before.feet[side].position.y,
                "recovery contacts stay pinned until explicit toeoff");
        if(!before.feet[side].swing && after.feet[side].swing) {
          ++result.recoverySteps;
          check(after.phase==DropPhase::Recovering,"stagger starts during recovery");
          check(std::abs(after.feet[side].landing.x-before.feet[side].position.x)<=.15*g.referenceReach(),
                "stagger is bounded relative to anatomy");
        }
      }
    }
    recovering |= after.phase==DropPhase::Recovering;
    if(before.phase==DropPhase::Airborne && after.phase==DropPhase::Landing)
      check(after.verticalVelocity>0,"touchdown carries incoming support momentum");
  }
  check(landed && recovering,"drop exposes landing and recovery phases");
  check(g.settled() && g.state().phase==DropPhase::Grounded && !g.state().active,
        "recovery ends with no physics deadline");
  return result;
}
int main() {
  try {
    GroundedController sequence(creature()); sequence.pickup(0,500);
    for(int frame=0;frame<200;++frame)tick(sequence);
    const double heldExtension=legExtension(sequence);
    sequence.release({0,0});
    double preparedExtension=0, compressedExtension=1;
    bool contactSeen=false;
    for(int frame=0;frame<1200 && !sequence.settled();++frame) {
      const auto phase=sequence.state().phase;
      if(phase==DropPhase::Airborne)preparedExtension=std::max(preparedExtension,legExtension(sequence));
      if(phase==DropPhase::Landing) {
        contactSeen=true; compressedExtension=std::min(compressedExtension,legExtension(sequence));
      }
      tick(sequence);
    }
    check(preparedExtension>.985 && preparedExtension>heldExtension+.1,
          "falling legs reach nearly straight before impact");
    check(contactSeen && compressedExtension<preparedExtension-.05,
          "pinned legs compress after impact rather than extending at contact");
    std::cout<<"leg sequence held/reach/compression "<<heldExtension<<"/"
             <<preparedExtension<<"/"<<compressedExtension<<"\n";
    auto arms=creature();
    for(auto suffix:{"Left","Right"}) {
      const int shoulder=arms.bones.size();const double sign=std::string(suffix)=="Left" ? -1 : 1;
      arms.bindings[std::string("arm")+suffix]=shoulder;
      arms.bones.push_back({std::string("arm")+suffix,0,{sign*12,-10}});
      arms.bindings[std::string("forearm")+suffix]=shoulder+1;
      arms.bones.push_back({std::string("forearm")+suffix,shoulder,{sign*4,21}});
    }
    GroundedController spread(arms); spread.pickup(0,1000); spread.release({0,0});
    for(int frame=0;frame<30;++frame)tick(spread);
    const auto spreadPose=spread.pose();
    check(spreadPose.bones[7].rotation>.6 && spreadPose.bones[9].rotation<-.6,
          "optional arms spread outward through airborne followers");
    check(spreadPose.bones[8].x==-4 && spreadPose.bones[8].y==21 &&
          spreadPose.bones[10].x==4 && spreadPose.bones[10].y==21,
          "outstretched arm pose keeps anatomical lengths");
    spread.pickup(spread.state().rootX,spread.state().elevation);
    for(int frame=0;frame<2000 && !spread.settled();++frame)tick(spread);
    check(spread.settled() && std::abs(spread.pose().bones[7].rotation)<1e-5,
          "held arm followers return to quiet neutral");
    DropMotion thrown(32); thrown.pickup(10000); thrown.release({1600,-1200});
    check(thrown.velocityX()==1600 && thrown.velocityY()==-1200,
          "strong throw preserves measured velocity instead of anatomy cap");
    for(int frame=0;frame<30;++frame) {
      const double prior=thrown.velocityX();
      const auto step=thrown.advance(1./120);
      check(step.displacementX>0 && thrown.velocityX()<prior && thrown.velocityX()>prior*.97,
            "quadratic air resistance slows each unobstructed throw continuously without a stall");
    }
    DropMotion diagonal(32),horizontal(32),mirrored(32);
    diagonal.pickup(10000);horizontal.pickup(10000);mirrored.pickup(10000);
    diagonal.release({300,400});horizontal.release({500,0});mirrored.release({-300,400});
    const FlightForces cancelledGravity{54*32,0,0};
    for(int frame=0;frame<30;++frame) {
      diagonal.advance(1./120,cancelledGravity);horizontal.advance(1./120,cancelledGravity);
      mirrored.advance(1./120,cancelledGravity);
      check(std::abs(diagonal.velocityX()/diagonal.velocityY()-.75)<1e-10 &&
            std::abs(std::hypot(diagonal.velocityX(),diagonal.velocityY())-horizontal.velocityX())<1e-8,
            "quadratic drag preserves direction and treats equal-speed orientations isotropically");
      check(std::abs(diagonal.velocityX()+mirrored.velocityX())<1e-8 &&
            std::abs(diagonal.velocityY()-mirrored.velocityY())<1e-8,
            "quadratic drag is symmetric for opposite horizontal directions");
    }
    check(std::abs(horizontal.velocityX()-500/(1+(.015/32)*500*.25))<1e-8,
          "isolated drag follows the quadratic differential equation's analytic decay");
    for(double reach:{1.,4.,32.})for(double linear:{.65,20.}) {
      DropMotion combined(reach);combined.pickup(1000000);combined.release({100000000,0});
      combined.advance(1./240,{54*reach,0,linear});
      check(combined.velocityX()>0 && combined.velocityX()<100000000 &&
            std::isfinite(combined.velocityX()) && std::abs(combined.velocityY())<1e-9,
            "combined linear and quadratic damping cannot reverse extreme throw direction");
    }
    DropMotion scale(32);scale.pickup(10000);scale.release({1600,0});double scaleTravel=0;
    for(int frame=0;frame<36;++frame)scaleTravel+=scale.advance(1./120).displacementX;
    std::cout<<"quadratic 1600-unit throw at .3s vx "<<scale.velocityX()
             <<" travel "<<scaleTravel<<" retained fraction "<<scale.velocityX()/1600<<"\n";
    double throwTravel[2]{};
    for(int strength=0;strength<2;++strength) {
      DropMotion fling(32);fling.pickup(500);fling.release({strength==0 ? 800. : 1600.,-400});
      for(int frame=0;frame<600 && fling.phase()==DropPhase::Airborne;++frame)
        throwTravel[strength]+=fling.advance(1./120).displacementX;
      check(fling.phase()==DropPhase::Landing,"quadratic throw reaches swept floor contact");
    }
    check(throwTravel[1]>throwTravel[0]*1.3,"stronger fling travels farther under quadratic drag");
    std::cout<<"quadratic weak/strong throw travel "<<throwTravel[0]<<"/"<<throwTravel[1]<<"\n";
    DropMotion heavier(32); heavier.pickup(500); heavier.release({0,0});
    double fallTime=0;
    while(heavier.phase()==DropPhase::Airborne) { heavier.advance(1./120);fallTime+=1./120; }
    check(fallTime<.8,"500-unit unpowered drop falls in under eight tenths of a second");
    std::cout<<"quadratic complete 500-unit fall time "<<fallTime<<"\n";
    GroundedController g(creature()); g.reset(0,{-500,500}); g.setTargetX(200);
    g.propose(1./60); g.pickup(100,160);
    check(g.state().rootX==100 && g.state().elevation==160,"pickup uses presented placement");
    check(g.state().phase==DropPhase::Held,"pickup enters held");
    check(g.state().feet[0].swing && g.state().feet[1].swing,"pickup detaches floor contacts");
    anatomical(g);
    g.realize(400); check(g.state().rootX==100,"pickup discards old proposal");
    g.hold(110,170); check(g.state().rootX==110 && g.state().elevation==170,"hold follows placement");
    g.release({0,0}); check(g.state().elevation==170,"release preserves elevation");
    const auto q=g.propose(1./60);
    check(q.displacementY>0,"elevated release falls");
    check(g.state().elevation==170,"proposal does not teleport");
    g.realize(110,false,170-q.displacementY); check(g.state().elevation<170,"realized body follows fall");
    for(int i=0;i<200 && !g.settled();++i) { g.pickup(g.state().rootX,g.state().elevation); tick(g); }
    for(int i=0;i<2000 && !g.settled();++i) tick(g);
    check(g.settled() && g.state().phase==DropPhase::Held,"stationary held followers become quiet");
    g.hold(110,0); g.release({0,0});
    check(g.state().phase==DropPhase::Grounded && g.state().elevation==0,"floor release remains supported");
    const auto low=drop(4),high=drop(500),fast=drop(500,3),slow=drop(500,.5);
    check(high.time>low.time && std::abs(high.time-fast.time)<1e-9 && std::abs(high.time-slow.time)<1e-9,
          "fall time scales with height and ignores stride");
    check(high.compression>low.compression+.5 && high.pitch>low.pitch,"larger impacts crouch and pitch more");
    check(high.recoverySteps==1 && low.recoverySteps==0,"large impact takes one recovery step; shallow impact stays planted");
    std::cout<<"low/high fall time "<<low.time<<"/"<<high.time
             <<" compression "<<low.compression<<"/"<<high.compression
             <<" pitch "<<low.pitch<<"/"<<high.pitch
             <<" recovery steps "<<low.recoverySteps<<"/"<<high.recoverySteps<<"\n";
    DropMotion extreme(32);extreme.pickup(1000000);extreme.release({100000000,100000000});
    for(int frame=0;frame<120 && extreme.phase()==DropPhase::Airborne;++frame) {
      const auto step=extreme.advance(.25);
      check(std::isfinite(step.displacementX) && std::isfinite(step.displacementY) &&
            std::isfinite(extreme.velocityX()) && std::isfinite(extreme.velocityY()) &&
            extreme.elevation()>=0 && extreme.velocityX()>=0,
            "extreme quadratic drag remains finite without reversal or floor tunnelling");
    }
    extreme.realizeElevation(0,false);
    check(extreme.phase()==DropPhase::Landing,"explicit raised-floor contact repairs extreme flight safely");
    DropMotion tunnel(32);tunnel.pickup(1);tunnel.release({100000000,100000000});
    const auto crossing=tunnel.advance(.25);
    check(tunnel.phase()==DropPhase::Landing && tunnel.elevation()==0 &&
          std::abs(crossing.displacementY-1)<1e-7 && std::isfinite(crossing.impactVelocity),
          "quadratic extreme impact sweeps first contact rather than tunnelling");
    DropMotion briefContact(32);briefContact.pickup(.0001);briefContact.release({0,1});
    briefContact.advance(1./240,{108*32,0,0});
    check(briefContact.phase()==DropPhase::Landing && briefContact.elevation()==0,
          "swept quadratic contact detects descent before strong lift reverses within a substep");
    drop(1000,1,.25);
    // Re-grab each active phase, with a still-pending proposal.
    for(auto phase:{DropPhase::Airborne,DropPhase::Landing,DropPhase::Recovering}) {
      GroundedController again(creature()); again.pickup(0,100); again.release({80,0});
      for(int i=0;i<1000 && again.state().phase!=phase;++i) tick(again);
      check(again.state().phase==phase,"interrupt phase reached");
      const auto before=again.state(); again.propose(1./60); again.pickup(before.rootX,before.elevation);
      again.realize(450,false,450);
      check(again.state().phase==DropPhase::Held && again.state().rootX==before.rootX &&
            again.state().elevation==before.elevation,"re-grab clears proposal without a jump");
    }
    GroundedController edge(creature()); edge.reset(0,{-500,500}); edge.pickup(499,100); edge.release({100000,-100000});
    auto move=edge.propose(1./30); check(std::abs(move.displacement)<=1,"flight clips to span");
    edge.realize(500,true,100-move.displacementY);
    check(edge.state().velocity==0,"outward clipping removes momentum");
    check(edge.state().verticalVelocity<0,"upward throw retains momentum before ceiling");
    move=edge.propose(1./120);
    edge.realize(500,false,edge.state().elevation-move.displacementY-3);
    check(edge.state().verticalVelocity==0,"top clipping removes upward momentum");
    edge.reconcile(500,{-100,500},10,50); check(edge.state().elevation==50,"floor reconcile preserves presented elevation");
    GroundedController raised(creature()); raised.pickup(0,100); raised.release({0,-64}); tick(raised);
    const double upward=raised.state().verticalVelocity;
    raised.reconcile(0,{-500,500},10,50);
    check(raised.state().verticalVelocity==upward,"geometry reconciliation preserves ascending momentum");
    for(double value:{std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()}) {
      bool rejected=false; try { edge.pickup(0,value); } catch(const std::exception&) { rejected=true; }
      check(rejected,"nonfinite pickup rejected");
    }
    GroundedController turn(creature()); turn.setTargetX(-200); tick(turn);
    check(turn.state().facingBlend!=turn.state().facing,"test interrupts an active facing follower");
    turn.pickup(turn.state().rootX,100);
    for(int i=0;i<2000 && !turn.settled();++i) tick(turn);
    check(turn.settled(),"pickup during reversal settles held facing follower");
    GroundedController contact(creature()); contact.pickup(0,20); contact.release({0,0});
    bool approached=false;
    for(int frame=0;frame<100000 && contact.state().phase==DropPhase::Airborne;++frame) {
      const auto before=contact.state(); const auto proposal=contact.propose(.00001);
      if(proposal.displacementY>=before.elevation-1e-10) {
        const auto world=worldTransforms(contact.definition(),contact.pose());
        for(auto leg:contact.definition().locomotion->legs) {
          const auto sole=world[leg.ankle].apply(leg.contact);
          check(std::abs(sole.y-contact.definition().locomotion->groundY)<1e-5,
                "descending airborne soles approach exact landing contact without a snap");
        }
        const double bodyBefore=contact.pose().bones[0].y;
        const double angleBefore=contact.pose().bones[0].rotation;
        contact.realize(before.rootX+proposal.displacement,false,0);
        const double bodyAfter=contact.pose().bones[0].y;
        check(bodyAfter>=bodyBefore && bodyAfter-bodyBefore<.003,
              "touchdown preserves extended body placement then starts compliant compression");
        check(std::abs(contact.pose().bones[0].rotation-angleBefore)<.0001,
              "impact pitch begins through follower momentum without an angle snap");
        approached=true;
        break;
      }
      contact.realize(before.rootX+proposal.displacement,false,
                      std::max(0.,before.elevation-proposal.displacementY));
    }
    check(approached,"fine-grained contact crossing was exercised");
    GroundedController display(creature()); display.reset(0,{-1000,1000});
    display.reconcile(500,{400,600},0,0);
    check(display.state().feet[0].position.x==494 && display.state().feet[1].position.x==506,
          "display placement repair carries obsolete contact anchors to the new monitor");
    std::cout<<"drop lifecycle, fall, anatomical IK, impact, interruption and quiet recovery passed\n";
  } catch(const std::exception &e) { std::cerr<<"drop check failed: "<<e.what()<<"\n"; return 1; }
}
