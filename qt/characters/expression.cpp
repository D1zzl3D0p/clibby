#include "expression.h"
#include "second_order.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
namespace pets::character {
namespace {
using pets::motion::Vec3;
constexpr unsigned count=static_cast<unsigned>(Role::Count);
bool finite(Vec3 v) { return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z); }
double limit(Role role) {
  switch(role) {
  case Role::Head: return .65;
  case Role::EarLeft: case Role::EarRight: return .6;
  case Role::Tail: return .9;
  case Role::WristLeft: case Role::WristRight: return 3.141592653589793;
  default: return 0;
  }
}
Vec3 bounded(Vec3 v,double bound) {
  return {std::clamp(v.x,-bound,bound),std::clamp(v.y,-bound,bound),std::clamp(v.z,-bound,bound)};
}
}
struct ExpressionMotion::State {
  std::uint32_t capabilities=0;
  std::array<Vec3,count> goals{};
  Vec3 gaze{};
  std::array<std::array<SecondOrderTracker,3>,count> channels;
  explicit State(std::uint32_t mask):capabilities(mask) {
    for(auto &role:channels)for(auto &axis:role)axis.setTuning({2.8,.85,0});
  }
  Vec3 target(unsigned i) const {
    auto g=goals[i];
    if(i==static_cast<unsigned>(Role::Head)) { g.x+=gaze.x; g.y+=gaze.y; }
    return bounded(g,limit(static_cast<Role>(i)));
  }
};
ExpressionMotion::ExpressionMotion(std::uint32_t mask):state_(std::make_unique<State>(mask)) {}
ExpressionMotion::~ExpressionMotion()=default;
ExpressionMotion::ExpressionMotion(const ExpressionMotion &other):state_(std::make_unique<State>(*other.state_)) {}
ExpressionMotion &ExpressionMotion::operator=(const ExpressionMotion &other) {
  if(this!=&other)state_=std::make_unique<State>(*other.state_);
  return *this;
}
ExpressionMotion::ExpressionMotion(ExpressionMotion &&) noexcept=default;
ExpressionMotion &ExpressionMotion::operator=(ExpressionMotion &&) noexcept=default;
bool ExpressionMotion::supports(Role role) const { return (state_->capabilities&roleBit(role))!=0; }
bool ExpressionMotion::goal(Role role,Vec3 g) {
  if(!supports(role)||!finite(g))return false;
  state_->goals[static_cast<unsigned>(role)]=bounded(g,limit(role));
  return true;
}
bool ExpressionMotion::impulse(Role role,Vec3 v) {
  if(!supports(role)||!finite(v))return false;
  v=bounded(v,4);
  auto &c=state_->channels[static_cast<unsigned>(role)];
  c[0].impulse(v.x); c[1].impulse(v.y); c[2].impulse(v.z);
  return true;
}
bool ExpressionMotion::look(Vec3 direction) {
  if(!supports(Role::Head)||!finite(direction))return false;
  const double scale=std::max({std::abs(direction.x),std::abs(direction.y),std::abs(direction.z)});
  if(scale==0)return false;
  direction={direction.x/scale,direction.y/scale,direction.z/scale};
  state_->gaze=bounded({-std::atan2(direction.y,std::hypot(direction.x,direction.z)),
                        std::atan2(direction.x,direction.z),0},limit(Role::Head));
  return true;
}
bool ExpressionMotion::clear(Role role) {
  if(!supports(role))return false;
  state_->goals[static_cast<unsigned>(role)]={};
  if(role==Role::Head)state_->gaze={};
  return true;
}
void ExpressionMotion::clear() { state_->goals={}; state_->gaze={}; }
void ExpressionMotion::reset() {
  clear();
  for(auto &role:state_->channels)for(auto &axis:role)axis.reset();
}
void ExpressionMotion::step(double dt) {
  if(!std::isfinite(dt)||dt<0)throw std::runtime_error("invalid expression delta");
  for(unsigned i=0;i<count;++i) {
    auto target=state_->target(i); auto &c=state_->channels[i];
    c[0].step(dt,target.x); c[1].step(dt,target.y); c[2].step(dt,target.z);
  }
}
bool ExpressionMotion::settled() const {
  for(unsigned i=0;i<count;++i) {
    auto t=state_->target(i); const auto &c=state_->channels[i];
    if(!c[0].settled(t.x)||!c[1].settled(t.y)||!c[2].settled(t.z))return false;
  }
  return true;
}
Vec3 ExpressionMotion::value(Role role) const {
  if(!supports(role))return {};
  const auto &c=state_->channels[static_cast<unsigned>(role)];
  return bounded({c[0].value(),c[1].value(),c[2].value()},limit(role));
}
} // namespace pets::character
