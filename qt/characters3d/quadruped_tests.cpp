#include "pose.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace pets::character3d;
using pets::character::Gait;
using pets::character::LimbArray;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
double distance(Vec3 a,Vec3 b){return std::hypot(std::hypot(a.x-b.x,a.y-b.y),a.z-b.z);}
Vec3 rotate(Vec3 p,double yaw){return {std::cos(yaw)*p.x+std::sin(yaw)*p.z,p.y,-std::sin(yaw)*p.x+std::cos(yaw)*p.z};}
LimbArray<Leg> anatomy(){return {
 {{.23,.78,.58},{.24,.43,.45},{.23,.12,.52},{.23,0,.62}},
 {{-.23,.78,.58},{-.24,.43,.45},{-.23,.12,.52},{-.23,0,.62}},
 {{.25,.77,-.58},{.26,.46,-.34},{.25,.12,-.62},{.25,0,-.52}},
 {{-.25,.77,-.58},{-.26,.46,-.34},{-.25,.12,-.62},{-.25,0,-.52}}};}
void authoredBend(const Pose& pose,const LimbArray<Leg>& legs){
 for(std::size_t i=0;i<legs.size();++i){
  const auto& rest=legs[i];const auto& solved=pose.legs[i];
  Vec3 axis{solved.ankle.x-solved.hip.x,solved.ankle.y-solved.hip.y,solved.ankle.z-solved.hip.z};
  const double norm=axis.x*axis.x+axis.y*axis.y+axis.z*axis.z;
  auto reject=[&](Vec3 v){const double d=(v.x*axis.x+v.y*axis.y+v.z*axis.z)/norm;return Vec3{v.x-d*axis.x,v.y-d*axis.y,v.z-d*axis.z};};
  const Vec3 restAxis{rest.ankle.x-rest.hip.x,rest.ankle.y-rest.hip.y,rest.ankle.z-rest.hip.z};
  const Vec3 restKnee{rest.knee.x-rest.hip.x,rest.knee.y-rest.hip.y,rest.knee.z-rest.hip.z};
  const double fraction=(restAxis.x*restKnee.x+restAxis.y*restKnee.y+restAxis.z*restKnee.z)/(restAxis.x*restAxis.x+restAxis.y*restAxis.y+restAxis.z*restAxis.z);
  const auto expected=reject({restKnee.x-restAxis.x*fraction,restKnee.y-restAxis.y*fraction,restKnee.z-restAxis.z*fraction});
  const auto actual=reject({solved.knee.x-solved.hip.x,solved.knee.y-solved.hip.y,solved.knee.z-solved.hip.z});
  const double cosine=(expected.x*actual.x+expected.y*actual.y+expected.z*actual.z)/(std::hypot(std::hypot(expected.x,expected.y),expected.z)*std::hypot(std::hypot(actual.x,actual.y),actual.z));
  check(cosine>.99999,"quadruped knee reverses its authored fore/hind bend plane");
 }
}
void inspect(const Pose& before,const Pose& p,const LimbArray<Leg>& legs,bool grounded){
 check(p.legs.size()==4&&p.contacts.size()==4&&p.swing.size()==4&&p.grounded.size()==4,"quadruped has phantom or missing feet");
 int lifted=0;
 for(int i=0;i<4;++i){
  check(std::isfinite(p.legs[i].knee.x)&&std::isfinite(p.legs[i].knee.y)&&std::isfinite(p.legs[i].knee.z),"nonfinite quadruped knee");
  if(p.swing[i])++lifted;
  if(grounded&&before.grounded[i]&&p.grounded[i])check(distance(before.contacts[i],p.contacts[i])<1e-9,"quadruped slides planted sole");
  if(grounded){
   auto ankle=rotate(p.legs[i].ankle,p.yaw);auto offset=Vec3{legs[i].contact.x-legs[i].ankle.x,legs[i].contact.y-legs[i].ankle.y,legs[i].contact.z-legs[i].ankle.z};
   const double c=std::cos(p.footPitch[i]),s=std::sin(p.footPitch[i]);offset={offset.x,c*offset.y-s*offset.z,s*offset.y+c*offset.z};offset=rotate(offset,p.footYaw[i]);
   check(distance({ankle.x+offset.x,ankle.y+offset.y,ankle.z+offset.z+p.rootX/100},p.contacts[i])<1e-8,"quadruped rendered paw disagrees with world pin");
  }
 }
 if(grounded)check(lifted<=1,"quadruped lifts multiple feet during walk/turn");
}
}
int main(){try{
 auto legs=anatomy();bool rejected=false;try{Motion bad(legs);}catch(const std::exception&){rejected=true;}check(rejected,"four legs silently use biped gait");
 rejected=false;try{Motion bad(legs,{},false,0,static_cast<Gait>(99));}catch(const std::exception&){rejected=true;}check(rejected,"unknown spatial gait accepted");
 Motion motion(legs,{},false,0,Gait::QuadrupedWalk);check(motion.pose().legs.size()==4,"quadruped pose loses hind legs");
 authoredBend(motion.pose(),legs);
 auto previous=motion.pose();std::array<bool,4> stepped{};
 for(double target:{90.,-90.,35.}){
  motion.target(target);
  for(int tick=0;tick<14000&&!motion.settled();++tick){motion.step(1./120);auto p=motion.pose();inspect(previous,p,legs,true);for(int i=0;i<4;++i)stepped[i]=stepped[i]||p.swing[i];previous=p;}
  check(motion.settled(),"quadruped walk/reversal never settles");
  check(std::abs(motion.state().rootX-target)<.03,"quadruped does not arrive");
  authoredBend(motion.pose(),legs);
 }
 check(std::all_of(stepped.begin(),stepped.end(),[](bool x){return x;}),"some quadruped legs never step");
 motion.setSupportBounds(pets::character::GroundSpan{-120,120});motion.pickup(0,180);motion.release({0,0});bool landed=false;
 for(int tick=0;tick<10000&&!motion.settled();++tick){auto phase=motion.state().phase;motion.step(1./120);auto p=motion.pose();const auto after=motion.state().phase;bool support=after!=pets::character::DropPhase::Held&&after!=pets::character::DropPhase::Airborne;
  inspect(previous,p,legs,support&&phase!=pets::character::DropPhase::Airborne);if(!support)for(int i=0;i<4;++i)check(!p.grounded[i],"airborne quad has support");else{landed=true;for(auto contact:p.contacts)check(contact.z>=-1.200001&&contact.z<=1.200001,"quad touchdown exceeds bounds");}previous=p;}
 check(landed&&motion.settled(),"quadruped drop/recovery never settles");
 // Host quantization/blocking must preserve all stance pins. The spatial
 // adapter must aggregate hind contacts when a moving support is reconciled.
 Motion hosted(legs,{},false,0,Gait::QuadrupedWalk);hosted.setFacingBias(-.3);hosted.setCameraFacingTurns(true);hosted.reset(.25,{-200,200});
 auto last=hosted.pose();
 for(double target:{70.,-70.}){
  hosted.target(target);
  for(int tick=0;tick<14000&&!hosted.settled();++tick){
   auto proposal=hosted.propose(1./120);const double root=hosted.state().rootX;
   double realized=std::round((root+proposal.displacement)*100)/100;
   if(tick==60){realized=root;hosted.realize(realized,true);}else hosted.realize(realized);
   auto p=hosted.pose();inspect(last,p,legs,true);last=p;
  }
  check(hosted.settled(),"quantized or blocked quadruped never settles");
 }
 const auto stationary=hosted.pose();hosted.reconcile(hosted.state().rootX+12,{-188,212},0,0);const auto moved=hosted.pose();
 for(int i=0;i<4;++i)check(std::abs(moved.contacts[i].z-stationary.contacts[i].z-.12)<1e-10,"moving support does not carry all four world pins");
 const auto quiet=motion.pose();for(int i=0;i<20;++i){const auto p=motion.pose();check(distance(p.contacts[3],quiet.contacts[3])==0,"pose read advances hind foot");}
 std::cout<<"Spatial quadruped gait, turns, contacts and recovery passed\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
