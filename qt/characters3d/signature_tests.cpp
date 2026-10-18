#include "signature.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace pets::character3d;
namespace {
constexpr std::array<const char*,26> assets{{"marmalade","pip","biscuit","clover","mossback","rivet","paper-heron","acorn-custodian","puddle","pocket-observatory","night-librarian","sprig","hearthling","crumb-golem","nell","alden","saffron","tess","rowan","vesper","iona","milo","orris","fen","oona","quillan"}};
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
double norm(pets::motion::Vec3 v){return std::hypot(std::hypot(v.x,v.y),v.z);}
double delta(const std::vector<SignatureOffset>& a,const std::vector<SignatureOffset>& b){
 check(a.size()==b.size(),"signature channels change over time");double result=0;
 for(size_t i=0;i<a.size();++i){check(a[i].joint==b[i].joint,"signature channel order changes");
  check(a[i].rotationSpace==b[i].rotationSpace,"signature rotation space changes over time or cancellation");
  const auto difference=[](auto x,auto y){return pets::motion::Vec3{x.x-y.x,x.y-y.y,x.z-y.z};};
  result=std::max(result,norm(difference(a[i].rotation,b[i].rotation)));
  result=std::max(result,norm(difference(a[i].translation,b[i].translation)));}
 return result;
}
void zero(const std::vector<SignatureOffset>& a){for(const auto& o:a)check(norm(o.rotation)==0&&norm(o.translation)==0,"signature fails exact rest");}
void allProfiles(){for(const char* asset:assets){
 SignatureMotion s(asset);check(s.supported()&&!s.name().empty(),"missing supported signature");
 const auto rest=s.offsets();zero(rest);check(!rest.empty(),"signature has no channels");
 const auto joints=s.requiredJoints();check(joints.size()==rest.size(),"required joint mismatch");
 for(size_t i=0;i<rest.size();++i){check(joints[i]==rest[i].joint,"required joint order mismatch");
  const auto& j=rest[i].joint;check(j!="root"&&j!="pelvis"&&j!="spine"&&j.find("thigh")==std::string::npos&&j.find("shin")==std::string::npos&&j.find("foot")==std::string::npos,"signature alters support chain");}
 check(s.start()&&!s.start(),"repeated start restarts active action");zero(s.offsets());
 s.advance(1e-5);check(delta(rest,s.offsets())<1e-8,"nonzero start derivative");
 s.reset();check(s.start(),"reset prevents restart");double peak=0;
 for(int frame=0;frame<900;++frame){s.advance(1./120);const auto p=s.offsets();peak=std::max(peak,delta(rest,p));
  for(const auto& o:p){check(std::isfinite(norm(o.rotation))&&std::isfinite(norm(o.translation)),"nonfinite offset");check(norm(o.rotation)<1.75&&norm(o.translation)<=.151,"unbounded signature");}}
 check(peak>.03,"signature does not move");check(!s.active(),"signature never finishes");zero(s.offsets());
 s.advance(1000);zero(s.offsets());
 s.start();s.advance(2.1);const auto before=s.offsets();s.advance(0);check(delta(before,s.offsets())==0,"zero dt changes action");
 for(double invalid:{-1.,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}){
  bool rejected=false;try{s.advance(invalid);}catch(const std::invalid_argument&){rejected=true;}
  check(rejected&&delta(before,s.offsets())==0&&s.active(),"invalid dt mutates action");}
 }}
void cancellation(){for(const char* asset:assets)for(double time:{.001,.7,2.1,4.8}){
 SignatureMotion s(asset);s.start();s.advance(time);if(!s.active())continue;
 const auto before=s.offsets();s.cancel();check(delta(before,s.offsets())<1e-12,"cancel snaps pose");
 s.cancel();check(delta(before,s.offsets())<1e-12,"repeated cancel snaps pose");
 for(int i=0;i<34;++i){s.advance(.01);for(const auto& o:s.offsets())check(std::isfinite(norm(o.rotation))&&norm(o.rotation)<1.75&&norm(o.translation)<=.151,"cancel escapes motion bounds");}
 const auto fading=s.offsets();bool rejected=false;try{s.advance(-.1);}catch(const std::invalid_argument&){rejected=true;}
 check(rejected&&delta(fading,s.offsets())==0,"invalid dt mutates cancellation");
 s.advance(.009);check(s.active(),"cancel completes too early");s.advance(.0011);
 check(!s.active(),"cancel exceeds bounded duration");zero(s.offsets());check(s.start(),"cancel prevents restart");
 s.reset();check(!s.active(),"reset remains active");zero(s.offsets());
 }}
void continuity(){constexpr double h=1e-5;for(const char* asset:assets)
 for(double time:{.35,.5,.8,1.,1.1,1.8,2.5,2.6,2.9,4.4,4.6,6.2,6.3,6.4}){
 SignatureMotion a(asset),b(asset),c(asset);a.start();b.start();c.start();
 a.advance(time-h);b.advance(time);c.advance(time+h);
 auto previous=a.offsets(),at=b.offsets(),next=c.offsets();b.cancel();b.advance(h);
 const auto cancelled=b.offsets();
 // Discrete velocity across cancellation must match continued action locally.
 check(delta(next,cancelled)<2e-8,"cancellation velocity discontinuity");
 check(delta(previous,at)<.0001,"signature internal discontinuity");
 b.advance(.35-h-1e-5);auto nearEnd=b.offsets();b.advance(1e-5);zero(b.offsets());
 check(delta(nearEnd,b.offsets())<1e-8,"nonzero cancellation end derivative");
 }}
void absentAnatomy(){for(const char* asset:{"biscuit","mossback"}){SignatureMotion s(asset);for(auto j:s.requiredJoints())check(j.find("tail")==std::string_view::npos,"missing tail referenced");}
 SignatureMotion clover("clover");for(auto j:clover.requiredJoints())check(j!="tail_tip","missing cotton tail tip referenced");
 SignatureMotion unknown("../../pip");check(!unknown.supported()&&!unknown.start()&&unknown.offsets().empty()&&unknown.requiredJoints().empty()&&unknown.name().empty(),"unknown asset enabled");
 unknown.cancel();unknown.reset();unknown.advance(20);check(!unknown.active(),"unknown action active");
}
void timeAndReadPurity(){for(const char* asset:assets){
 SignatureMotion one(asset),many(asset);one.start();many.start();one.advance(2.25);
 for(int i=0;i<225;++i)many.advance(.01);
 check(delta(one.offsets(),many.offsets())<1e-11,"frame partition changes signature");
 const auto snapshot=one.offsets();for(int i=0;i<30;++i){one.name();one.requiredJoints();one.supported();one.active();check(delta(snapshot,one.offsets())==0,"signature read advances time");}
 one.advance(std::numeric_limits<double>::max());check(!one.active(),"large finite interval overflows lifetime");zero(one.offsets());
 }
 // The last samples on either side of the finite action boundary approach zero
 // velocity as well as zero position; the caller may stop ticking immediately.
 for(double duration:{4.5}){SignatureMotion s("pip");s.start();s.advance(duration-2e-5);
 const auto before=s.offsets();s.advance(1e-5);const auto last=s.offsets();s.advance(2e-5);
 check(delta(before,last)<1e-8&&delta(last,s.offsets())<1e-8,"natural end derivative discontinuity");zero(s.offsets());}
}
// Solve the authored shoulder/elbow/wrist geometry, independently of the
// timeline, to catch gestures that merely extend a waist-height prop forward.
void humanoidInspectionHeight(){
 using V=pets::motion::Vec3;
 auto rotate=[](V v,V r){
  const V z{v.x*std::cos(r.z)-v.y*std::sin(r.z),v.x*std::sin(r.z)+v.y*std::cos(r.z),v.z};
  const V x{z.x,z.y*std::cos(r.x)-z.z*std::sin(r.x),z.y*std::sin(r.x)+z.z*std::cos(r.x)};
  return V{x.x*std::cos(r.y)+x.z*std::sin(r.y),x.y,-x.x*std::sin(r.y)+x.z*std::cos(r.y)};
 };
 const std::array<std::pair<const char*,double>,5> bodies{{{"nell",.34},{"saffron",.275},{"tess",.365},{"rowan",.26},{"vesper",.26}}};
 for(const auto& body:bodies){
  SignatureMotion action(body.first);action.start();action.advance(2.);
  V upper{},fore{};
  for(const auto& o:action.offsets()){
   if(o.joint=="upper_arm_left")upper=o.rotation;
   if(o.joint=="forearm_left")fore=o.rotation;
  }
  // These are actual glTF rest-chain displacements: the elbow drops .33 m,
  // the forearm drops .24 m and the hand begins .255 m ahead of the shoulder.
  const auto elbow=rotate({.065,-.33,.035},upper);
  const auto wrist=rotate(rotate({-.035,-.24,.22},upper),fore);
  const V hand{body.second+.035+elbow.x+wrist.x,elbow.y+wrist.y,elbow.z+wrist.z};
  check(hand.y-(-.57)>=.25,"inspection leaves actual hand near waist");
  check(hand.z>.355,"inspection fails front-of-torso clearance");
  check(hand.x>.2,"inspection brings held prop into the central face region");
 }
}
void humanoidIntent(){
 for(const char* asset:{"nell","alden","saffron","tess","rowan","vesper"}){
  SignatureMotion s(asset);s.start();s.advance(2.);
  bool head=false,upperArm=false,forearm=false,prop=false;
  const bool fastening=std::string_view(asset)=="alden";
  for(const auto& o:s.offsets()){
   head=head||o.joint=="head";
   upperArm=upperArm||o.joint==(fastening?"upper_arm_right":"upper_arm_left");
   forearm=forearm||o.joint==(fastening?"forearm_right":"forearm_left");
   prop=prop||o.joint==(fastening?"fastening":"held_prop");
   check(norm(o.translation)==0,"humanoid signature detaches a prop with translation");
   check(norm(o.rotation)<1.2,"humanoid signature exceeds restrained joint range");
   if(o.joint=="prop_hinge")check(o.rotation.y<-.6,"book or watch does not open its actual hinge");
  }
  check(head&&upperArm&&forearm&&prop,"humanoid action lacks articulated arm or bound-prop intent");
  s.advance(3.);check(s.active(),"humanoid inspection ends before five seconds");
  s.advance(2.);check(!s.active(),"humanoid inspection exceeds seven seconds");zero(s.offsets());
 }
}
// Exercise the new activities through their public procedural interface. A broad
// local arm gesture, hand-owned prop and articulated detail must each move;
// translations may never fake an inspection lift or detach a held object.
void wayfarerActivities(){
 const std::array<std::pair<const char*,double>,6> bodies{{{"iona",1.},{"milo",1.},{"orris",.78},{"fen",.96},{"oona",1.},{"quillan",1.10}}};
 for(const auto& body:bodies){
  SignatureMotion action(body.first);check(action.supported(),"missing wayfarer signature");
  const auto required=action.requiredJoints();
  auto has=[&](std::string_view joint){return std::find(required.begin(),required.end(),joint)!=required.end();};
  const bool visitor=std::string_view(body.first)=="quillan";
  const bool hinge=std::string_view(body.first)=="iona"||std::string_view(body.first)=="orris"||std::string_view(body.first)=="fen";
  for(const char* joint:{"head","upper_arm_left","forearm_left","hand_left","upper_arm_right","forearm_right","hand_right"})
   check(has(joint),"wayfarer lacks real articulated arm gesture");
  check(has("held_prop")==!visitor,"wayfarer prop ownership mismatch");
  check(has("prop_hinge")==hinge,"wayfarer hinge capability mismatch");
  check(has("ear_fin_left")==visitor&&has("ear_fin_right")==visitor,"visitor ear fin capability mismatch");
  std::vector<double> peaks(required.size(),0);
  action.start();
  for(int frame=0;frame<768;++frame){
   action.advance(1./120);
   const auto pose=action.offsets();
   for(size_t i=0;i<pose.size();++i){
    peaks[i]=std::max(peaks[i],norm(pose[i].rotation));
    check(norm(pose[i].translation)==0,"wayfarer detaches held prop or fin");
    check((pose[i].rotationSpace==SignatureRotationSpace::JointLocal)==(pose[i].joint=="prop_hinge"),"wayfarer hinge rotation space mismatch");
    check(norm(pose[i].rotation)<1.4,"wayfarer exceeds restrained gesture bounds");
   }
  }
  check(!action.active(),"wayfarer exceeds 6.4-second native evidence window");zero(action.offsets());
  for(double peak:peaks)check(peak>.035,"wayfarer required joint never moves");
  action.start();action.advance(2.);
  pets::motion::Vec3 upper{},fore{};
  for(const auto& o:action.offsets()){
   if(o.joint=="upper_arm_left")upper=o.rotation;
   if(o.joint=="forearm_left")fore=o.rotation;
  }
  auto rotate=[](pets::motion::Vec3 v,pets::motion::Vec3 r){
   const pets::motion::Vec3 z{v.x*std::cos(r.z)-v.y*std::sin(r.z),v.x*std::sin(r.z)+v.y*std::cos(r.z),v.z};
   const pets::motion::Vec3 x{z.x,z.y*std::cos(r.x)-z.z*std::sin(r.x),z.y*std::sin(r.x)+z.z*std::cos(r.x)};
   return pets::motion::Vec3{x.x*std::cos(r.y)+x.z*std::sin(r.y),x.y,-x.x*std::sin(r.y)+x.z*std::cos(r.y)};
  };
  const auto elbow=rotate({.065,.10-.43*body.second,.035},upper);
  const auto wrist=rotate(rotate({-.035,-.24*body.second,.22},fore),upper);
  check(elbow.y+wrist.y-(.10-.67*body.second)>.20,"wayfarer leaves inspection hand at waist");
  check(elbow.z+wrist.z>.35,"wayfarer fails front-of-chest clearance");
  // Iona's free hand must approach the map-bearing hand, and Orris's must
  // approach the face, using their actual wide/narrow shoulder placements.
  action.advance(1.8);
  auto hand=[&](bool left,double width){
   pets::motion::Vec3 u{},f{};
   for(const auto& o:action.offsets()){
    if(o.joint==(left?"upper_arm_left":"upper_arm_right"))u=o.rotation;
    if(o.joint==(left?"forearm_left":"forearm_right"))f=o.rotation;
   }
   const double sign=left?1.:-1.;
   const auto e=rotate({sign*.065,.10-.43*body.second,.035},u);
   const auto w=rotate(rotate({-sign*.035,-.24*body.second,.22},f),u);
   return pets::motion::Vec3{sign*(width+.035)+e.x+w.x,e.y+w.y,e.z+w.z};
  };
  if(std::string_view(body.first)=="iona"){
   const auto left=hand(true,.255),right=hand(false,.255);
   check(norm({left.x-right.x,left.y-right.y,left.z-right.z})<.40,"route gesture stays far from held map");
  }
  if(std::string_view(body.first)=="orris"){
   const auto right=hand(false,.425);
   check(norm({right.x,right.y-.308,right.z-.035})<.40,"spectacles gesture stays far from face");
   for(const auto& o:action.offsets())if(o.joint=="prop_hinge")
    check(std::abs(o.rotation.y)>.035&&o.rotation.x==0&&o.rotation.z==0&&o.rotationSpace==SignatureRotationSpace::JointLocal,"clock key fails to wind about authored local-Y shaft");
  }
 }
}
void rivetDirection(){
 SignatureMotion rivet("rivet");rivet.start();rivet.advance(2);
 const auto pose=rivet.offsets();bool antenna=false,salute=false;
 for(const auto& o:pose){
  if(o.joint=="antenna"){
   // Authored mast leans toward +X; positive model-Z rotation straightens it.
   const double newX=.08*std::cos(o.rotation.z)-.26*std::sin(o.rotation.z);
   check(std::abs(newX)<.02,"antenna check increases authored lean");antenna=true;
  }
  if(o.joint=="signature_arm_left"){
   // Authored mitten center relative to shoulder in glTF is(-.05,-.44,.03).
   // Qt's zero-yaw Euler rotation applies model Z then X. The raise brings
   // this actual endpoint forward and above its shoulder beside the face.
   const double afterZy=-.05*std::sin(o.rotation.z)-.44*std::cos(o.rotation.z);
   const double newY=afterZy*std::cos(o.rotation.x)-.03*std::sin(o.rotation.x);
   const double newZ=afterZy*std::sin(o.rotation.x)+.03*std::cos(o.rotation.x);
   check(newY>.03&&newZ>.3,"salute leaves arm below shoulder or behind body");salute=true;
  }
 }
 check(antenna&&salute,"missing Rivet signature joints");
}
}
int main(){try{allProfiles();cancellation();continuity();absentAnatomy();timeAndReadPurity();humanoidIntent();humanoidInspectionHeight();wayfarerActivities();rivetDirection();std::cout<<"Twenty-six bounded signature actions, cancellation, exact rest and support isolation passed\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
