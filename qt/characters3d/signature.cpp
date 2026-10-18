#include "signature.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

namespace pets::character3d {
namespace {
using Vec3=pets::motion::Vec3;
constexpr double pi=3.14159265358979323846;
constexpr double cancelDuration=.35;
struct Channel {
 const char* joint;
 Vec3 rotation{}, oscillation{}, translation{};
 double frequency=0, phase=0;
 // Optional activity window; zero end retains the full profile lifetime.
 double begin=0, end=0;
 SignatureRotationSpace rotationSpace=SignatureRotationSpace::Model;
};
struct Profile {
 const char* asset;
 const char* name;
 double duration;
 std::vector<Channel> channels;
};
// Authored intent amplitudes, independent of renderer, host, timers and limbs.
// Continuous envelopes shape bounded movement; no pose keys or clip playback.
const std::array<Profile,26> profiles{{
 {"marmalade","Tail-tip inspection",5.2,{
  {"head",{-.12,-.28,.06}},
  {"tail_base",{0,.32,0},{0,.035,0},{},.6},
  {"tail_tip",{-.12,.48,0},{0,.10,0},{},.75}}},
 {"pip","Perk and wag",4.5,{
  {"head",{-.10,.07,-.16}},
  {"ear_left",{-.20,0,-.08}},{"ear_right",{-.20,0,.08}},
  {"tail_base",{-.13,0,0},{0,.30,0},{},1.8},
  {"tail_tip",{-.08,0,0},{0,.20,0},{},1.8,-.5}}},
 {"biscuit","Unhurried nose survey",5.8,{
  {"head",{-.10,0,.03},{0,.20,0},{},.21},
  {"ear_left",{0,0,-.10}},{"ear_right",{0,0,.10}}}},
 {"clover","Ear settle and nose dip",4.8,{
  {"head",{.17,.08,0}},
  {"ear_left",{.15,0,-.16},{.035,0,0},{},.8},
  {"ear_right",{.12,0,.13},{.025,0,0},{},.8,-.6}}},
 {"mossback","Shell retreat",5.6,{
  {"head",{.10,0,0},{},{0,0,-.15}}}},
 {"rivet","Antenna check and salute",4.6,{
  {"head",{-.06,0,-.06}},
  {"antenna",{0,0,.30},{0,0,.015},{},1.1},
  {"signature_arm_left",{-1.7,0,-.25}}}},
 {"paper-heron","Paper bow",5.2,{
  {"head",{.30,0,0}},
  {"wing_fold_left",{0,.12,-.20}},
  {"wing_fold_right",{0,-.12,.20}}}},
 {"acorn-custodian","Cap adjustment",4.2,{
  {"head",{.06,0,.08}},
  {"cap",{0,.08,-.13},{0,0,.025},{0,.025,0},.8}}},
 {"puddle","Wing tuck",4.4,{
  {"head",{.16,.10,0}},
  {"wing_fold_left",{0,.17,-.19}},
  {"wing_fold_right",{0,-.17,.19}}}},
 {"pocket-observatory","Lens survey",5.8,{
  {"head",{-.16,0,-.04},{0,.27,0},{},.20}}},
 {"night-librarian","Greeting spread",5.6,{
  {"cover_front",{0,-.38,0}},
  {"cover_back",{0,.38,0}},
  {"bookmark",{.12,0,.08},{.06,0,.03},{},.65}}},
 {"sprig","Leaf presentation",4.8,{
  {"head",{0,.08,-.04}},
  {"leaf_present",{-.30,.15,-.20},{.035,0,.025},{},.6}}},
 {"hearthling","Ember-tail curl",5.2,{
  {"head",{-.06,-.12,0}},
  {"tail_base",{.15,.43,0}},
  {"tail_tip",{.22,.58,.04},{0,.04,0},{},.65}}},
 {"crumb-golem","Careful balancing gesture",4.8,{
  {"head",{.10,0,-.10},{0,0,.025},{},.7},
  {"signature_arm_left",{0,0,-.72},{0,0,.045},{},.7}}},
 {"nell","A passage to consider",6.2,{
  {"head",{.40,-.08,.035}},
  {"upper_arm_left",{-.55,0,-.035}},
  {"forearm_left",{-.80,0,0}},
  {"held_prop",{-.08,.06,0}},
  {"prop_hinge",{0,-.72,0}}}},
 {"alden","Fastening check",5.8,{
  {"head",{.20,-.08,.025}},
  {"upper_arm_right",{-.50,0,1.00}},
  {"forearm_right",{-.15,0,.30}},
  {"fastening",{0,0,.07}}}},
 {"saffron","Vial inspection",6.0,{
  {"head",{.40,.14,-.035}},
  {"upper_arm_left",{-.70,0,.035}},
  {"forearm_left",{-1.00,0,.035}},
  {"held_prop",{.12,0,-.16},{0,0,.035},{},.35}}},
 {"tess","Mechanism inspection",5.6,{
  {"head",{.35,.09,-.04}},
  {"upper_arm_left",{-.45,0,.04}},
  {"forearm_left",{-.70,0,0}},
  {"held_prop",{0,.48,.12},{0,.11,0},{},.45}}},
 {"rowan","Seed study",6.4,{
  {"head",{.45,.12,.035}},
  {"upper_arm_left",{-.60,0,.025}},
  {"forearm_left",{-.95,0,0}},
  {"held_prop",{0,.24,-.08},{0,.07,0},{},.28}}},
 {"vesper","Pocket-watch inspection",6.3,{
  {"head",{.40,.10,-.025}},
  {"upper_arm_left",{-.50,0,.025}},
  {"forearm_left",{-.85,0,0}},
  {"held_prop",{-.08,.12,0}},
  {"prop_hinge",{0,-.88,0}}}},
 {"iona","Map route study",6.4,{
  {"head",{.36,.12,-.035}},
  {"upper_arm_left",{-.64,0,-.22}},
  {"forearm_left",{-.92,0,0}},
  {"hand_left",{-.08,.10,0}},
  {"held_prop",{-.10,.08,0}},
  {"prop_hinge",{0,-.95,0},{},{},0,0,0,0,SignatureRotationSpace::JointLocal},
  {"upper_arm_right",{-.70,0,1.12},{0,0,.06},{},.40,0,1.1},
  {"forearm_right",{-.82,0,.12},{0,.13,0},{},.40,0,1.1},
  {"hand_right",{-.10,.12,0},{0,0,.10},{},.40,0,1.1}}},
 {"milo","Instrument tune and rhythm",6.2,{
  {"head",{.32,.14,-.06},{0,0,.025},{},.6},
  {"upper_arm_left",{-.60,0,-.045}},
  {"forearm_left",{-.92,0,.025}},
  {"hand_left",{-.06,.12,0}},
  {"held_prop",{0,.22,-.12}},
  {"upper_arm_right",{-.62,0,.45},{.035,0,0},{},1.1,0,.5},
  {"forearm_right",{-.82,0,-.08},{0,.14,0},{},.55,0,.5},
  {"hand_right",{-.08,.13,0},{.12,0,.06},{},1.1,0,.5}}},
 {"orris","Clock winding and spectacles check",6.4,{
  {"head",{.30,.13,-.035}},
  {"upper_arm_left",{-.66,0,-.025}},
  {"forearm_left",{-.90,0,0}},
  {"hand_left",{-.08,.10,0}},
  {"held_prop",{-.10,.14,0}},
  {"prop_hinge",{0,.28,0},{0,.34,0},{},.85,0,.35,4.4,SignatureRotationSpace::JointLocal},
  {"upper_arm_right",{-.85,0,1.03},{},{},0,0,2.5},
  {"forearm_right",{-.20,.30,.10},{},{},0,0,2.5},
  {"hand_right",{-.14,.16,0},{0,.05,0},{},.7,0,2.5}}},
 {"fen","Lantern shutter adjustment",6.0,{
  {"head",{.36,.16,-.04}},
  {"upper_arm_left",{-.64,0,-.035}},
  {"forearm_left",{-.94,0,0}},
  {"hand_left",{-.08,.12,0}},
  {"held_prop",{-.08,.12,-.06}},
  {"prop_hinge",{0,-.92,0},{0,.12,0},{},.42,0,.5,0,SignatureRotationSpace::JointLocal},
  {"upper_arm_right",{-.64,0,.40},{},{},0,0,.8},
  {"forearm_right",{-.86,0,-.12},{0,.09,0},{},.42,0,.8},
  {"hand_right",{-.10,.15,0},{0,.10,0},{},.42,0,.8}}},
 {"oona","Mineral inspection",6.3,{
  {"head",{.36,.15,-.025}},
  {"upper_arm_left",{-.70,0,-.025}},
  {"forearm_left",{-.98,0,0}},
  {"hand_left",{-.08,.12,0},{0,.10,0},{},.32},
  {"held_prop",{-.08,.28,-.12},{0,.16,.04},{},.32},
  {"upper_arm_right",{-.48,0,.42},{},{},0,0,1.0},
  {"forearm_right",{-.70,0,-.10},{},{},0,0,1.0},
  {"hand_right",{-.08,.08,0},{0,.06,0},{},.32,0,1.0}}},
 {"quillan","Hand study and tentative greeting",6.4,{
  {"head",{.34,.14,-.045},{0,-.09,0},{},.25},
  {"upper_arm_left",{-.68,0,-.04},{},{},0,0,0,4.6},
  {"forearm_left",{-.96,0,0},{},{},0,0,0,4.6},
  {"hand_left",{-.10,.26,0},{0,.16,0},{},.38,0,0,4.6},
  {"upper_arm_right",{-.90,0,.36},{},{},0,0,2.6},
  {"forearm_right",{-.82,0,.08},{},{},0,0,2.6},
  {"hand_right",{-.08,.18,0},{0,0,.19},{},.85,0,2.6},
  {"ear_fin_left",{.08,0,-.15},{0,0,.04},{},.55,0,2.9},
  {"ear_fin_right",{.08,0,.15},{0,0,-.04},{},.55,0,2.9}}}
}};
Vec3 add(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec3 scale(Vec3 a,double f){return {a.x*f,a.y*f,a.z*f};}
double smooth(double t){t=std::clamp(t,0.,1.);return t*t*(3-2*t);}
double envelope(double t,double duration){
 if(t<=0||t>=duration)return 0;
 return smooth(t/.95)*smooth((duration-t)/1.15);
}
}
SignatureMotion::SignatureMotion(std::string_view asset){
 for(size_t i=0;i<profiles.size();++i)if(asset==profiles[i].asset){profile_=static_cast<int>(i);break;}
}
bool SignatureMotion::supported() const{return profile_>=0;}
std::string_view SignatureMotion::name() const{return supported()?profiles[profile_].name:std::string_view{};}
std::vector<std::string_view> SignatureMotion::requiredJoints() const{
 std::vector<std::string_view> result;
 if(supported())for(const auto& channel:profiles[profile_].channels)result.emplace_back(channel.joint);
 return result;
}
bool SignatureMotion::start(){
 if(!supported()||active_)return false;
 reset();active_=true;return true;
}
void SignatureMotion::cancel(){
 if(!active_||cancelling_)return;
 cancelPose_=evaluate(elapsed_);cancelVelocity_=cancelPose_;
 constexpr double h=1e-5;
 const auto before=evaluate(elapsed_-h),after=evaluate(elapsed_+h);
 for(size_t i=0;i<cancelVelocity_.size();++i){
  cancelVelocity_[i].rotation=scale(add(after[i].rotation,scale(before[i].rotation,-1)),.5/h);
  cancelVelocity_[i].translation=scale(add(after[i].translation,scale(before[i].translation,-1)),.5/h);
 }
 cancelling_=true;cancelElapsed_=0;
}
void SignatureMotion::reset(){
 elapsed_=cancelElapsed_=0;active_=cancelling_=false;
 cancelPose_.clear();cancelVelocity_.clear();
}
void SignatureMotion::advance(double dt){
 if(!std::isfinite(dt)||dt<0)throw std::invalid_argument("Signature interval must be finite and nonnegative");
 if(!active_||dt==0)return;
 // Compare remaining time before adding, keeping even enormous valid dt safe.
 if(cancelling_){if(dt>=cancelDuration-cancelElapsed_){reset();return;}cancelElapsed_+=dt;}
 else {if(dt>=profiles[profile_].duration-elapsed_){reset();return;}elapsed_+=dt;}
}
bool SignatureMotion::active() const{return active_;}
std::vector<SignatureOffset> SignatureMotion::evaluate(double elapsed) const{
 std::vector<SignatureOffset> result;
 if(!supported())return result;
 const auto& p=profiles[profile_];
 result.reserve(p.channels.size());
 for(const auto& c:p.channels){
  const double end=c.end>0?c.end:p.duration;
  const double e=envelope(elapsed-c.begin,end-c.begin);
  const double wave=std::sin(2*pi*c.frequency*elapsed+c.phase);
  result.push_back({c.joint,scale(add(c.rotation,scale(c.oscillation,wave)),e),scale(c.translation,e),c.rotationSpace});
 }
 return result;
}
std::vector<SignatureOffset> SignatureMotion::offsets() const{
 if(!active_)return evaluate(0);
 if(!cancelling_)return evaluate(elapsed_);
 auto result=cancelPose_;const double u=cancelElapsed_/cancelDuration;
 const double position=2*u*u*u-3*u*u+1,velocity=(u*u*u-2*u*u+u)*cancelDuration;
 for(size_t i=0;i<result.size();++i){
  result[i].rotation=add(scale(cancelPose_[i].rotation,position),scale(cancelVelocity_[i].rotation,velocity));
  result[i].translation=add(scale(cancelPose_[i].translation,position),scale(cancelVelocity_[i].translation,velocity));
 }
 return result;
}
}
