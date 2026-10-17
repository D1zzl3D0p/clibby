#include "signature.h"
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace pets::character3d;
namespace {
constexpr std::array<const char*,14> assets{{"marmalade","pip","biscuit","clover","mossback","rivet","paper-heron","acorn-custodian","puddle","pocket-observatory","night-librarian","sprig","hearthling","crumb-golem"}};
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
double norm(pets::motion::Vec3 v){return std::hypot(std::hypot(v.x,v.y),v.z);}
double delta(const std::vector<SignatureOffset>& a,const std::vector<SignatureOffset>& b){
 check(a.size()==b.size(),"signature channels change over time");double result=0;
 for(size_t i=0;i<a.size();++i){check(a[i].joint==b[i].joint,"signature channel order changes");
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
void continuity(){constexpr double h=1e-5;for(const char* asset:assets){
 SignatureMotion a(asset),b(asset),c(asset);a.start();b.start();c.start();
 a.advance(1.8-h);b.advance(1.8);c.advance(1.8+h);
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
int main(){try{allProfiles();cancellation();continuity();absentAnatomy();timeAndReadPurity();rivetDirection();std::cout<<"Fourteen bounded signature actions, cancellation, exact rest and support isolation passed\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
