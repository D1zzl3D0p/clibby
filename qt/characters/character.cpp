#include "character.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <set>
#include <stdexcept>
#include <utility>
namespace pets::character {
namespace {
constexpr double pi=3.141592653589793; 
void require(bool ok, const char *reason) {
  if (!ok)
    throw std::runtime_error(reason);
}
bool finite(double v) { return std::isfinite(v); }
bool valid(Vec2 p) {
  return finite(p.x) && finite(p.y) && std::abs(p.x) <= 8192 &&
         std::abs(p.y) <= 8192;
}
std::uint32_t expressionCapabilities(const Definition &d) {
  std::uint32_t mask=0;
  for(auto entry: {std::pair<const char *,Role>{"head",Role::Head},
                  {"earLeft",Role::EarLeft},{"earRight",Role::EarRight},
                  {"tail",Role::Tail}})
    if(d.bindings.count(entry.first))mask|=roleBit(entry.second);
  return mask;
}
bool valid(Transform t) {
  return valid(Vec2{t.x, t.y}) && finite(t.rotation) &&
         std::abs(t.rotation) <= 100 && finite(t.scaleX) && finite(t.scaleY) &&
         t.scaleX >= .05 && t.scaleX <= 20 && t.scaleY >= .05 && t.scaleY <= 20;
}
WorldTransform matrix(Transform t) {
  double c = std::cos(t.rotation), s = std::sin(t.rotation);
  return {c * t.scaleX, -s * t.scaleY, s * t.scaleX, c * t.scaleY, t.x, t.y};
}
WorldTransform multiply(WorldTransform a, WorldTransform b) {
  return {
      a.m00 * b.m00 + a.m01 * b.m10,      a.m00 * b.m01 + a.m01 * b.m11,
      a.m10 * b.m00 + a.m11 * b.m10,      a.m10 * b.m01 + a.m11 * b.m11,
      a.m00 * b.tx + a.m01 * b.ty + a.tx, a.m10 * b.tx + a.m11 * b.ty + a.ty};
}
} // namespace
Vec2 WorldTransform::apply(Vec2 p) const {
  return {m00 * p.x + m01 * p.y + tx, m10 * p.x + m11 * p.y + ty};
}
std::vector<WorldTransform> artworkTransforms(const Definition &d,const Pose &p) {
  const auto world=worldTransforms(d,p);
  auto art=world;
  const auto eyes=d.bindings.find("eyes");
  const double openness=std::isfinite(p.eyeOpenness)
      ? std::clamp(p.eyeOpenness,0.,1.) : 1.;
  if(eyes!=d.bindings.end() && openness<1.) {
    const int eyeBone=eyes->second;
    // Scale appearance around the authored eye origin, including pupil art.
    // Compose local descendants directly so edge-on (singular) facing poses
    // remain valid without inverting a world transform.
    const auto eyelid=multiply(world.at(eyeBone),
        WorldTransform{1,0,0,std::max(.05,openness),0,0});
    for(size_t i=0;i<d.bones.size();++i) {
      int ancestor=int(i);
      WorldTransform relative;
      while(ancestor>=0 && ancestor!=eyeBone) {
        relative=multiply(matrix(p.bones.at(ancestor)),relative);
        ancestor=d.bones.at(ancestor).parent;
      }
      if(ancestor==eyeBone)art[i]=multiply(eyelid,relative);
    }
  }
  if(!d.locomotion)return art;
  for(const auto& leg:d.locomotion->legs) {
    for(const auto pair: {std::pair<int,int>{leg.hip,leg.knee},
                         std::pair<int,int>{leg.knee,leg.ankle}}) {
      const auto rest=d.bones.at(pair.second).rest;
      const auto posed=p.bones.at(pair.second);
      const double authored=std::hypot(rest.x,rest.y);
      if(authored<1e-12)continue;
      const double extension=std::max(1.,std::hypot(posed.x,posed.y)/authored)-1;
      const double x=rest.x/authored,y=rest.y/authored;
      // Scale only along the segment axis, without propagating artwork scale
      // to child joints or enlarging the paw's floor-contact offset.
      const WorldTransform stretch{1+extension*x*x,extension*x*y,
                                   extension*x*y,1+extension*y*y,0,0};
      art.at(pair.first)=multiply(world.at(pair.first),stretch);
    }
  }
  return art;
}
void validateDefinition(const Definition &d) {
  require(!d.id.empty() && !d.name.empty(), "character identity required");
  require(d.id.size() <= 64 && std::all_of(d.id.begin(), d.id.end(),
                                           [](unsigned char c) {
                                             return (c >= 'A' && c <= 'Z') ||
                                                    (c >= 'a' && c <= 'z') ||
                                                    (c >= '0' && c <= '9') ||
                                                    c == '_' || c == '-';
                                           }),
          "invalid character ID");
  require(d.controller == "soft" || d.controller == "articulated",
          "unknown controller");
  require(d.width > 0 && d.height > 0 && d.width <= 2048 && d.height <= 2048 &&
              valid(d.origin),
          "invalid character canvas");
  require(finite(d.motionEnvelope) && d.motionEnvelope >= 0 &&
              d.motionEnvelope <= 256,
          "invalid motion envelope");
  require(!d.bones.empty() && d.bones.size() <= 64 && !d.attachments.empty() &&
              d.attachments.size() <= 64,
          "bone/attachment count exceeded");
  std::set<std::string> ids;
  for (auto &b : d.bones) {
    require(!b.id.empty() && ids.insert(b.id).second,
            "duplicate/empty bone ID");
    require(b.parent >= -1 && b.parent < int(d.bones.size()), "invalid parent");
    require(valid(b.rest) && finite(b.minRotation) && finite(b.maxRotation) &&
                b.minRotation <= b.maxRotation && b.minRotation >= -100 &&
                b.maxRotation <= 100 && b.rest.rotation >= b.minRotation &&
                b.rest.rotation <= b.maxRotation,
            "invalid rest transform or joint limits");
  }
  std::vector<int> marks(d.bones.size());
  std::function<void(int)> visit = [&](int i) {
    require(marks[i] != 1, "cyclic hierarchy");
    if (marks[i] == 2)
      return;
    marks[i] = 1;
    if (d.bones[i].parent >= 0)
      visit(d.bones[i].parent);
    marks[i] = 2;
  };
  for (int i = 0; i < int(d.bones.size()); ++i)
    visit(i);
  for (auto &a : d.attachments)
    require(a.bone >= 0 && a.bone < int(d.bones.size()) && a.source.x >= 0 &&
                a.source.y >= 0 && a.source.width > 0 && a.source.height > 0 &&
                a.source.width <= 2048 && a.source.height <= 2048 &&
                valid(a.pivot) && valid(a.offset),
            "invalid attachment");
  for (auto &b : d.bindings)
    require(!b.first.empty() && b.second >= 0 && b.second < int(d.bones.size()),
            "invalid binding");
  require(d.bindings.count("body") != 0, "controller requires body binding");
  int wingBindings=0;
  for(const auto side:{"Left","Right"})
    for(const auto role:{"wingShoulder","wingElbow","wingWrist"})wingBindings+=d.bindings.count(std::string(role)+side);
  require(wingBindings==0||wingBindings==6,"partial wing anatomy");
  if(wingBindings) {
    require(d.locomotion.has_value(),"flight wings require grounded locomotion");
    std::set<int> wingBones;
    for(const auto side:{"Left","Right"}) {
      int shoulder=d.bindings.at(std::string("wingShoulder")+side),elbow=d.bindings.at(std::string("wingElbow")+side),wrist=d.bindings.at(std::string("wingWrist")+side);
      require(wingBones.insert(shoulder).second&&wingBones.insert(elbow).second&&wingBones.insert(wrist).second,"shared wing joints");
      require(d.bones[elbow].parent==shoulder&&d.bones[wrist].parent==elbow,"disconnected wing chain");
      require(d.bones[shoulder].minRotation<=d.bones[shoulder].rest.rotation-(std::string(side)=="Left"?.95:2.20)&&d.bones[shoulder].maxRotation>=d.bones[shoulder].rest.rotation+(std::string(side)=="Left"?2.20:.95)&&
              (std::string(side)=="Left" ? d.bones[elbow].maxRotation>=d.bones[elbow].rest.rotation+pi&&d.bones[wrist].minRotation<=d.bones[wrist].rest.rotation-pi : d.bones[elbow].minRotation<=d.bones[elbow].rest.rotation-pi&&d.bones[wrist].maxRotation>=d.bones[wrist].rest.rotation+pi),"wing limits clip procedural motion");
      for(int joint:{elbow,wrist})require(std::hypot(d.bones[joint].rest.x,d.bones[joint].rest.y)>=.1,"degenerate wing segment");
      int ancestor=shoulder;while(ancestor>=0&&ancestor!=d.bindings.at("body"))ancestor=d.bones[ancestor].parent;
      require(ancestor==d.bindings.at("body"),"wing needs body ancestry");
    }
  }
  require(d.interaction.size() >= 3 && d.interaction.size() <= 128,
          "interaction polygon needs 3..128 points");
  for (auto p : d.interaction)
    require(valid(p) && p.x >= 0 && p.y >= 0 && p.x <= d.width &&
                p.y <= d.height,
            "interaction point outside canvas");
  require(d.contacts.size() <= 64, "too many contacts");
  std::set<int> contacts;
  for (auto c : d.contacts)
    require(c.bone >= 0 && c.bone < int(d.bones.size()) && valid(c.point) &&
                contacts.insert(c.bone).second,
            "invalid/duplicate contact");
  Pose rest;
  for (const auto &bone : d.bones)
    rest.bones.push_back(bone.rest);
  auto restWorld = worldTransforms(d, rest);
  if (d.locomotion) {
    const auto &l = *d.locomotion;
    auto range = [&](double v, double lo, double hi) {
      return finite(v) && v >= lo && v <= hi;
    };
    require(range(l.groundY, 0, d.height) && range(l.speed, .1, 500) &&
                range(l.acceleration, .1, 2000) &&
                range(l.reachThreshold, .1, 128) && range(l.stride, .1, 128) &&
                range(l.lift, 0, 64) && range(l.minStepDuration, .04, 1) &&
                range(l.maxStepDuration, l.minStepDuration, 2),
            "invalid locomotion tuning");
    require(range(l.secondary.frequency, .1, 30) &&
                range(l.secondary.damping, .1, 4) &&
                range(l.secondary.response, -4, 4),
            "invalid secondary tuning");
    std::set<int> used;
    for (auto leg : l.legs) {
      for (int bone : {leg.hip, leg.knee, leg.ankle})
        require(bone >= 0 && bone < int(d.bones.size()) &&
                    used.insert(bone).second,
                "invalid or shared leg bone");
      require(d.bones[leg.knee].parent == leg.hip &&
                  d.bones[leg.ankle].parent == leg.knee,
              "disconnected leg chain");
      require((leg.bend == 1 || leg.bend == -1) && valid(leg.contact),
              "invalid leg bend/contact");
      double length = 0;
      for (int bone : {leg.knee, leg.ankle}) {
        auto t = d.bones[bone].rest;
        double n = std::hypot(t.x, t.y);
        require(n >= .1 && n <= 256, "invalid leg segment length");
        length += n;
      }
      auto upperRest = d.bones[leg.knee].rest,
           lowerRest = d.bones[leg.ankle].rest;
      // Unequal limbs also have an inner unreachable disk. The highest swing
      // must leave its ankle below that fold radius, even directly under the hip.
      double fold = std::abs(std::hypot(upperRest.x, upperRest.y) -
                             std::hypot(lowerRest.x, lowerRest.y));
      require(l.groundY - leg.contact.y - restWorld[leg.hip].ty - l.lift > fold,
              "swing exceeds leg fold radius");
      require(l.reachThreshold < length * .75 && l.stride < length * .9 &&
                  l.lift < length * .75,
              "step tuning exceeds leg reach");
      int ancestor = leg.hip;
      while (ancestor >= 0 && ancestor != d.bindings.at("body"))
        ancestor = d.bones[ancestor].parent;
      require(ancestor == d.bindings.at("body") &&
                  d.bones[ancestor].parent == -1,
              "IK needs root body ancestry");
      for (int bone : {leg.hip, leg.knee, leg.ankle})
        require(d.bones[bone].minRotation <= -3.141592653589793 &&
                    d.bones[bone].maxRotation >= 3.141592653589793,
                "restricted IK joints unsupported");
      for (int bone = leg.ankle; bone >= 0; bone = d.bones[bone].parent) {
        auto facing = d.bindings.find("facing");
        require(facing == d.bindings.end() || facing->second != bone,
                "facing branch cannot contain IK ancestry");
        require(d.bones[bone].rest.scaleX == 1 &&
                    d.bones[bone].rest.scaleY == 1,
                "scaled IK ancestry unsupported");
      }
      auto hip = restWorld[leg.hip];
      auto foot = restWorld[leg.ankle].apply(leg.contact);
      require(std::abs(foot.y - l.groundY) <= d.motionEnvelope &&
                  l.groundY - hip.ty > 0 &&
                  l.groundY - hip.ty <=
                      length + std::hypot(leg.contact.x, leg.contact.y),
              "invalid ground anchor for chain");
    }
  }
}
std::vector<WorldTransform> worldTransforms(const Definition &d,
                                            const Pose &p) {
  require(p.bones.size() == d.bones.size() && valid(p.root),
          "invalid pose size/root");
  std::vector<WorldTransform> world(p.bones.size());
  std::vector<int> marks(p.bones.size());
  std::function<void(int)> visit = [&](int i) {
    require(marks[i] != 1, "cyclic pose hierarchy");
    if (marks[i] == 2)
      return;
    marks[i] = 1;
    auto checked = p.bones[i];
    auto facing = d.bindings.find("facing");
    if (facing != d.bindings.end() && facing->second == i) {
      require(finite(checked.scaleX) && std::abs(checked.scaleX) <= 1,
              "invalid facing scale");
      checked.scaleX = 1; // Only this visual branch supports reflection/collapse.
    }
    require(valid(checked), "invalid pose transform");
    int parent = d.bones[i].parent;
    require(parent >= -1 && parent < int(p.bones.size()),
            "invalid pose parent");
    if (parent >= 0) {
      visit(parent);
      world[i] = multiply(world[parent], matrix(p.bones[i]));
    } else {
      world[i] = matrix(p.bones[i]);
      world[i].tx += p.root.x;
      world[i].ty += p.root.y;
    }
    auto supported = [](double component) {
      return finite(component) && std::abs(component) <= 1e8;
    };
    require(supported(world[i].tx) && supported(world[i].ty) &&
                supported(world[i].m00) && supported(world[i].m01) &&
                supported(world[i].m10) && supported(world[i].m11),
            "world transform exceeds supported range");
    marks[i] = 2;
  };
  for (int i = 0; i < int(p.bones.size()); ++i)
    visit(i);
  return world;
}
Controller::Controller(Definition d) : definition_(std::move(d)), expressions_(expressionCapabilities(definition_)) {
  validateDefinition(definition_);
}
bool Controller::settled() const {
  return displacement_.x == 0 && displacement_.y == 0 && velocity_.x == 0 &&
         velocity_.y == 0 && expressions_.settled();
}
void Controller::setAction(Action action,double time) {
  require(finite(time),"invalid action time");
  procedural_=true;
  if((action==Action::Celebrate||action==Action::Failure||action==Action::Acknowledge)&&time>=1)
    action=Action::Rest;
  if(action==action_)return;
  for(unsigned i=0;i<static_cast<unsigned>(Role::Count);++i)
    if(reactionRoles_&roleBit(static_cast<Role>(i)))expressions_.clear(static_cast<Role>(i));
  reactionRoles_=0; action_=action;
  auto goal=[&](Role role,pets::motion::Vec3 target) {
    if(expressions_.goal(role,target))reactionRoles_|=roleBit(role);
  };
  switch(action) {
  case Action::Attend: goal(Role::Head,{-.18,0,.08}); break;
  case Action::Celebrate:
    goal(Role::Head,{-.22,0,-.12}); goal(Role::Tail,{0,0,.5});
    goal(Role::EarLeft,{0,0,-.25}); goal(Role::EarRight,{0,0,.25}); break;
  case Action::Failure:
    goal(Role::Head,{.2,0,.12}); goal(Role::EarLeft,{0,0,.2});
    goal(Role::EarRight,{0,0,-.2}); break;
  case Action::Acknowledge: goal(Role::Head,{.2,0,0}); break;
  default: break;
  }
}
void Controller::resetDynamics() {
  displacement_ = {};
  velocity_ = {};
  expressions_.reset(); action_=Action::Rest; reactionRoles_=0;
}
void Controller::impulse(Vec2 direction, double magnitude) {
  require(valid(direction) && finite(magnitude), "nonfinite impulse");
  double n = std::hypot(direction.x, direction.y);
  if (n == 0 || magnitude == 0)
    return;
  magnitude = std::clamp(magnitude, -4.0, 4.0);
  velocity_.x =
      std::clamp(velocity_.x + direction.x / n * magnitude * 3, -16.0, 16.0);
  velocity_.y =
      std::clamp(velocity_.y + direction.y / n * magnitude * 3, -16.0, 16.0);
}
void Controller::step(double dt) {
  require(finite(dt) && dt >= 0, "invalid dynamics delta");
  if (settled() || dt == 0)
    return;
  expressions_.step(dt);
  dt = std::min(dt, 4.0 / 120);
  int steps = std::min(4, std::max(1, int(std::ceil(dt * 120))));
  double h = dt / steps;
  for (int i = 0; i < steps; ++i) {
    auto advance = [&](double &x, double &v) {
      v = std::clamp(v + (-100 * x - 20 * v) * h, -16.0, 16.0);
      x = std::clamp(x + v * h, -1.0, 1.0);
    };
    advance(displacement_.x, velocity_.x);
    advance(displacement_.y, velocity_.y);
  }
  if (std::hypot(displacement_.x, displacement_.y) < .0005 &&
      std::hypot(velocity_.x, velocity_.y) < .002)
    { displacement_={}; velocity_={}; }
}
Pose Controller::pose(Action action, double time, Vec2 gaze) const {
  require(finite(time) && valid(gaze), "invalid action time/gaze");
  time = std::fmod(time, 3600.0);
  Pose p;
  for (auto b : definition_.bones)
    p.bones.push_back(b.rest);
  auto bound = [&](const std::string &name) -> Transform * {
    auto it = definition_.bindings.find(name);
    return it == definition_.bindings.end() ? nullptr : &p.bones[it->second];
  };
  Transform *body = bound("body");
  double softness = definition_.controller == "soft" ? 1 : .45;
  double breath =
      procedural_ || action == Action::Rest ? 0 : std::sin(time * 2.3) * .015 * softness;
  body->scaleY *= 1 + breath;
  body->rotation += displacement_.x * .18;
  if (auto head = bound("head")) {
    head->rotation +=
        (procedural_ ? 0 : std::clamp(gaze.x / 180.0, -.25, .25)) + displacement_.x * .22;
    head->y += displacement_.y * 2;
  }
  for (auto name : {"earLeft", "earRight", "tail"})
    if (auto bone = bound(name))
      bone->rotation +=
          displacement_.x * (std::string(name) == "tail" ? .5 : -.35);
  if (!procedural_ && action == Action::Walk) {
    p.desiredRootMotion = {18, 0};
    body->y -= std::abs(std::sin(time * 7)) * .8;
    for (auto name : {"footLeft", "footRight"})
      if (auto foot = bound(name)) {
        double phase =
            time * 7 +
            (std::string(name) == "footRight" ? 3.141592653589793 : 0);
        foot->x += std::sin(phase) * 3;
        foot->y -= std::max(0.0, std::cos(phase)) * 3;
      }
  }
  if (!procedural_ && action == Action::Attend)
    body->rotation += .045;
  double reaction = std::sin(std::clamp(time, 0.0, 1.0) * 3.141592653589793);
  if (!procedural_ && action == Action::Celebrate) {
    body->y -= std::abs(std::sin(time * 6)) * 3 * reaction;
    body->rotation += std::sin(time * 6) * .07 * reaction;
  }
  if (!procedural_ && action == Action::Failure) {
    body->scaleY *= 1 - .04 * softness * reaction;
    if (auto head = bound("head"))
      head->rotation += .12 * reaction;
  }
  if (!procedural_ && action == Action::Acknowledge)
    if (auto head = bound("head"))
      head->rotation += std::sin(time * 8) * .12 * reaction;
  for(auto entry: {std::pair<const char *,Role>{"head",Role::Head},
                  {"earLeft",Role::EarLeft},{"earRight",Role::EarRight},
                  {"tail",Role::Tail}}) {
    if(auto bone=bound(entry.first)) {
      const auto v=expressions_.value(entry.second);
      // A planar head nods for pitch and tips gently toward lateral gaze.
      // The canvas cannot represent a spatial yaw, so use a bounded tilt.
      bone->rotation+=v.z+(entry.second==Role::Head ? v.x+v.y*.5 : 0);
    }
  }
  for (size_t i = 0; i < p.bones.size(); ++i) {
    p.bones[i].x = std::clamp(p.bones[i].x, -8192.0, 8192.0);
    p.bones[i].y = std::clamp(p.bones[i].y, -8192.0, 8192.0);
    p.bones[i].rotation =
        std::clamp(p.bones[i].rotation, definition_.bones[i].minRotation,
                   definition_.bones[i].maxRotation);
    p.bones[i].scaleX = std::clamp(p.bones[i].scaleX, .05, 20.0);
    p.bones[i].scaleY = std::clamp(p.bones[i].scaleY, .05, 20.0);
  }
  // Contacts are solved in hierarchy order. Horizontal planted position is also
  // restored for non-walking poses; walking permits stride but never below
  // ground.
  Pose rest;
  for (auto b : definition_.bones)
    rest.bones.push_back(b.rest);
  auto baseline = worldTransforms(definition_, rest);
  auto contacts = definition_.contacts;
  auto depth = [&](int i) {
    int n = 0;
    while (definition_.bones[i].parent >= 0) {
      i = definition_.bones[i].parent;
      ++n;
    }
    return n;
  };
  std::stable_sort(contacts.begin(), contacts.end(), [&](Contact a, Contact b) {
    return depth(a.bone) < depth(b.bone);
  });
  try {
    worldTransforms(definition_, p);
    for (auto c : contacts) {
      auto world = worldTransforms(definition_, p);
      auto wanted = baseline[c.bone].apply(c.point);
      auto current = world[c.bone].apply(c.point);
      Vec2 correction{action == Action::Walk ? 0 : wanted.x - current.x,
                      action == Action::Walk
                          ? std::min(0.0, wanted.y - current.y)
                          : wanted.y - current.y};
      int parent = definition_.bones[c.bone].parent;
      if (parent >= 0) {
        auto m = world[parent];
        double det = m.m00 * m.m11 - m.m01 * m.m10;
        if (!finite(det) || std::abs(det) <= 1e-12)
          return rest;
        correction = {(m.m11 * correction.x - m.m01 * correction.y) / det,
                      (-m.m10 * correction.x + m.m00 * correction.y) / det};
      }
      auto &local = p.bones[c.bone];
      Transform candidate = local;
      candidate.x += correction.x;
      candidate.y += correction.y;
      if (!valid(candidate))
        return rest;
      local = candidate;
    }
    worldTransforms(definition_, p);
  } catch (const std::runtime_error &) {
    // Definitions already passed the complete rest-hierarchy validation.
    // A permitted action can cross the numeric limit; publish safe rest
    // instead of passing unsafe float matrices to the scene graph.
    return rest;
  }
  return p;
}
} // namespace pets::character
