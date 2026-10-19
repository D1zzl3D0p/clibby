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
// Production Pip and Mossback joint/contact coordinates converted from
// Blender (x,y,z) to the motion adapter's (x,z,-y) frame.
LimbArray<Leg> catalogueAnatomy(bool low){
 const double x=low?.44:.29,fore=low?.56:.59,hind=low?-.69:-.66;
 LimbArray<Leg> legs;legs.setCount(4);
 for(int i=0;i<4;++i){const double side=i%2==0?-x:x;const bool front=i<2;
  legs[i]={{side,front?(low?.48:.96):(low?.46:.91),front?fore:hind},
           {side,front?(low?.2544:.5088):(low?.2438:.4823),front?(low?.49:.46):(low?-.59:-.48)},
           {side,low?.1275:.17,front?(low?.535:.565):(low?-.715:-.685)},
           {side,0,front?(low?.62:.65):(low?-.63:-.60)}};
 }
 return legs;
}
void midstrideReversals(){
 constexpr double dt=1./120;
 bool exercisedSwing=false,exercisedPair=false,exercisedSteering=false;
 for(bool low:{false,true})for(int reverseTick:{96,128,160,176,200})for(bool rapid:{false,true}){
  const auto legs=catalogueAnatomy(low);
  Motion m(legs,{},false,0,Gait::QuadrupedWalk);
  m.setFacingBias(-10.8*3.141592653589793/180);m.setCameraFacingTurns(true);m.reset(0,{-120,120});
  for(int i=0;i<64;++i)m.step(dt);
  m.target(35);auto previous=m.pose();bool reversedSwing=false;
  for(int tick=0;tick<1200;++tick){
   if(tick==reverseTick){reversedSwing=m.state().anyFootSwinging();m.target(-20);}
   // Repeated requests during the same lifted phase must not replace its
   // original basis, including an immediate reversal back to the first goal.
   if(rapid&&(tick==reverseTick+1||tick==reverseTick+3))m.target(35);
   if(rapid&&(tick==reverseTick+2||tick==reverseTick+4))m.target(-20);
   if(tick==reverseTick+8){
    const auto old=m.pose();m.reconcile(m.state().rootX+12,{-108,132},0,0);
    const auto moved=m.pose();for(int i=0;i<4;++i)check(std::abs(moved.contacts[i].z-old.contacts[i].z-.12)<1e-10,"midstride support move loses a paw pin");
    previous=moved;
   }
   m.step(dt);const auto p=m.pose();
   for(int i=0;i<4;++i){
    const double delta=distance(previous.contacts[i],p.contacts[i]);
    // Even a full leg-length swing in the minimum .2 s duration stays well
    // below this generous 18 m/s bound; a one-tick heading-basis jump does not.
    if(delta>18*dt){std::cerr<<(low?"Mossback":"Pip")<<" reverse tick "<<reverseTick<<" tick "<<tick<<" foot "<<i<<" contact delta "<<delta<<"\n";}
    check(delta<=18*dt,"midstride reversal teleports a spatial paw");
    if(previous.grounded[i]&&p.grounded[i])check(delta<1e-9,"midstride reversal slides a planted paw");
   }
   const int swinging=int(std::count(p.swing.begin(),p.swing.end(),true));
   check(swinging<=2,"midstride reversal exceeds its declared swing support");
   check(std::count(p.grounded.begin(),p.grounded.end(),true)>=2,"midstride reversal loses its declared support");
   if(swinging==2){
    // Travel retains diagonal pairs. A foot-led turn can transfer through
    // ipsilateral support while the planted leading paw owns the body path.
    if(!p.pivoting)check(!((p.swing[0]&&p.swing[2])||(p.swing[1]&&p.swing[3])),"paired travel lifts both paws on one side");
    exercisedPair=true;
   }
   for(int i=0;i<4;++i){
    if(previous.grounded[i]&&p.grounded[i])check(std::abs(previous.footYaw[i]-p.footYaw[i])<1e-9,
      "turn twists a loaded paw instead of steering during its step");
    if(p.swing[i]&&std::abs(previous.footYaw[i]-p.footYaw[i])>1e-7)exercisedSteering=true;
   }
   previous=p;
  }
  exercisedSwing|=reversedSwing;
  // Reset after a reversed swing must clear the retained trajectory basis.
  m.reset(0,{-120,120});
  m.target(35);previous=m.pose();
  for(int tick=0;tick<180;++tick){m.step(dt);const auto p=m.pose();for(int i=0;i<4;++i)check(distance(previous.contacts[i],p.contacts[i])<=18*dt,"reset retains a discontinuous swing basis");previous=p;}
  m.target(-20);m.step(dt);m.pickup(0,60);m.release({0,0});previous=m.pose();
  for(int tick=0;tick<5000&&!m.settled();++tick){
   const auto phase=m.state().phase;m.step(dt);const auto p=m.pose();
   const auto next=m.state().phase;
   const bool grounded=phase!=pets::character::DropPhase::Airborne&&next!=pets::character::DropPhase::Airborne&&next!=pets::character::DropPhase::Held;
   if(grounded){for(int i=0;i<4;++i){check(distance(previous.contacts[i],p.contacts[i])<=18*dt,"pickup/drop retains a discontinuous swing basis");if(previous.grounded[i]&&p.grounded[i])check(distance(previous.contacts[i],p.contacts[i])<1e-9,"recovery slides a planted paw");}}
   previous=p;
  }
  check(m.settled(),"reversed quadruped drop recovery never settles");
 }
 check(exercisedSwing,"reversal fixtures never interrupt an in-flight paw");
 check(exercisedPair,"reversal fixtures never exercise coordinated paired placement");
 check(exercisedSteering,"reversal fixtures never steer a lifted paw");
}
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
 if(grounded){
  check(lifted<=2,"quadruped exceeds its declared walk/pivot support");
  check(std::count(p.grounded.begin(),p.grounded.end(),true)>=2,"quadruped loses its declared support contacts");
  if(lifted==2)check(!((p.swing[0]&&p.swing[2])||(p.swing[1]&&p.swing[3])),"quadruped paired swing lifts both paws on one side");
 }
}
}
int main(){try{
 midstrideReversals();
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
