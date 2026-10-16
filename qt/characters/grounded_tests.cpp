#include "grounded.h"
#include "ik.h"
#include "loader.h"
#include "second_order.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <algorithm>
#include <vector>
using namespace pets::character;
#undef assert
#define assert(condition)                                                      \
  do {                                                                         \
    if (!(condition)) {                                                        \
      std::cerr << "check failed: " << #condition << " at " << __LINE__        \
                << "\n";                                                       \
      return 1;                                                                \
    }                                                                          \
  } while (false)
static Definition creature() {
  Definition d;
  d.id = "test";
  d.name = "test";
  d.controller = "soft";
  d.width = 100;
  d.height = 100;
  d.origin = {50, 60};
  d.bindings["body"] = 0;
  d.interaction = {{0, 0}, {100, 0}, {100, 100}};
  d.bones = {{"body", -1, {50, 60}}, {"lh", 0, {-6, 0}}, {"lk", 1, {0, 16}},
             {"la", 2, {0, 16}},     {"rh", 0, {6, 0}},  {"rk", 4, {0, 16}},
             {"ra", 5, {0, 16}}};
  d.contacts = {{3, {}}, {6, {}}};
  d.attachments.push_back({0, {0, 0, 1, 1}});
  Locomotion l;
  l.groundY = 90;
  l.reachThreshold = 10;
  l.stride = 12;
  l.legs = {LegChain{1, 2, 3, {}, 1}, LegChain{4, 5, 6, {}, -1}};
  d.locomotion = l;
  return d;
}
int main(int argc, char **argv) {
  // Attention changes heading without converting the look goal into travel.
  {
    GroundedController looking(creature());
    assert(!looking.face(0));
    assert(!looking.face(std::numeric_limits<double>::quiet_NaN()));
    assert(looking.face(-100));
    assert(!looking.face(-100));
    assert(!looking.settled());
    for(int frame=0;frame<240;++frame) {
      const auto before=looking.state();
      const auto q=looking.propose(1./120);
      assert(q.displacement==0);
      looking.realize(before.rootX+q.displacement);
      const auto after=looking.state();
      assert(after.rootX==0 && after.targetX==0 && after.velocity==0);
      for(int side=0;side<2;++side) {
        assert(!after.feet[side].swing);
        assert(after.feet[side].position.x==before.feet[side].position.x);
      }
    }
    assert(looking.state().facingBlend==-1 && looking.settled());
    assert(looking.face(100));
    for(int frame=0;frame<240;++frame) {
      const auto q=looking.propose(1./120);
      looking.realize(looking.state().rootX+q.displacement);
    }
    assert(looking.state().facingBlend==1 && looking.settled());
    looking.pickup(0,10);
    assert(!looking.face(-100));
  }
  // A small displacement fits the existing stance; resting must not replant
  // both soles just to reproduce the authored home coordinates.
  {
    GroundedController comfortable(creature());
    comfortable.setTargetX(3);
    for(int frame=0;frame<1500 && !comfortable.settled();++frame) {
      const auto before=comfortable.state();
      const auto q=comfortable.propose(1./120);
      comfortable.realize(before.rootX+q.displacement);
      assert(!comfortable.state().feet[0].swing && !comfortable.state().feet[1].swing);
      assert(comfortable.state().feet[0].position.x == -6);
      assert(comfortable.state().feet[1].position.x == 6);
    }
    assert(comfortable.settled());
  }
  // Canceling a journey before its first frame must also cancel its turn.
  {
    GroundedController canceled(creature());
    canceled.setTargetX(-100); canceled.setTargetX(0); canceled.setTargetX(.5);
    auto q=canceled.propose(1./120); canceled.realize(q.displacement);
    assert(canceled.state().facing==1);
    GroundedController interrupted(creature());
    interrupted.setTargetX(-100); interrupted.pickup(0,10); interrupted.release({0,0});
    for(int tick=0;tick<2400&&!interrupted.settled();++tick) {
      q=interrupted.propose(1./120);
      interrupted.realize(interrupted.state().rootX+q.displacement);
    }
    assert(interrupted.settled());
    q=interrupted.propose(1./120);
    interrupted.realize(interrupted.state().rootX+q.displacement);
    assert(interrupted.state().facing==1);
  }
  // Fractional destinations on an integer-position desktop must settle
  // without changing the requested heading on quantization crossings.
  for(double destination:{20.4,-20.4}) {
    GroundedController quantized(creature());
    quantized.setTargetX(destination);
    double residual=0;
    const int direction=destination>0?1:-1;
    for(int frame=0;frame<2400 && !quantized.settled();++frame) {
      const auto before=quantized.state();
      const auto q=quantized.propose(1./120);
      const double requested=before.rootX+q.displacement+residual;
      const double realized=std::round(requested);
      residual=requested-realized;
      quantized.realize(realized);
      assert(quantized.state().facing==direction);
    }
    assert(quantized.settled());
    assert(std::abs(quantized.state().rootX-destination)<=.75);
    assert(quantized.state().facingBlend==direction);
  }
  for(double pause:{.04,.02,.01,0.}) {
    GroundedController transfer(creature());
    transfer.setFootPlantDuration(pause);transfer.setTargetX(1000);
    constexpr double dt=1./240;
    double touchdown=-1;int transfers=0;
    for(int tick=0;tick<960;++tick) {
      auto before=transfer.state();auto proposal=transfer.propose(dt);
      transfer.realize(before.rootX+proposal.displacement);
      auto after=transfer.state();const double now=(tick+1)*dt;
      assert(!(after.feet[0].swing&&after.feet[1].swing));
      for(int side=0;side<2;++side)
        if(before.feet[side].swing&&!after.feet[side].swing)touchdown=now;
      for(int side=0;side<2;++side)
        if(!before.feet[side].swing&&after.feet[side].swing&&touchdown>=0) {
          assert(now-touchdown>=pause-dt-1e-9);
          assert(now-touchdown<=pause+dt+1e-9);
          ++transfers;
        }
    }
    assert(transfers>=8);
  }
  for(double bad:{-.01,.21,std::numeric_limits<double>::infinity(),
                  std::numeric_limits<double>::quiet_NaN()}) {
    bool rejected=false;
    try{GroundedController(creature()).setFootPlantDuration(bad);}
    catch(const std::exception&){rejected=true;}
    assert(rejected);
  }
  // Body navigation is independent of planted feet and stride reach. Cruise
  // must realize the authored velocity smoothly at every setting.
  std::array<double, 3> travel{};
  for (int multiplier = 1; multiplier <= 3; ++multiplier) {
    GroundedController adjustable(creature());
    adjustable.reset(0, {-2000, 2000});
    adjustable.setSpeedMultiplier(multiplier);
    adjustable.setTargetX(1500);
    for (int frame = 0; frame < 360; ++frame) {
      auto before = adjustable.state();
      auto q = adjustable.propose(1. / 120);
      adjustable.realize(before.rootX + q.displacement);
      auto state = adjustable.state();
      if (frame > 300) {
        assert(std::abs(state.velocity - creature().locomotion->speed * multiplier) < .001);
        assert(std::abs((state.rootX-before.rootX)*120-state.velocity) < .001);
      }
      auto world = worldTransforms(adjustable.definition(), adjustable.pose(Action::Rest));
      for (int side = 0; side < 2; ++side) {
        auto leg = adjustable.definition().locomotion->legs[side];
        assert(std::abs(world[leg.ankle].tx + state.rootX - 50 - state.feet[side].position.x) < .1);
        if (!state.feet[side].swing && !before.feet[side].swing && !state.feet[side].recovering)
          assert(std::abs(state.feet[side].position.x-before.feet[side].position.x) < 1e-8);
      }
    }
    travel[multiplier-1] = adjustable.state().rootX;
  }
  assert(travel[1] > travel[0]*1.7);
  assert(travel[2] > travel[0]*2.4);
  for (double invalid : {0., 3.1, std::numeric_limits<double>::quiet_NaN()}) {
    bool rejected = false;
    try { GroundedController(creature()).setSpeedMultiplier(invalid); }
    catch (...) { rejected = true; }
    assert(rejected);
  }
  for (double fraction : {-1., .31, std::numeric_limits<double>::quiet_NaN()}) {
    bool rejected = false;
    try { GroundedController(creature()).setLandingLeadFraction(fraction); }
    catch (...) { rejected = true; }
    assert(rejected);
  }
  GroundedController authoredLead(creature()), explicitLead(creature());
  explicitLead.setLandingLeadFraction(.3);
  authoredLead.setTargetX(100); explicitLead.setTargetX(100);
  for (int frame = 0; frame < 100; ++frame) {
    auto a = authoredLead.propose(1./120), b = explicitLead.propose(1./120);
    authoredLead.realize(authoredLead.state().rootX+a.displacement);
    explicitLead.realize(explicitLead.state().rootX+b.displacement);
    assert(a.displacement == b.displacement);
    assert(authoredLead.state().feet[0].landing.x == explicitLead.state().feet[0].landing.x);
  }
  // A renderer-owned turn may occupy the lifted foot slot without stopping
  // body travel. Admission closes only new launches, then opens normally.
  {
    GroundedController admission(creature()); admission.setTargetX(100);
    admission.setSwingAdmission(false);
    for (int frame = 0; frame < 30; ++frame) {
      auto q = admission.propose(1./120); admission.realize(admission.state().rootX+q.displacement);
      assert(!admission.state().feet[0].swing && !admission.state().feet[1].swing);
    }
    assert(admission.state().rootX > 0);
    admission.setSwingAdmission(true);
    auto q = admission.propose(1./120); admission.realize(admission.state().rootX+q.displacement);
    assert(admission.state().feet[0].swing != admission.state().feet[1].swing);
  }
  // Contact denial is genuine free fall, independent of spring strength.
  for (double stiffness : {320.,1000.}) {
    GroundedController airborne(creature());
    SupportTuning tuning; tuning.stiffness = stiffness; tuning.damping = 64; tuning.gravityPerReach = 1; tuning.stanceReserveFraction = 0;
    tuning.contactLossGravityScale = 1; tuning.airborneDamping = 0;
    airborne.setSupportTuning(tuning); airborne.reset(0,{-100,100});
    airborne.setSupportContacts({false,false});
    double lastV = 0;
    for (int frame = 0; frame < 100; ++frame) {
      auto q = airborne.propose(1./120);
      airborne.realize(airborne.state().rootX+q.displacement);
      assert(std::abs(airborne.state().verticalAcceleration-32) < 1e-8);
      assert(std::abs(airborne.state().verticalVelocity-lastV-32/120.) < 1e-8);
      lastV = airborne.state().verticalVelocity;
    }
    assert(airborne.pose().bones[0].y > 65.12); // exceeds former midair sag cap
    airborne.setSupportContacts({true,true});
    bool loaded = false;
    // Restoring the contacts must carry load. A deliberately severe impact
    // may rebound; pins must not follow the body to fake a quick settling.
    for (int frame = 0; frame < 30; ++frame) {
      auto q = airborne.propose(1./120);
      airborne.realize(airborne.state().rootX+q.displacement);
      loaded |= airborne.state().verticalAcceleration < 0;
    }
    assert(loaded);
    airborne.reset(0,{-100,100});
    for (int frame = 0; frame < 600 && !airborne.settled(); ++frame) {
      auto q = airborne.propose(1./120); airborne.realize(q.displacement);
    }
    assert(airborne.settled());
  }
  // An extended floor leg retains its damper rather than dropping all force
  // when spring compression crosses zero.
  std::array<pets::motion::SupportLeg,2> extended;
  for (auto &leg : extended) leg = {{0,0,0},{0,10,0},5,5,1,10};
  pets::motion::SupportDynamics force{100,20,32};
  auto beyond = extended; for (auto &leg : beyond) leg.ankle.y += 1e-7;
  double a = pets::motion::supportAcceleration(extended,force,0,1);
  double b = pets::motion::supportAcceleration(beyond,force,0,1);
  assert(std::abs(a-b) < .001 && b < force.gravity);
  // Contact loss softens gravity, but the airborne body keeps descending.
  for (auto &leg : extended) leg.load = 0;
  force.contactLossGravityScale = .5; force.airborneDamping = 8;
  pets::motion::SupportState falling;
  assert(pets::motion::supportAcceleration(extended,force,0,0) == 16);
  for (int frame = 0; frame < 240; ++frame) {
    double before = falling.offset;
    pets::motion::stepSupport(falling,extended,1./120,force);
    assert(falling.offset > before && falling.velocity > 0);
  }
  assert(falling.offset > 2); // air drag permits descent; it is not a height clamp
  GroundedController roomy(creature()); roomy.setFootHomeRadius(4);
  roomy.setTargetX(1);
  for (int frame = 0; frame < 1500 && !roomy.settled(); ++frame) {
    auto q = roomy.propose(1./120); roomy.realize(roomy.state().rootX+q.displacement);
    assert(!roomy.state().feet[0].swing && !roomy.state().feet[1].swing);
  }
  assert(roomy.settled());
  GroundedController paused(creature());
  paused.setTargetX(.01);
  assert(paused.propose(0).displacement == 0);
  assert(paused.state().rootX == 0);
  GroundedController eased(creature());
  eased.reset(0, {-2000, 2000}); eased.setTargetX(400);
  double lastVelocity = 0, lastAcceleration = 0;
  for (int frame = 0; frame < 3000 && !eased.settled(); ++frame) {
    auto q = eased.propose(1. / 120);
    eased.realize(eased.state().rootX+q.displacement);
    double v = eased.state().velocity;
    double acceleration = (v-lastVelocity)*120;
    if (std::abs(v) > .2 || std::abs(lastVelocity) > .2) {
      assert(std::abs(acceleration) <= creature().locomotion->acceleration + .001);
      assert(std::abs(acceleration-lastAcceleration) <= creature().locomotion->acceleration*6/120 + .001);
    }
    lastVelocity = v; lastAcceleration = acceleration;
  }
  if (!eased.settled()) std::cerr << "eased root " << eased.state().rootX << " vy " << eased.state().verticalVelocity << " ay " << eased.state().verticalAcceleration << " y " << eased.pose().bones[0].y << " flags " << eased.state().feet[0].swing << eased.state().feet[1].swing << "\n";
  assert(eased.settled());
  assert(std::abs(eased.state().rootX-400) < .02);
  // Support must yield under a walking load rather than lock the torso height.
  GroundedController supported(creature());
  const double restingY = supported.pose().bones[0].y;
  supported.setTargetX(500);
  double lowest = restingY, highest = restingY;
  for (int frame = 0; frame < 360; ++frame) {
    auto q = supported.propose(1. / 120);
    supported.realize(supported.state().rootX + q.displacement);
    double y = supported.pose().bones[0].y;
    lowest = std::min(lowest, y); highest = std::max(highest, y);
  }
  assert(highest-lowest > .2);
  supported.setTargetX(supported.state().rootX);
  for (int frame = 0; frame < 6000 && !supported.settled(); ++frame) {
    auto q = supported.propose(1. / 120);
    supported.realize(supported.state().rootX + q.displacement);
  }
  assert(supported.settled());
  assert(std::abs(supported.pose().bones[0].y-restingY) < .02);
  // A small integration interval exposes endpoint easing directly through
  // the realized contact, rather than duplicating trajectory formulas.
  auto endpoint = creature();
  endpoint.bones[1].rest.x = endpoint.bones[4].rest.x = 0;
  GroundedController toeOff(endpoint);
  toeOff.setTargetX(100);
  auto tiny = toeOff.propose(.0001);
  toeOff.realize(toeOff.state().rootX+tiny.displacement);
  assert(toeOff.state().feet[0].swing);
  assert(std::abs(toeOff.state().feet[0].position.y) < 1e-6);
  assert(std::abs(toeOff.state().feet[0].position.x) < 1e-6);
  auto ik = solveTwoLink({0, 0}, {12, 8}, 10, 10, 1);
  assert(std::hypot(ik.knee.x, ik.knee.y) > 9.999);
  assert(std::hypot(ik.end.x - 12, ik.end.y - 8) < 1e-6);
  auto unreachable = solveTwoLink({0, 0}, {100, 0}, 10, 10, -1);
  assert(std::isfinite(unreachable.knee.y));
  assert(unreachable.end.x < 20);
  SecondOrderTracker tracker;
  tracker.reset(0);
  for (int i = 0; i < 2000; i++)
    tracker.step(1.0 / 120, 1);
  assert(tracker.settled(1));
  SecondOrderTracker anticipation({3, .8, -3});
  anticipation.step(1. / 120, 1);
  anticipation.step(1. / 120, 1);
  assert(anticipation.value() < 0);
  SecondOrderTracker bouncy({3, .2, 0});
  double peak = 0;
  for (int i = 0; i < 2000; i++) {
    peak = std::max(peak, bouncy.step(1. / 120, 1));
  }
  assert(peak > 1.1);
  assert(bouncy.settled(1));
  SecondOrderTracker rapid({30, .1, 4});
  for (int i = 0; i < 4000; i++) {
    double value = rapid.step(1. / 120, 1);
    assert(std::isfinite(value));
    assert(std::abs(value) < 10);
  }
  assert(rapid.settled(1));
  SecondOrderTracker lag, bounded;
  lag.impulse(4);
  bounded.impulse(4);
  assert(lag.step(100, 1) == bounded.step(1. / 30, 1));
  bool badInput = false;
  try {
    lag.step(std::numeric_limits<double>::quiet_NaN(), 1);
  } catch (...) {
    badInput = true;
  }
  assert(badInput);
  badInput = false;
  try {
    lag.setTuning({0, .8, 0});
  } catch (...) {
    badInput = true;
  }
  assert(badInput);
  auto asymmetric = creature();
  asymmetric.bones[2].rest.y = 1;
  asymmetric.bones[3].rest.y = 31;
  asymmetric.bones[5].rest.y = 1;
  asymmetric.bones[6].rest.y = 31;
  asymmetric.locomotion->lift = 23;
  bool unsupported = false;
  try {
    validateDefinition(asymmetric);
  } catch (...) {
    unsupported = true;
  }
  assert(unsupported);
  // Unequal anatomical segments are valid when the entire swing clears the
  // chain's fold radius, and their contacts still agree with the posed bones.
  auto unequal = creature();
  unequal.bones[2].rest.y = unequal.bones[5].rest.y = 18;
  unequal.bones[3].rest.y = unequal.bones[6].rest.y = 14;
  bool accepted = true;
  try { validateDefinition(unequal); } catch (...) { accepted = false; }
  assert(accepted);
  GroundedController unequalMotion(unequal);
  unequalMotion.setTargetX(70);
  for (int frame = 0; frame < 1200 && !unequalMotion.settled(); ++frame) {
    auto q = unequalMotion.propose(1. / 120);
    unequalMotion.realize(unequalMotion.state().rootX + q.displacement);
    auto w = worldTransforms(unequal, unequalMotion.pose(Action::Rest));
    for (int side = 0; side < 2; ++side) {
      auto leg = unequal.locomotion->legs[side];
      auto contact = w[leg.ankle].apply(leg.contact);
      auto state = unequalMotion.state();
      assert(std::abs(contact.x + state.rootX - 50 - state.feet[side].position.x) < .1);
      assert(std::abs(contact.y - 90 - state.feet[side].position.y) < .1);
      assert(std::hypot(w[leg.knee].tx-w[leg.hip].tx,
                                w[leg.knee].ty-w[leg.hip].ty) >= 18-1e-7);
      assert(std::hypot(w[leg.ankle].tx-w[leg.knee].tx,
                                w[leg.ankle].ty-w[leg.knee].ty) >= 14-1e-7);
    }
  }
  assert(unequalMotion.settled());
  // The torso should lead the hips as movement starts, and retain a small
  // forward inclination during cruise rather than leaning only after acceleration.
  for (int direction : {-1, 1}) {
    GroundedController coordinated(creature());
    coordinated.reset(0, {-500, 500});
    coordinated.setTargetX(direction * 450);
    auto q = coordinated.propose(1. / 120);
    coordinated.realize(coordinated.state().rootX + q.displacement);
    assert(direction * coordinated.pose(Action::Rest).bones[0].rotation > .01);
    for (int frame = 0; frame < 180; ++frame) {
      q = coordinated.propose(1. / 120);
      coordinated.realize(coordinated.state().rootX + q.displacement);
    }
    assert(direction * coordinated.pose(Action::Rest).bones[0].rotation > .025);
  }
  // A planted character must prepare support before its body has travelled
  // the reach threshold. Landing should lead the root in either direction.
  for (int direction : {-1, 1}) {
    GroundedController preparing(creature());
    preparing.reset(0, {-100, 100});
    preparing.setTargetX(direction * 70);
    bool started = false;
    for (int frame = 0; frame < 12 && !started; frame++) {
      auto q = preparing.propose(1. / 120);
      preparing.realize(preparing.state().rootX + q.displacement);
      auto s = preparing.state();
      for (int foot = 0; foot < 2; foot++)
        if (s.feet[foot].swing) {
          started = true;
          assert(std::abs(s.rootX) < 1);
          double rest = foot == 0 ? -6 : 6;
          assert(direction * (s.feet[foot].landing.x - rest - s.rootX) >= -1e-8);
          assert(direction * (s.feet[foot].landing.x - rest) <= 70);
        }
    }
    assert(started);
  }
  // Both knees flex toward travel in a side view. Reversing the visual branch
  // must change the pole direction without moving the planted contact anchors.
  auto anatomical = creature();
  anatomical.locomotion->groundY = 92;
  for (auto leg : anatomical.locomotion->legs) {
    anatomical.bones[leg.knee].rest.x = 3;
    anatomical.bones[leg.ankle].rest.x = -3;
  }
  anatomical.locomotion->legs[0].bend = anatomical.locomotion->legs[1].bend = -1;
  GroundedController bending(anatomical);
  bending.setTargetX(-70);
  for (int frame = 0; frame < 30; ++frame) {
    auto q = bending.propose(1. / 120);
    bending.realize(bending.state().rootX + q.displacement);
  }
  auto reversedWorld = worldTransforms(anatomical, bending.pose(Action::Rest));
  for (auto leg : anatomical.locomotion->legs)
    assert(reversedWorld[leg.knee].tx <
           (reversedWorld[leg.hip].tx + reversedWorld[leg.ankle].tx) * .5);
  GroundedController turning(creature());
  turning.reset(0, {-100, 100});
  turning.setTargetX(-70);
  auto turnProposal = turning.propose(1. / 120);
  turning.realize(turning.state().rootX + turnProposal.displacement);
  assert(turning.state().facing == -1);
  assert(turning.state().facingBlend < 1 && turning.state().facingBlend > 0);
  double previousFacing = turning.state().facingBlend;
  for (int frame = 0; frame < 30; frame++) {
    auto q = turning.propose(1. / 120);
    turning.realize(turning.state().rootX + q.displacement);
    double facing = turning.state().facingBlend;
    assert(facing <= previousFacing);
    assert(std::abs(facing - previousFacing) <= .1);
    previousFacing = facing;
  }
  assert(turning.state().facingBlend == -1);
  turning.setTargetX(70);
  auto qTurn = turning.propose(1. / 120);
  turning.realize(turning.state().rootX + qTurn.displacement);
  assert(turning.state().facing == 1);
  assert(turning.state().facingBlend > -1 && turning.state().facingBlend < 0);
  GroundedController g(creature());
  g.reset(0, {-100, 100});
  g.setTargetX(70);
  auto prev = g.state();
  for (int i = 0; i < 2400 && !g.settled(); i++) {
    auto proposal = g.propose(1.0 / 120);
    g.realize(g.state().rootX + proposal.displacement);
    auto s = g.state();
    auto p = g.pose();
    auto world = worldTransforms(creature(), p);
    for (int j = 0; j < 2; j++) {
      if (!s.feet[j].swing && !prev.feet[j].swing && !s.feet[j].recovering)
        assert(std::abs(s.feet[j].position.x - prev.feet[j].position.x) < .001);
      auto point = world[j == 0 ? 3 : 6].apply({});
      assert(std::abs(point.x + s.rootX - 50 - s.feet[j].position.x) < .1);
      assert(std::abs(point.y - 90 - s.feet[j].position.y) < .1);
    }
    prev = s;
  }
  auto reaction = g.pose(Action::Celebrate, .25);
  auto worldReaction = worldTransforms(creature(), reaction);
  for (int i : {3, 6})
    assert(std::abs(worldReaction[i].ty - 90) < .1);
  assert(std::abs(g.state().rootX - 70) < .5);
  assert(g.settled());
  g.setTargetX(-1000);
  for (int i = 0; i < 4000 && !g.settled(); i++) {
    auto q = g.propose(1.0 / 120);
    g.realize(g.state().rootX + q.displacement);
  }
  assert(g.settled());
  assert(g.state().rootX >= -100);
  g.reset(0, {-100, 100});
  g.setTargetX(90);
  for (int i = 0; i < 100; i++) {
    auto q = g.propose(1. / 120);
    g.realize(g.state().rootX + q.displacement);
  }
  g.setTargetX(-50);
  for (int i = 0; i < 3000 && !g.settled(); i++) {
    auto q = g.propose(1. / 120);
    g.realize(g.state().rootX + q.displacement);
    assert(!(g.state().feet[0].swing && g.state().feet[1].swing));
  }
  assert(g.settled());
  assert(std::abs(g.state().rootX + 50) < .5);
  g.setTargetX(90);
  g.propose(1);
  g.realize(g.state().rootX, true);
  for (int i = 0; i < 3000 && !g.settled(); i++) {
    auto q = g.propose(1. / 120);
    g.realize(g.state().rootX + q.displacement);
  }
  assert(g.settled());
  g.impulse({1, -1}, 4);
  assert(!g.settled());
  for (int i = 0; i < 3000 && !g.settled(); i++) {
    auto q = g.propose(1. / 120);
    g.realize(g.state().rootX + q.displacement);
    worldTransforms(creature(), g.pose());
  }
  assert(g.settled());
  g.impulse({1, 0}, 4);
  g.reset(g.state().rootX, {-100, 100}, 0, true);
  assert(!g.settled());
  g.reset(0, {-100, 100});
  assert(g.settled());
  auto fastDefinition = creature();
  fastDefinition.locomotion->speed = 500;
  fastDefinition.locomotion->acceleration = 2000;
  GroundedController fast(fastDefinition);
  fast.reset(0, {-1000, 1000});
  fast.setTargetX(500);
  for (int i = 0; i < 200; i++) {
    auto q = fast.propose(1. / 30);
    fast.realize(fast.state().rootX + q.displacement);
    auto world = worldTransforms(fastDefinition, fast.pose());
    for (int j = 0; j < 2; j++) {
      if (std::abs(world[j == 0 ? 3 : 6].tx + fast.state().rootX - 50 -
                   fast.state().feet[j].position.x) >= .1)
        std::cerr << "fast frame " << i << " root " << fast.state().rootX
                  << " foot " << j << " anchor "
                  << fast.state().feet[j].position.x << " rendered "
                  << world[j == 0 ? 3 : 6].tx + fast.state().rootX - 50 << "\n";
      assert(std::abs(world[j == 0 ? 3 : 6].tx + fast.state().rootX - 50 -
                      fast.state().feet[j].position.x) < .1);
    }
  }
  bool rejected = false;
  auto invalid = creature();
  invalid.locomotion->legs[1].knee = 2;
  try {
    validateDefinition(invalid);
  } catch (...) {
    rejected = true;
  }
  assert(rejected);
  invalid = creature();
  invalid.bones[0].rest.scaleX = 2;
  rejected = false;
  try {
    validateDefinition(invalid);
  } catch (...) {
    rejected = true;
  }
  assert(rejected);
  if (argc > 1) {
    auto d = loadCharacter(QString::fromLocal8Bit(argv[1])).definition;
    {
      std::array<double,2> peak{{1,1}};
      for(int profile=0;profile<2;++profile) {
        GroundedController walk(d);walk.reset(0,{-3000,3000});
        if(profile==0)walk.setFootPlantDuration(.04);
        walk.setSpeedMultiplier(2);walk.setTargetX(2500);
        for(int frame=0;frame<720;++frame) {
          auto before=walk.state();auto move=walk.propose(1./120);
          walk.realize(before.rootX+move.displacement);
          if(frame<240)continue;
          const auto posed=walk.pose();
          for(const auto& leg:d.locomotion->legs)for(int child:{leg.knee,leg.ankle}) {
            const auto rest=d.bones[child].rest,actual=posed.bones[child];
            peak[profile]=std::max(peak[profile],std::hypot(actual.x,actual.y)/std::hypot(rest.x,rest.y));
          }
        }
      }
      assert(peak[1]-1 < (peak[0]-1)*.85);
    }
    for (double setting : {.5,1.,2.,3.}) {
      GroundedController alternating(d); alternating.reset(0,{-3000,3000});
      alternating.setSpeedMultiplier(setting); alternating.setTargetX(2500);
      int launches[2]{};
      int lastLaunch = -1; double lastLanding = -100, elapsed = 0, launchTime = -100;
      for (int frame = 0; frame < 720; ++frame) {
        auto before = alternating.state(); auto q = alternating.propose(1./120);
        alternating.realize(before.rootX+q.displacement); elapsed += 1./120;
        auto after = alternating.state();
        assert(!(after.feet[0].swing && after.feet[1].swing));
        for (int side = 0; side < 2; ++side) {
          if (before.feet[side].swing && !after.feet[side].swing) lastLanding = elapsed;
          if (!before.feet[side].swing && after.feet[side].swing) {
            assert(lastLaunch != side);
            if (lastLaunch >= 0) assert(elapsed-launchTime >= d.locomotion->maxStepDuration*.5-1e-8);
            launchTime = elapsed;
            lastLaunch = side;
            ++launches[side];
          }
        }
      }
      assert(launches[0] >= 7 && launches[1] >= 7);
    }
    // Ground contacts outrank anatomical reach: even a large host displacement
    // leaves the supporting rendered sole pinned while the other foot travels.
    {
      GroundedController pins(d); pins.reset(0,{-3000,3000}); pins.setTargetX(2500);
      bool stretched = false;
      for (int frame = 0; frame < 180; ++frame) {
        auto before = pins.state(); auto q = pins.propose(1./120);
        pins.realize(before.rootX+q.displacement+(frame == 8 ? 120 : 0));
        auto after = pins.state(); auto world = worldTransforms(d,pins.pose());
        assert(!(after.feet[0].swing && after.feet[1].swing));
        for (int side = 0; side < 2; ++side) {
          auto leg = d.locomotion->legs[side];
          auto contact = world[leg.ankle].apply(leg.contact);
          auto authored = d.bones[leg.knee].rest;
          stretched |= std::hypot(world[leg.knee].tx-world[leg.hip].tx,
                                   world[leg.knee].ty-world[leg.hip].ty) > std::hypot(authored.x,authored.y)+1;
          assert(std::abs(contact.x+after.rootX-d.origin.x-after.feet[side].position.x)<1e-7);
          assert(std::abs(contact.y-d.locomotion->groundY-after.feet[side].position.y)<1e-7);
          if (!before.feet[side].swing && !after.feet[side].swing) {
            assert(after.feet[side].position.x == before.feet[side].position.x);
            assert(after.feet[side].position.y == 0);
          }
        }
      }
      assert(stretched);
    }
    // A steady cruise must keep transferring weight visibly after startup,
    // without counting the initial settling drop as walking bounce.
    {
      GroundedController bob(d); bob.reset(0,{-3000,3000});
      bob.setSpeedMultiplier(2); bob.setTargetX(2500);
      double minimum = 1e9, maximum = -1e9, previousY = 0;
      int previousDirection = 0, directionChanges = 0;
      for (int frame = 0; frame < 720; ++frame) {
        auto q = bob.propose(1./120); bob.realize(bob.state().rootX+q.displacement);
        if (frame >= 240) {
          double y = bob.pose().bones[d.bindings.at("body")].y;
          minimum = std::min(minimum,y); maximum = std::max(maximum,y);
          if (frame > 240 && std::abs(y-previousY) > 1e-5) {
            int direction = y > previousY ? 1 : -1;
            if (previousDirection && direction != previousDirection) ++directionChanges;
            previousDirection = direction;
          }
          previousY = y;
        }
      }
      std::cerr << "sustained cruise bob " << d.id << " " << maximum-minimum << "\n";
      assert(maximum-minimum > (d.id == "soft-creature" ? .7 : 1.2));
      assert(directionChanges >= 8);
    }
    // Walking must leave a visible supported leg below each hip rather than
    // letting the body sag onto the floor during single-support steps.
    for (double speed : {1.,2.,3.}) for (bool quantized : {false,true}) {
      double fraction = 0;
      std::vector<double> extensions;
      GroundedController clearance(d); clearance.reset(0,{-3000,3000});
      clearance.setSpeedMultiplier(speed); clearance.setTargetX(2500);
      auto idleWorld = worldTransforms(d,clearance.pose());
      for (auto leg : d.locomotion->legs) {
        double reach = std::hypot(d.bones[leg.knee].rest.x,d.bones[leg.knee].rest.y)+
                       std::hypot(d.bones[leg.ankle].rest.x,d.bones[leg.ankle].rest.y);
        double extension = std::hypot(idleWorld[leg.ankle].tx-idleWorld[leg.hip].tx,
                                      idleWorld[leg.ankle].ty-idleWorld[leg.hip].ty)/reach;
        assert(extension >= .94 && extension <= .980001);
      }
      for (int frame = 0; frame < 720; ++frame) {
        auto beforeFeet = clearance.state().feet;
        auto q = clearance.propose(1./120);
        double requested = clearance.state().rootX+q.displacement+fraction;
        double realized = quantized ? std::round(requested) : requested;
        fraction = requested-realized;
        clearance.realize(realized);
        auto posed = clearance.pose(); auto w = worldTransforms(d,posed);
        for (int side = 0; side < 2; ++side)
          if (!beforeFeet[side].swing && !clearance.state().feet[side].swing &&
              beforeFeet[side].position.y == 0 && clearance.state().feet[side].position.y == 0)
            assert(std::abs(clearance.state().feet[side].position.x-beforeFeet[side].position.x) < 1e-8);
        for (int side = 0; side < 2; ++side) {
          auto foot = clearance.state().feet[side];
          if (foot.swing && foot.phase < 1) {
            double previousPhase = beforeFeet[side].swing ? beforeFeet[side].phase : 0;
            assert(std::abs(foot.phase-previousPhase-1./(120*d.locomotion->maxStepDuration)) < 1e-6);
          }
        }
        for (auto leg : d.locomotion->legs) {
          double reach = std::hypot(d.bones[leg.knee].rest.x,d.bones[leg.knee].rest.y)+
                         std::hypot(d.bones[leg.ankle].rest.x,d.bones[leg.ankle].rest.y);
          if (!(d.locomotion->groundY-leg.contact.y-w[leg.hip].ty > .72*reach)) std::cerr << "sag frame " << frame << " speed " << speed << " vy " << clearance.state().verticalVelocity << " y " << posed.bones[d.bindings.at("body")].y << "\n";
          assert(d.locomotion->groundY-leg.contact.y-w[leg.hip].ty > .72*reach);
          int side = leg.hip == d.locomotion->legs[0].hip ? 0 : 1;
          if (!clearance.state().feet[side].swing && clearance.state().feet[side].position.y == 0)
            {
            double extension = std::hypot(w[leg.ankle].tx-w[leg.hip].tx,
                                          w[leg.ankle].ty-w[leg.hip].ty)/reach;
            assert(extension > .8 && std::isfinite(extension));
            extensions.push_back(extension);
          }
        }
        // The whole trunk texture rectangle clears the floor, a conservative
        // bound that also covers its opaque pixels without an alpha threshold.
        auto torso = d.bindings.at("facing");
        for (auto attachment : d.attachments) if (attachment.bone == torso) {
          for (double x : {0., double(attachment.source.width)})
            for (double y : {0., double(attachment.source.height)}) {
              auto point = w[torso].apply({x-attachment.pivot.x+attachment.offset.x,
                                          y-attachment.pivot.y+attachment.offset.y});
              if (!(point.y < d.locomotion->groundY-1.)) std::cerr << "floor frame " << frame << " speed " << speed << " vy " << clearance.state().verticalVelocity << " y " << posed.bones[d.bindings.at("body")].y << " point " << point.y << " flags " << clearance.state().feet[0].swing << clearance.state().feet[1].swing << "\n";
              assert(point.y < d.locomotion->groundY-1.);
            }
        }
      }
      std::sort(extensions.begin(),extensions.end());
      assert(!extensions.empty());
      std::cerr << "planted extension " << d.id << " speed " << speed << " q " << quantized << " median " << extensions[extensions.size()/2] << " min " << extensions.front() << "\n";
      assert(extensions[extensions.size()/2] >= .93);

    }
    // A forward arrival must not overshoot and then turn around to correct it.
    for (double setting : {.5,1.,2.,3.}) for (double destination : {20.,90.,500.})
        for (double dt : {1./120,1./30}) {
      GroundedController arrival(d);
      arrival.reset(0, {-1000,1000});
      arrival.setSpeedMultiplier(setting); arrival.setTargetX(destination);
      for (int frame = 0; frame < 18000 && !arrival.settled(); ++frame) {
        auto previous = arrival.state();
        auto q = arrival.propose(dt);
        arrival.realize(previous.rootX+q.displacement);
        assert(arrival.state().rootX <= destination+.00001);
        assert(arrival.state().rootX >= previous.rootX-.00001);
        assert(arrival.state().facing == 1);
      }
      assert(arrival.settled());
    }
    // Loaded anatomical packages must travel at requested speed with a stable
    // torso and exact contacts, extending segments at saturated reach.
    for (double setting : {.5, 1., 2., 3.}) for (bool quantized : {false, true}) {
      GroundedController follow(d);
      follow.reset(0, {-3000, 3000});
      follow.setSpeedMultiplier(setting);
      follow.setTargetX(2500);
      double fraction = 0;
      auto previous = follow.state();
      double bodyY = 0;
      for (int frame = 0; frame < 720; ++frame) {
        auto q = follow.propose(1. / 120);
        double requested = previous.rootX + q.displacement + fraction;
        double realized = quantized ? std::round(requested) : requested;
        fraction = requested-realized;
        follow.realize(realized);
        auto current = follow.state();
        assert(!(current.feet[0].position.y < 0 && current.feet[1].position.y < 0));
        auto pose = follow.pose(Action::Celebrate, .25);
        auto world = worldTransforms(d, pose);
        if (frame > 360) {
          assert(std::abs(current.velocity-d.locomotion->speed*setting) < .001);
          if (!quantized) assert(std::abs(q.displacement*120-current.velocity) < .001);
          assert(std::abs(pose.bones[d.bindings.at("body")].y-bodyY) < 3.);
        }
        bodyY = pose.bones[d.bindings.at("body")].y;
        for (int side = 0; side < 2; ++side) {
          auto leg = d.locomotion->legs[side];
          auto contact = world[leg.ankle].apply(leg.contact);
          if (std::hypot(contact.x+current.rootX-d.origin.x-current.feet[side].position.x,
                           contact.y-d.locomotion->groundY-current.feet[side].position.y) > .001)
            std::cerr << "contact mismatch frame=" << frame << " setting=" << setting << " side=" << side << " dx=" << contact.x+current.rootX-d.origin.x-current.feet[side].position.x << " dy=" << contact.y-d.locomotion->groundY-current.feet[side].position.y << " swing=" << current.feet[side].swing << "\n";
          assert(std::abs(contact.x+current.rootX-d.origin.x-current.feet[side].position.x) < .001);
          assert(std::abs(contact.y-d.locomotion->groundY-current.feet[side].position.y) < .001);
          auto kneeRest = d.bones[leg.knee].rest, ankleRest = d.bones[leg.ankle].rest;
          assert(std::hypot(world[leg.knee].tx-world[leg.hip].tx,
              world[leg.knee].ty-world[leg.hip].ty) >= std::hypot(kneeRest.x,kneeRest.y)-1e-8);
          assert(std::hypot(world[leg.ankle].tx-world[leg.knee].tx,
              world[leg.ankle].ty-world[leg.knee].ty) >= std::hypot(ankleRest.x,ankleRest.y)-1e-8);
          if (!previous.feet[side].swing && !current.feet[side].swing)
            assert(std::abs(previous.feet[side].position.x-current.feet[side].position.x) < 1e-8);
        }
        previous = current;
      }
      assert(follow.state().rootX > d.locomotion->speed*setting*5);
      follow.setTargetX(-200);
      for (int frame = 0; frame < 18000 && !follow.settled(); ++frame) {
        auto before = follow.state();
        auto q = follow.propose(1. / 120);
        follow.realize(before.rootX+q.displacement);
        auto after = follow.state(); auto world = worldTransforms(d,follow.pose());
        assert(!(after.feet[0].swing && after.feet[1].swing));
        for (int side = 0; side < 2; ++side) {
          auto leg = d.locomotion->legs[side];
          auto contact = world[leg.ankle].apply(leg.contact);
          assert(std::abs(contact.x+after.rootX-d.origin.x-after.feet[side].position.x)<1e-7);
          assert(std::abs(contact.y-d.locomotion->groundY-after.feet[side].position.y)<1e-7);
          if (!before.feet[side].swing && !after.feet[side].swing)
            assert(after.feet[side].position.x == before.feet[side].position.x);
          if (!after.feet[side].swing) assert(after.feet[side].position.y == 0);
        }
      }
      assert(follow.settled());
      assert(follow.state().feet[0].position.y == 0 && follow.state().feet[1].position.y == 0);
    }
    // Both authored knees must flex forward in either facing direction.
    // Contact-only checks missed the werewolf's mirrored left knee pole.
    for (int direction : {1, -1}) {
      GroundedController anatomy(d);
      anatomy.reset(0, {-200, 200});
      anatomy.setTargetX(direction * 80);
      for (int frame = 0; frame < 2400 && !anatomy.settled(); ++frame) {
        auto q = anatomy.propose(1. / 120);
        anatomy.realize(anatomy.state().rootX + q.displacement);
      }
      assert(anatomy.settled());
      auto world = worldTransforms(d, anatomy.pose(Action::Rest));
      for (auto leg : d.locomotion->legs) {
        const auto &hip = world[leg.hip], &knee = world[leg.knee],
                   &ankle = world[leg.ankle];
        double pole = (ankle.tx-hip.tx)*(knee.ty-hip.ty) -
                      (ankle.ty-hip.ty)*(knee.tx-hip.tx);
        assert(direction * pole < -1e-4);
      }
    }
    GroundedController actual(d);
    actual.setSpeedMultiplier(3);
    actual.reset(0, {-200, 200});
    actual.setTargetX(100);
    for (int i = 0; i < 4000; i++) {
      if (i == 200)
        actual.setTargetX(-100);
      auto q = actual.propose(1. / 120);
      actual.realize(actual.state().rootX + q.displacement);
      auto p =
          actual.pose(i % 100 < 20 ? Action::Celebrate : Action::Idle, .25);
      auto world = worldTransforms(d, p);
      for (int j = 0; j < 2; j++) {
        auto leg = d.locomotion->legs[j];
        auto contact = world[leg.ankle].apply(leg.contact);
        assert(std::abs(contact.x + actual.state().rootX - d.origin.x -
                        actual.state().feet[j].position.x) < .1);
        assert(std::abs(contact.y - d.locomotion->groundY -
                        actual.state().feet[j].position.y) < .1);
      }
    }
    assert(actual.settled());
  }
  std::cout << "grounded primitives, planted contacts, navigation and "
               "validation passed\n";
}
