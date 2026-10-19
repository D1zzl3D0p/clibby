#include "controller.h"
#include "characters/fixtures.h"
#include "characters3d/builtin_assets.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>
#include <QMatrix4x4>
#include <QQmlListReference>
#include <stdexcept>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <set>
namespace {
pets::character3d::Vec3 point(QJsonArray a){
 if(a.size()!=3)throw std::runtime_error("invalid rig point");
 for(auto value:a)if(!value.isDouble()||!std::isfinite(value.toDouble())||std::abs(value.toDouble())>10)throw std::runtime_error("invalid rig coordinate");
 return {a[0].toDouble(),a[2].toDouble(),-a[1].toDouble()};
}
QVector3D vector(pets::character3d::Vec3 p){return {float(p.x),float(p.y),float(p.z)};}
}
WolfMotion::WolfMotion(const QString& path,QObject* parent,std::function<double()> clock):QObject(parent),behaviorClock_(std::move(clock)){
 if(!behaviorClock_)behaviorClock_=[] {return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();};
 QFile f(path);if(!f.open(QIODevice::ReadOnly))throw std::runtime_error("3D rig missing");
 auto document=QJsonDocument::fromJson(f.readAll());
 if(!document.isObject())throw std::runtime_error("invalid 3D rig document");
 auto rig=document.object();std::map<QString,pets::character3d::Vec3> heads;
 const auto authoredYaw=rig["presentation"].toObject().value("yaw_bias_degrees");
 if(!authoredYaw.isUndefined()){
  if(!authoredYaw.isDouble()||!std::isfinite(authoredYaw.toDouble())||authoredYaw.toDouble()<-90||authoredYaw.toDouble()>0)
   throw std::runtime_error("invalid authored facing angle");
  authoredFacingBiasDegrees_=authoredYaw.toDouble();
 }
 const auto gait=rig.value("gait");
 if(gait.isUndefined()||gait.toString()=="biped")gait_=pets::character::Gait::Biped;
 else if(gait.toString()=="quadrupedWalk")gait_=pets::character::Gait::QuadrupedWalk;
 else throw std::runtime_error("unsupported 3D gait");
 const bool quadruped=gait_==pets::character::Gait::QuadrupedWalk;
 const auto declaredJoints=rig["joints"].toArray();
 if(declaredJoints.isEmpty()||declaredJoints.size()>128)throw std::runtime_error("invalid 3D joint count");
 for(auto value:declaredJoints){auto j=value.toObject();QString name=j["name"].toString();
  if(name.isEmpty()||heads.count(name))throw std::runtime_error("invalid or duplicate rig joint");
  heads[name]=point(j["head"].toArray());Joint entry;entry.parent=j["parent"].toString();joints_[name]=entry;
 }
 for(const auto& [name,joint]:joints_){
  std::set<QString> ancestry{name};auto parent=joint.parent;
  while(!parent.isEmpty()){
   if(!joints_.count(parent)||!ancestry.insert(parent).second)throw std::runtime_error("invalid rig hierarchy");
   parent=joints_.at(parent).parent;
  }
 }
 for(const auto groupValue:rig["skinBounds"].toArray()) {
  const auto group=groupValue.toArray();
  if(group.isEmpty()||group.size()>4||skinBounds_.size()>=64)throw std::runtime_error("invalid skin bound group");
  std::vector<SkinBound> bounds;
  for(const auto entryValue:group){
   const auto entry=entryValue.toObject();const auto name=entry["joint"].toString();
   if(!joints_.count(name))throw std::runtime_error("unknown skin bound joint");
   auto read=[](QJsonValue value){
    const auto a=value.toArray();if(a.size()!=3)throw std::runtime_error("invalid skin bound point");
    for(const auto v:a)if(!v.isDouble()||!std::isfinite(v.toDouble())||std::abs(v.toDouble())>10)throw std::runtime_error("unbounded skin point");
    return QVector3D(a[0].toDouble(),a[1].toDouble(),a[2].toDouble());
   };
   const auto lo=read(entry["minimum"]),hi=read(entry["maximum"]);
   if(lo.x()>hi.x()||lo.y()>hi.y()||lo.z()>hi.z())throw std::runtime_error("inverted skin bounds");
   bounds.push_back({name,lo,hi});
  }skinBounds_.push_back(std::move(bounds));
 }
 wings_=heads.count("wing_shoulder_left")&&heads.count("wing_shoulder_right");
 if(quadruped&&wings_)throw std::runtime_error("unsupported quadruped wings");
 const auto arms=rig.value("arms");
 if(!arms.isUndefined()&&!arms.isBool())throw std::runtime_error("invalid arm capability");
 hasArms_=arms.isUndefined()?(!wings_&&!quadruped):arms.toBool();
 if(hasArms_&&(wings_||quadruped))throw std::runtime_error("unsupported arm anatomy");
 skinName_=quadruped?"MarmaladeSkin":wings_?"RavenSkin":"WerewolfSkin";
 if(rig.contains("skin"))skinName_=rig["skin"].toString();
 const auto* asset=pets::character3d::builtinSkin(skinName_);
 if(!asset||asset->quadruped!=quadruped)throw std::runtime_error("rig skin does not match trusted anatomy");
 catalogueStyle_=asset->catalogueStyle;
 personality_=pets::behavior::personalityFor(asset->disposition);
 signature_=pets::character3d::SignatureMotion(asset->asset);
 signatureBound_=signature_.supported();
 for(auto name:signature_.requiredJoints())
  signatureBound_=signatureBound_&&heads.count(QString::fromUtf8(name.data(),int(name.size())));
 for(const auto* role:{"pelvis","spine","head"})if(!heads.count(role))throw std::runtime_error("missing body joint");
 legRoles_=quadruped?std::vector<QString>{"foreLeft","foreRight","hindLeft","hindRight"}:std::vector<QString>{"left","right"};
 legs_.setCount(legRoles_.size());
 const auto contacts=rig["contacts"].toObject();
 if(contacts.size()!=int(legRoles_.size()))throw std::runtime_error("rig contact count does not match gait");
 int i=0;for(const auto& side:legRoles_){
  for(auto role:{"thigh_","shin_","foot_"})if(!heads.count(QString(role)+side))throw std::runtime_error("missing leg joint");
  if(joints_.at("shin_"+side).parent!="thigh_"+side||joints_.at("foot_"+side).parent!="shin_"+side)throw std::runtime_error("invalid leg chain hierarchy");
  auto leg=pets::character3d::Leg{heads.at("thigh_"+side),heads.at("shin_"+side),heads.at("foot_"+side),point(contacts[side].toArray())};
  if((vector(leg.knee)-vector(leg.hip)).length()<1e-5||(vector(leg.ankle)-vector(leg.knee)).length()<1e-5)throw std::runtime_error("degenerate leg chain");
  legs_[i++]=leg;
 }
 i=0;if(hasArms_)for(auto side:{QString("left"),QString("right")}) {
  // Side-aware outward/downward elbow bend keeps the thick sleeve clear of
  // torso and cheek during the preview's hold and semantic gestures.
  for(auto role:{"upper_arm_","forearm_","hand_"})if(!heads.count(QString(role)+side))throw std::runtime_error("missing arm joint");
  if(joints_.at("forearm_"+side).parent!="upper_arm_"+side||joints_.at("hand_"+side).parent!="forearm_"+side)
   throw std::runtime_error("invalid arm chain hierarchy");
  arms_[i++]={heads.at("upper_arm_"+side),heads.at("forearm_"+side),heads.at("hand_"+side),{0,-.08,0},{heads.at("upper_arm_"+side).x>0?1.:-1.,-.4,.2}};
 }
 for(auto& arm:arms_)arm.torsoPivot=heads.at("spine");
 double wingReferenceReach=0;
 if(wings_) {
  wingReferenceReach=512;
  auto distance=[](pets::character3d::Vec3 a,pets::character3d::Vec3 b){return std::sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y)+(a.z-b.z)*(a.z-b.z));};
  for(auto side:{QString("left"),QString("right")}) {
   for(auto role:{"wing_shoulder_","wing_elbow_","wing_wrist_"})if(!heads.count(QString(role)+side))throw std::runtime_error("missing wing joint");
   wingReferenceReach=std::min(wingReferenceReach,100*(distance(heads.at("wing_shoulder_"+side),heads.at("wing_elbow_"+side))+distance(heads.at("wing_elbow_"+side),heads.at("wing_wrist_"+side))));
  }
 }
 motion_=std::make_unique<pets::character3d::Motion>(legs_,hasArms_?std::optional<std::array<pets::character3d::Arm,2>>{arms_}:std::optional<std::array<pets::character3d::Arm,2>>{},wings_,wingReferenceReach,gait_);
 using pets::character::Role;using pets::character::roleBit;
 expressionRoles_=roleBit(Role::Head);
 if(heads.count("ear_left"))expressionRoles_|=roleBit(Role::EarLeft);
 if(heads.count("ear_right"))expressionRoles_|=roleBit(Role::EarRight);
 if(heads.count("tail_base"))expressionRoles_|=roleBit(Role::Tail);
 if(hasArms_)expressionRoles_|=roleBit(Role::WristLeft)|roleBit(Role::WristRight);
 motion_->expressions()=pets::character::ExpressionMotion(expressionRoles_);
 behaviorNow_=behaviorTime();curiosity_.reset(behaviorNow_,pets::behavior::Profile::Werewolf,{expressionRoles_,hasArms_,true},personality_);
 motion_->setSpeedMultiplier(2);
 // Scheduling belongs to the preview wrapper or resident application.
}
void WolfMotion::bind(QObject* scene){
 if(!scene)throw std::runtime_error("3D scene missing");
 avatar_=scene->findChild<QObject*>("avatar");
 if(!avatar_)throw std::runtime_error("3D avatar root missing");
 for(auto& [name,j]:joints_){j.node=scene->findChild<QObject*>(name);if(!j.node)throw std::runtime_error(("unaddressable joint "+name).toStdString());
  j.restScale=j.node->property("scale").value<QVector3D>();
  j.restPosition=j.node->property("position").value<QVector3D>();j.restRotation=j.node->property("rotation").value<QQuaternion>();
  const auto body=avatar_->property("sceneRotation").value<QQuaternion>();
  const auto origin=avatar_->property("scenePosition").value<QVector3D>();
  j.restWorldPosition=body.conjugated().rotatedVector(j.node->property("scenePosition").value<QVector3D>()-origin);
  j.restWorldRotation=body.conjugated()*j.node->property("sceneRotation").value<QQuaternion>();
 }
 auto model=scene->findChild<QObject*>(skinName_);
 auto skin=model?model->property("skin").value<QObject*>():nullptr;
 QQmlListReference deform(skin,"joints");
 if(!skin||!deform.isValid()||deform.count()!=int(joints_.size()))throw std::runtime_error("converted model skin is not connected to all deform joints");
 for(auto& [name,j]:joints_){bool connected=false;for(int i=0;i<deform.count();++i)connected|=deform.at(i)==j.node;if(!connected)throw std::runtime_error("addressable joint missing from live model skin");}
 apply();
}
QString WolfMotion::status()const{auto s=motion_->state();return QString("Target %1 • root %2 • %3").arg(s.targetX,0,'f',1).arg(s.rootX,0,'f',1).arg(settled()?"settled, no motion timer":"moving");}
void WolfMotion::wake(){}
void WolfMotion::setWalkingSpeed(double multiplier){motion_->setSpeedMultiplier(multiplier);emit changed();}
void WolfMotion::target(double x){manualInput();motion_->target(x);wake();emit changed();}
bool WolfMotion::hop(double height){manualInput();const bool accepted=motion_->hop(height);emit changed();return accepted;}
bool WolfMotion::flyTo(double x,double elevation){manualInput();const bool accepted=motion_->flyTo(x,elevation);emit changed();return accepted;}
void WolfMotion::land(){manualInput();motion_->land();emit changed();}
void WolfMotion::stop(){target(motion_->state().rootX);}
void WolfMotion::impulse(){manualInput();motion_->impulse();wake();emit changed();}
void WolfMotion::pause(bool p){paused_=p;emit changed();}
void WolfMotion::advance(double dt){if(paused_)return;motion_->step(dt);advancePresentation(dt);}
void WolfMotion::advancePresentation(double dt,bool render){
 if(!std::isfinite(dt)||dt<0)throw std::invalid_argument("invalid presentation interval");
 if(paused_)return;
 const auto state=motion_->state();
 if(state.phase!=pets::character::DropPhase::Grounded || state.elevation>.01 || state.anyFootSwinging() || std::abs(state.targetX-state.rootX)>.05 || std::abs(state.velocity)>.02)
  signature_.cancel();
 signature_.advance(dt);
 if(!externalReaction_){gestureTime_=std::min(1.,gestureTime_+std::min(dt,1./30));submitReaction();}
 if(render){emit changed();apply();emit anchorsChanged();}
}
void WolfMotion::setReaction(const QString& semantic,double progress){
 if(!semantic.isEmpty()&&semantic!="look"&&semantic!="wave"&&semantic!="point"&&semantic!="present"&&semantic!="nod"&&semantic!="success"&&semantic!="failure")throw std::runtime_error("unknown gesture");
 if(!std::isfinite(progress))throw std::runtime_error("invalid gesture progress");
 if(!semantic.isEmpty())signature_.cancel();
 externalReaction_=true;
 if(gesture_!=semantic)reactionPhase_=-1;
 gesture_=semantic;gestureTime_=semantic.isEmpty()?1:std::clamp(progress,0.,1.);submitReaction();
 emit changed();apply();emit anchorsChanged();
}
QVector3D WolfMotion::headAnchor()const {
 auto found=joints_.find("head");
 return found!=joints_.end()&&found->second.node?found->second.node->property("scenePosition").value<QVector3D>():QVector3D();
}
QVector3D WolfMotion::cardAnchor()const{return headAnchor()+QVector3D(0,.42,0);}
QVector3D WolfMotion::effectAnchor()const{return headAnchor()+QVector3D(0,.3,0);}
QVector3D WolfMotion::grip(int side)const {
 if(!hasArms_||side<0||side>1)return headAnchor();
 const auto& hand=joints_.at(side==0?"hand_left":"hand_right");
 if(!hand.node)return {};
 auto local=hand.restWorldRotation.conjugated().rotatedVector(vector(arms_[side].gripOffset));
 return hand.node->property("scenePosition").value<QVector3D>()+hand.node->property("sceneRotation").value<QQuaternion>().rotatedVector(local);
}
void WolfMotion::reach(int side,QVector3D target) {
 if(side<0||side>1||!std::isfinite(target.x())||!std::isfinite(target.y())||!std::isfinite(target.z()))throw std::runtime_error("invalid grip goal");
 if(!hasArms_)return;
 manualInput();motion_->armGoal(side,{target.x(),target.y(),target.z()});emit changed();
}
void WolfMotion::release(int side) {
 if(side<0||side>1)throw std::runtime_error("invalid grip side");
 manualInput();motion_->clearArmGoal(side);emit changed();
}
void WolfMotion::gesture(const QString& semantic) {
 if(semantic!="look"&&semantic!="wave"&&semantic!="point"&&semantic!="present"&&semantic!="nod"&&semantic!="success"&&semantic!="failure")throw std::runtime_error("unknown gesture");
 manualInput();externalReaction_=false;gesture_=semantic;gestureTime_=0;reactionPhase_=-1;submitReaction();wake();emit changed();
}
void WolfMotion::submitReaction(){
 using pets::character::Role;
 // Semantic phases choose goals; followers generate every transition.
 int phase=gestureTime_>=1?3:gestureTime_<.12?0:gestureTime_<.7?1:2;
 if(phase==reactionPhase_)return;
 reactionPhase_=phase;
 auto& expression=motion_->expressions();
 if(phase>=2){expression.clear();motion_->clearArmGoal(0);return;}
 if(phase==0){expression.goal(Role::Head,{-.06,0,0});return;}
 expression.goal(Role::Head,{gesture_=="failure"?.26:gesture_=="nod"?.2:-.14,0,0});
 if(gesture_=="look")expression.look({.45,.15,1});
 if(gesture_=="failure")expression.goal(Role::EarLeft,{0,0,-.22});
 if(!hasArms_)return;
 if(gesture_=="wave"||gesture_=="success"){
  motion_->armGoal(0,{.79,1.94,.3});expression.goal(Role::WristLeft,{3.141592653589793,0,0});
 }else if(gesture_=="point")motion_->armGoal(0,{.72,1.55,.4});
 else if(gesture_=="present")motion_->armGoal(0,{.68,1.3,.35});
}
void WolfMotion::manualInput(){
 signature_.cancel();
 if(autonomous_){autonomous_=false;motion_->target(motion_->state().rootX);motion_->expressions().clear();motion_->clearArmGoal(0);motion_->clearArmGoal(1);}
}
bool WolfMotion::signature(){
 const auto state=motion_->state();
 if(!signatureBound_||paused_||state.phase!=pets::character::DropPhase::Grounded||state.elevation>.01||!motion_->settled()||gestureTime_<1||signature_.active())return false;
 manualInput();
 motion_->expressions().clear();
 const bool started=signature_.start();
 if(started){wake();emit changed();}
 return started;
}
void WolfMotion::cancelSignature(bool immediate){
 if(immediate)signature_.reset();else signature_.cancel();
}
void WolfMotion::setAutonomous(bool enabled){
 behaviorNow_=behaviorTime();
 if(autonomous_&&!enabled)motion_->target(motion_->state().rootX);
 autonomous_=enabled;motion_->expressions().clear();motion_->clearArmGoal(0);motion_->clearArmGoal(1);
 curiosity_.reset(behaviorNow_,pets::behavior::Profile::Werewolf,{expressionRoles_,hasArms_,true},personality_);emit changed();
}
void WolfMotion::setSeed(quint32 seed){behaviorNow_=behaviorTime();seed_=seed;curiosity_=pets::behavior::Curiosity(seed);curiosity_.reset(behaviorNow_,pets::behavior::Profile::Werewolf,{expressionRoles_,hasArms_,true},personality_);emit changed();}
std::optional<double> WolfMotion::nextBehaviorDeadline()const{return autonomous_&&!paused_?curiosity_.nextDeadline():std::nullopt;}
void WolfMotion::advanceBehavior(double now){
 behaviorNow_=now;if(!autonomous_)return;
 auto s=motion_->state();
 pets::behavior::Observation observation{s.rootX,-120,120,55,paused_,false,gestureTime_<1,false,std::abs(s.rootX-s.targetX)<.1};
 for(const auto& command:curiosity_.advance(now,observation)){
  using Kind=pets::behavior::Command::Kind;
  auto& expression=motion_->expressions();
  switch(command.kind){
   case Kind::Look:expression.look(command.value);break;
   case Kind::RoleGoal:expression.goal(command.role,command.value);break;
   case Kind::RoleImpulse:expression.impulse(command.role,command.value);break;
   case Kind::Clear:expression.clear(command.role);break;
   case Kind::StopNavigation:motion_->target(s.rootX);break;
   case Kind::Navigate:motion_->target(command.targetX);break;
   case Kind::Reach:if(hasArms_)motion_->reach(command.side,command.value);break;
   case Kind::ReleaseReach:motion_->clearArmGoal(command.side);break;
  }
 }emit changed();
}
void WolfMotion::look(QVector3D direction){manualInput();motion_->expressions().look({direction.x(),direction.y(),direction.z()});emit changed();}
void WolfMotion::clearExpression(){manualInput();motion_->expressions().clear();motion_->clearArmGoal(0);motion_->clearArmGoal(1);emit changed();}
void WolfMotion::roleGoal(const QString& name,QVector3D value){
 manualInput();using pets::character::Role;
 auto role=name=="tail"?Role::Tail:name=="earLeft"?Role::EarLeft:name=="earRight"?Role::EarRight:Role::Head;
 motion_->expressions().goal(role,{value.x(),value.y(),value.z()});emit changed();
}
void WolfMotion::aim(const QString& name,QVector3D from,QVector3D to){
 auto& j=joints_.at(name);QQuaternion delta=QQuaternion::rotationTo(from.normalized(),to.normalized());
 QQuaternion parentWorld=j.parent.isEmpty()?QQuaternion():joints_.at(j.parent).node->property("sceneRotation").value<QQuaternion>();
 QQuaternion body=avatar_?avatar_->property("sceneRotation").value<QQuaternion>():QQuaternion();
 QQuaternion desired=body*delta*j.restWorldRotation;
 j.node->setProperty("rotation",QVariant::fromValue(parentWorld.conjugated()*desired));
}
void WolfMotion::place(const QString& name,QVector3D modelPosition){
 auto& joint=joints_.at(name);
 const auto world=avatar_->property("sceneTransform").value<QMatrix4x4>().map(modelPosition);
 const auto parent=joint.parent.isEmpty()?avatar_:joints_.at(joint.parent).node;
 bool invertible=false;
 const auto inverse=parent->property("sceneTransform").value<QMatrix4x4>().inverted(&invertible);
 if(!invertible)throw std::runtime_error("noninvertible live joint parent");
 joint.node->setProperty("position",QVariant::fromValue(inverse.map(world)));
}
void WolfMotion::setEyeOpenness(double value){
 if(!std::isfinite(value))return;
 eyeOpenness_=std::clamp(value,0.,1.);
}
void WolfMotion::apply(){
 if(joints_.empty()||!joints_.begin()->second.node)return;
 // The resident may reset/realize the core directly. Refresh the avatar's
 // QML heading binding before solving live joints in its scene frame.
 emit changed();
 for(auto& [name,j]:joints_){j.node->setProperty("position",QVariant::fromValue(j.restPosition));j.node->setProperty("rotation",QVariant::fromValue(j.restRotation));}
 for(auto side:{"left","right"}){
  auto found=joints_.find(QString("eye_")+side);
  if(found!=joints_.end()){
   const auto rest=found->second.restScale;
   found->second.node->setProperty("scale",QVariant::fromValue(QVector3D(rest.x(),rest.y()*float(std::max(.05,eyeOpenness_)),rest.z())));
  }
 }
 auto p=motion_->pose();
 // Pelvis follows grounded support; ankle articulation retains world soles.
 auto& pelvis=joints_.at("pelvis");const auto shift=vector(p.legs[0].hip)-vector(legs_[0].hip);
 pelvis.node->setProperty("position",QVariant::fromValue(pelvis.restPosition+shift));
 // The renderer applies only solved snapshots from the procedural core.
 auto rotation=[](pets::motion::Vec3 r){
  constexpr double degrees=180/3.141592653589793;
  return QQuaternion::fromAxisAndAngle(0,1,0,float(r.y*degrees))*
         QQuaternion::fromAxisAndAngle(1,0,0,float(r.x*degrees))*
         QQuaternion::fromAxisAndAngle(0,0,1,float(r.z*degrees));
 };
 auto& spine=joints_.at("spine");
 spine.node->setProperty("rotation",QVariant::fromValue(spine.restRotation*rotation({p.lean,0,0})));
 for(const auto& named:std::array<std::pair<QString,pets::motion::Vec3>,4>{{{"head",p.head},{"ear_left",p.ears[0]},{"ear_right",p.ears[1]},{"tail_base",p.tail}}}){
  auto found=joints_.find(named.first);if(found==joints_.end())continue;
  auto& joint=found->second;joint.node->setProperty("rotation",QVariant::fromValue(joint.restRotation*rotation(named.second)));
 }
 // Convert neutral model-axis offsets through the current parent frame;
 // explicit local hinges instead turn about their own posed authored shaft.
 // Arm IK realizes its baseline before additive arm and attached-prop offsets.
 // Other appendages remain ahead of support realization. Classify by actual
 // ancestry rather than aliases, so a hand's bound prop follows the same pass.
 auto applySignature=[&](bool afterArms){
 if(signatureBound_&&signature_.active())for(const auto& offset:signature_.offsets()){
  const auto name=QString::fromStdString(offset.joint);
  bool armChannel=false;
  for(auto ancestor=name;!ancestor.isEmpty();ancestor=joints_.at(ancestor).parent)
   if(ancestor=="upper_arm_left"||ancestor=="upper_arm_right"){armChannel=true;break;}
  if(armChannel!=afterArms)continue;
  auto& joint=joints_.at(QString::fromStdString(offset.joint));
  const auto parent=joint.parent.isEmpty()?avatar_:joints_.at(joint.parent).node;
  const auto parentWorld=parent->property("sceneRotation").value<QQuaternion>();
  const auto modelWorld=avatar_->property("sceneRotation").value<QQuaternion>();
  const auto frame=parentWorld.conjugated()*modelWorld;
  const auto current=joint.node->property("rotation").value<QQuaternion>();
  const auto actionRotation=rotation(offset.rotation);
  const auto posed=offset.rotationSpace==pets::character3d::SignatureRotationSpace::JointLocal
   ? current*actionRotation : frame*actionRotation*frame.conjugated()*current;
  joint.node->setProperty("rotation",QVariant::fromValue(posed));
  const auto position=joint.node->property("position").value<QVector3D>();
  joint.node->setProperty("position",QVariant::fromValue(position+frame.rotatedVector(vector(offset.translation))));
 }
 };
 applySignature(false);
 // Parent expressions are applied first; every leg endpoint is then placed
 // from the solved model snapshot, including forelegs below a leaning spine.
 for(std::size_t i=0;i<legRoles_.size();++i){
  const auto& side=legRoles_[i];const auto& l=legs_[i];const auto& posed=p.legs[i];
  place("thigh_"+side,vector(posed.hip));
  aim("thigh_"+side,vector(l.knee)-vector(l.hip),vector(posed.knee)-vector(posed.hip));
  place("shin_"+side,vector(posed.knee));
  aim("shin_"+side,vector(l.ankle)-vector(l.knee),vector(posed.ankle)-vector(posed.knee));
  place("foot_"+side,vector(posed.ankle));
  auto& foot=joints_.at("foot_"+side);
  const auto parentRotation=joints_.at(foot.parent).node->property("sceneRotation").value<QQuaternion>();
  const auto footOrientation=QQuaternion::fromAxisAndAngle(0,1,0,float(p.footYaw[i]*180/3.141592653589793))*
                             QQuaternion::fromAxisAndAngle(1,0,0,float(p.footPitch[i]*180/3.141592653589793));
  const auto basis=avatar_->property("sceneRotation").value<QQuaternion>()*QQuaternion::fromAxisAndAngle(0,1,0,float(-yaw()));
  foot.node->setProperty("rotation",QVariant::fromValue(parentRotation.conjugated()*basis*footOrientation*foot.restWorldRotation));
 }
 if(wings_){
  const auto body=avatar_->property("sceneRotation").value<QQuaternion>();
  int index=0;for(auto side:{QString("left"),QString("right")}){
   const auto wing=p.wings[index];const double sign=index==0?1.:-1.;
   const auto& shoulder=joints_.at("wing_shoulder_"+side);
   const auto& torso=joints_.at(shoulder.parent);
   const auto torsoDelta=body.conjugated()*torso.node->property("sceneRotation").value<QQuaternion>()*torso.restWorldRotation.conjugated();
   // Folded feathers lie along the flank. Abduct, roll the vane flat, then
   // flap around the body forward axis, using the shared realized stroke.
   const auto spread=rotation({0,0,-sign*.32*wing.shoulder})*
       rotation({0,-sign*(1.10*wing.extension+.22*wing.shoulder),0})*
       rotation({0,0,sign*1.5707963267948966*wing.extension});
   const double elbow=wing.elbow*3.141592653589793/1.35;
   const double wrist=elbow+wing.wrist*3.141592653589793/.85;
   for(const auto& joint:std::array<std::pair<QString,double>,3>{{{"wing_shoulder_"+side,0},{"wing_elbow_"+side,elbow},{"wing_wrist_"+side,wrist}}}){
    auto& bound=joints_.at(joint.first);
    const auto parent=joints_.at(bound.parent).node->property("sceneRotation").value<QQuaternion>();
    const auto desired=body*torsoDelta*spread*rotation({joint.second,0,0})*bound.restWorldRotation;
    bound.node->setProperty("rotation",QVariant::fromValue(parent.conjugated()*desired));
   }++index;
  }return;
 }
 if(!hasArms_)return;
 const auto body=avatar_->property("sceneRotation").value<QQuaternion>();
 int sideIndex=0;
 for(auto side:{QString("left"),QString("right")}){
  const auto& rest=arms_[sideIndex];const auto& solved=p.arms[sideIndex];
  if(solved){
   aim("upper_arm_"+side,vector(rest.elbow)-vector(rest.shoulder),vector(solved->elbow)-vector(solved->shoulder));
   aim("forearm_"+side,vector(rest.hand)-vector(rest.elbow),vector(solved->hand)-vector(solved->elbow));
   auto& hand=joints_.at("hand_"+side);
   auto parent=joints_.at(hand.parent).node->property("sceneRotation").value<QQuaternion>();
   hand.node->setProperty("rotation",QVariant::fromValue(parent.conjugated()*body*rotation(p.wrists[sideIndex])*hand.restWorldRotation));
  }
  ++sideIndex;
 }
 applySignature(true);
}
bool WolfMotion::verifyDeformation(){
 if(eyeBlinkSupported()){
  apply();
  std::map<QString,QMatrix4x4> unchanged;
  for(const auto& named:joints_)if(!named.first.startsWith("eye_"))
   unchanged[named.first]=named.second.node->property("sceneTransform").value<QMatrix4x4>();
  setEyeOpenness(0);apply();
  for(auto side:{"left","right"}){
   const auto& eye=joints_.at(QString("eye_")+side);
   if(std::abs(eye.node->property("scale").value<QVector3D>().y()-eye.restScale.y()*.05f)>1e-5f)
    throw std::runtime_error("live eye closure did not apply");
  }
  for(const auto& named:unchanged)if(named.second!=joints_.at(named.first).node->property("sceneTransform").value<QMatrix4x4>())
   throw std::runtime_error("eye closure changed unrelated live anatomy");
  setEyeOpenness(1);apply();
  for(auto side:{"left","right"}){
   const auto& eye=joints_.at(QString("eye_")+side);
   if((eye.node->property("scale").value<QVector3D>()-eye.restScale).length()>1e-5f)
    throw std::runtime_error("live eye reopen did not restore rest scale");
  }
 }

 if(gait_==pets::character::Gait::QuadrupedWalk||(!hasArms_&&!wings_)||(catalogueStyle_&&hasArms_)){
  std::array<std::optional<QVector3D>,4> previousSole;
  std::array<bool,4> previousGrounded{},articulated{};
  bool moved=false;
  auto check=[&]{
   const auto pose=motion_->pose();
   const auto avatarTransform=avatar_->property("sceneTransform").value<QMatrix4x4>();
   int supportCount=0;
   for(std::size_t i=0;i<legRoles_.size();++i){
    const auto& role=legRoles_[i];const auto& rest=legs_[i];const auto& solved=pose.legs[i];
    const auto& foot=joints_.at("foot_"+role);
    const auto offset=foot.restWorldRotation.conjugated().rotatedVector(vector(rest.contact)-vector(rest.ankle));
    const auto actual=foot.node->property("scenePosition").value<QVector3D>()+foot.node->property("sceneRotation").value<QQuaternion>().rotatedVector(offset);
    if((actual-vector(pose.contacts[i])).length()>3e-4f)throw std::runtime_error(("quadruped live contact disagrees with solver: "+role).toStdString());
    if(previousSole[i]&&previousGrounded[i]&&pose.grounded[i]&&(actual-*previousSole[i]).length()>3e-4f)
     throw std::runtime_error("quadruped grounded paw slid before toe-off");
    previousSole[i]=actual;previousGrounded[i]=pose.grounded[i];
    if(actual.y()<=3e-4f)++supportCount;
    const std::array<std::pair<QString,QVector3D>,3> endpoints{{{"thigh_"+role,vector(solved.hip)},{"shin_"+role,vector(solved.knee)},{"foot_"+role,vector(solved.ankle)}}};
    for(const auto& endpoint:endpoints){
     const auto actualPosition=joints_.at(endpoint.first).node->property("scenePosition").value<QVector3D>();
     if((actualPosition-avatarTransform.map(endpoint.second)).length()>3e-4f)throw std::runtime_error("quadruped live chain lost solved endpoint");
    }
    articulated[i]=articulated[i]||std::abs(pose.footPitch[i])>.01;
    moved=moved||pose.swing[i];
   }
   if(supportCount<(legRoles_.size()==4?2:1))throw std::runtime_error("walking lost required foot support");
  };
  check();
  for(double targetX:{60.,-60.,0.}){
   target(targetX);
   for(int frame=0;frame<3000&&!settled();++frame){advance(1./120);check();}
   if(!settled())throw std::runtime_error("quadruped live navigation failed to settle");
  }
  if(!moved)throw std::runtime_error("quadruped live chains never moved");
  for(std::size_t i=0;i<legRoles_.size();++i)if(!articulated[i])throw std::runtime_error("walking foot never articulated");
  for(auto semantic:{QString("look"),QString("wave"),QString("failure")}){
   gesture(semantic);for(int frame=0;frame<900&&!settled();++frame){advance(1./120);check();}
   if(!settled())throw std::runtime_error("quadruped expression failed to settle");
   if(!hasArms_&&(motion_->pose().arms[0]||motion_->pose().arms[1]))throw std::runtime_error("armless expression activated arms");
  }
  return true;
 }
 if(wings_){
  if(!motion_->hop(20))throw std::runtime_error("raven hop unavailable");
  for(int i=0;i<500;++i)advance(1./120);
  if(!motion_->flyTo(40,45))throw std::runtime_error("raven flight unavailable");
  double wingLow=1e9,wingHigh=-1e9;
  bool moved=false;for(int i=0;i<240;++i){
   advance(1./120);moved|=std::abs(motion_->state().wings[0].strokeVelocity)>.1;
   const auto body=avatar_->property("sceneRotation").value<QQuaternion>();
   const auto origin=avatar_->property("scenePosition").value<QVector3D>();
   for(auto side:{QString("left"),QString("right")}){
    const auto local=[&](const QString& role){return body.conjugated().rotatedVector(joints_.at(role+side).node->property("scenePosition").value<QVector3D>()-origin);};
    const auto shoulder=local("wing_shoulder_"),wrist=local("wing_wrist_");
    if(motion_->state().wings[0].extension>.8){wingLow=std::min(wingLow,double(wrist.y()-shoulder.y()));wingHigh=std::max(wingHigh,double(wrist.y()-shoulder.y()));}
    if(motion_->state().wings[0].extension>.8&&wrist.z()>shoulder.z()+.1)
     throw std::runtime_error("extended raven wrist sweeps ahead of shoulder rather than trailing");
   }
  }
  if(wingHigh-wingLow>.45)throw std::runtime_error("raven wing stroke exceeds trailing vertical envelope: "+std::to_string(wingHigh-wingLow));
  motion_->land();for(int i=0;i<1200&&!motion_->settled();++i)advance(1./120);
  if(!moved)throw std::runtime_error("raven wing stroke unchanged");return true;
 }
 auto checkArmClearance=[&](const QString& side) {
  auto body=avatar_->property("sceneRotation").value<QQuaternion>();
  auto origin=avatar_->property("scenePosition").value<QVector3D>();
  const auto torsoOffset=vector(motion_->pose().bodyShift);
  auto local=[&](const QString& joint){return body.conjugated().rotatedVector(joints_.at(joint+"_"+side).node->property("scenePosition").value<QVector3D>()-origin)-torsoOffset;};
  const auto shoulder=local("upper_arm"), elbow=local("forearm"), hand=local("hand");
  const double sign=shoulder.x()>0?1:-1;
  // The authored elbow/forearm radius is .12; lateral room beyond the .36
  // torso half-width prevents the sleeve folding back into the body.
  if(sign*elbow.x()<.48)throw std::runtime_error(("elbow folds into torso clearance "+gesture_+" "+QString::number(gestureTime_)+ " x "+QString::number(elbow.x())).toStdString());
  // Sample the arm's tapered radius against the independently authored
  // cheek taper (.27,1.87)->(.42,1.75), whose radius falls .12->.005.
  const QVector3D cheekStart(sign*.27,shoulder.y()+.29,.035);
  const QVector3D cheekEnd(sign*.42,shoulder.y()+.17,.03);
  const auto cheekAxis=cheekEnd-cheekStart;
  for(int sample=0;sample<=20;++sample) {
   const float fraction=sample/20.f;
   const auto point=shoulder+(elbow-shoulder)*fraction;
   const float closest=std::clamp(QVector3D::dotProduct(point-cheekStart,cheekAxis)/cheekAxis.lengthSquared(),0.f,1.f);
   const auto cheek=cheekStart+cheekAxis*closest;
   const float clearance=.16f-.04f*fraction+.12f-.115f*closest;
   if((point-cheek).length()<clearance)throw std::runtime_error(("raised sleeve intersects cheek tuft "+gesture_+" "+QString::number(gestureTime_, 'f', 4)+" sample "+QString::number(sample)+" distance "+QString::number((point-cheek).length())+" clearance "+QString::number(clearance)).toStdString());
  }
  if((elbow-hand).length()<.25)throw std::runtime_error("forearm collapsed into paw");
 };
 reach(0,{.6f,1.3f,.3f});
 for(int frame=0;frame<360;++frame){advance(1./120);checkArmClearance("left");}
 release(0);for(int frame=0;frame<360;++frame)advance(1./120);
 reach(1,{-.6f,1.3f,.3f});
 for(int frame=0;frame<360;++frame){advance(1./120);checkArmClearance("right");}
 release(1);for(int frame=0;frame<360;++frame)advance(1./120);
 for(auto semantic:{QString("wave"),QString("success"),QString("point"),QString("present")}){
  gesture(semantic);auto previousGrip=leftGrip();
  for(int frame=0;frame<600&&!settled();++frame){
   advance(1./120);
   checkArmClearance("left");
   const auto current=leftGrip();
   if((current-previousGrip).length()>.065f)throw std::runtime_error("procedural grip jumps");
   previousGrip=current;
  }
  if(!settled())throw std::runtime_error("procedural gesture never settles");
 }
 setReaction("wave",.5);
 for(int frame=0;frame<600&&!motion_->settled();++frame){motion_->step(1./120);apply();checkArmClearance("left");}
 const auto held=motion_->pose();
 if(std::abs(held.wrists[0].x-3.141592653589793)>.002)throw std::runtime_error("held wave wrist does not pitch 180 degrees");
 const auto bodyHeld=avatar_->property("sceneRotation").value<QQuaternion>();
 const auto originHeld=avatar_->property("scenePosition").value<QVector3D>();
 if((bodyHeld.conjugated().rotatedVector(leftGrip()-originHeld)-vector(held.arms[0]->grip)).length()>2e-4f)throw std::runtime_error("live wrist loses solved grip");
 setReaction("",1);externalReaction_=false;
 for(int frame=0;frame<600&&!motion_->settled();++frame){motion_->step(1./120);apply();}
 auto before=joints_.at("thigh_left").node->property("rotation").value<QQuaternion>();
 reach(0,{.6f,1.3f,.3f});target(60);pause(false);bool changedTransform=false;
 std::array<bool,2> articulatedPaw{};
 std::array<std::optional<QVector3D>,2> previousLiveSole;
 std::array<bool,2> previousGrounded{};
 bool liveExtensionObserved=false;
 auto checkContacts=[&]{
  auto pose=motion_->pose();int i=0;std::array<bool,2> actualGrounded{};
  for(auto side:{QString("left"),QString("right")}){
   auto& foot=joints_.at("foot_"+side);auto l=legs_[i];
   auto offset=foot.restWorldRotation.conjugated().rotatedVector(vector({l.contact.x-l.ankle.x,l.contact.y-l.ankle.y,l.contact.z-l.ankle.z}));
   auto actual=foot.node->property("scenePosition").value<QVector3D>()+foot.node->property("sceneRotation").value<QQuaternion>().rotatedVector(offset);
   actualGrounded[i]=actual.y()<=2e-4f;
   if(previousLiveSole[i]&&previousGrounded[i]&&pose.grounded[i]&&
      (actual-*previousLiveSole[i]).length()>2e-4f)
    throw std::runtime_error("live grounded paw slides before toe-off");
   previousLiveSole[i]=actual;previousGrounded[i]=pose.grounded[i];
   const auto hip=joints_.at("thigh_"+side).node->property("scenePosition").value<QVector3D>();
   const auto knee=joints_.at("shin_"+side).node->property("scenePosition").value<QVector3D>();
   const auto ankle=foot.node->property("scenePosition").value<QVector3D>();
   const auto& solved=pose.legs[i];
   const float upper=vector({solved.knee.x-solved.hip.x,solved.knee.y-solved.hip.y,solved.knee.z-solved.hip.z}).length();
   const float lower=vector({solved.ankle.x-solved.knee.x,solved.ankle.y-solved.knee.y,solved.ankle.z-solved.knee.z}).length();
   if(std::abs((knee-hip).length()-upper)>2e-4f||std::abs((ankle-knee).length()-lower)>2e-4f)
    throw std::runtime_error("live leg translations do not match elastic IK lengths");
   liveExtensionObserved|=upper>vector({l.knee.x-l.hip.x,l.knee.y-l.hip.y,l.knee.z-l.hip.z}).length()+.001f||
                          lower>vector({l.ankle.x-l.knee.x,l.ankle.y-l.knee.y,l.ankle.z-l.knee.z}).length()+.001f;
   QVector3D expected=vector(pose.contacts[i]);
   auto orientation=QQuaternion::fromAxisAndAngle(0,1,0,float(pose.footYaw[i]*180/3.141592653589793))*
                    QQuaternion::fromAxisAndAngle(1,0,0,float(pose.footPitch[i]*180/3.141592653589793))*foot.restWorldRotation;
   auto actualOrientation=foot.node->property("sceneRotation").value<QQuaternion>();
   if(std::abs(QQuaternion::dotProduct(orientation.normalized(),actualOrientation.normalized()))<.99999f)
    throw std::runtime_error("live paw rotation disagrees with independent ankle pitch");
   articulatedPaw[i]|=std::abs(pose.footPitch[i])>.01;
   if((actual-expected).length()>2e-4)throw std::runtime_error(("3D world contact drift "+side+" "+QString::number((actual-expected).length())+" root "+QString::number(pose.rootX)+" hip "+QString::number(pose.legs[i].hip.y)+" expected "+QString::number(expected.x())+","+QString::number(expected.y())+","+QString::number(expected.z())+" ankle "+QString::number(pose.legs[i].ankle.x)+","+QString::number(pose.legs[i].ankle.y)+","+QString::number(pose.legs[i].ankle.z)).toStdString());
   ++i;
  }
  if(!actualGrounded[0]&&!actualGrounded[1])throw std::runtime_error("both live 3D paws are airborne");
 };
 for(int i=0;i<1500&&!settled();i++){advance(1./60);checkContacts();checkArmClearance("left");changedTransform|=before!=joints_.at("thigh_left").node->property("rotation").value<QQuaternion>();}
 if(!settled())throw std::runtime_error("3D forward motion did not quiesce");
 if(!articulatedPaw[0]||!articulatedPaw[1])throw std::runtime_error("live walking paws never articulate independently");
 target(-60);for(int i=0;i<2000&&!settled();i++){advance(1./60);checkContacts();checkArmClearance("left");}
 if(!settled())throw std::runtime_error("3D reversal motion did not quiesce");
 if(walkingSpeed()>=2&&!liveExtensionObserved)throw std::runtime_error("live high-speed gait never extends its leg translations");
 // A settled static reach needs no motion timer and preserves ground contacts.
 reach(0,{.6f,1.3f,.3f});for(int frame=0;frame<600&&!settled();++frame)advance(1./120);checkContacts();
 auto body=avatar_->property("sceneRotation").value<QQuaternion>();
 auto origin=avatar_->property("scenePosition").value<QVector3D>();
 auto localGrip=body.conjugated().rotatedVector(leftGrip()-origin)-vector(motion_->pose().bodyShift);
 if((localGrip-QVector3D(.6f,1.3f,.3f)).length()>2e-4)throw std::runtime_error("live grip misses target");
 release(0);
 gesture("wave");if(settled())throw std::runtime_error("gesture did not wake");
 for(int j=0;j<600&&!settled();++j){advance(1./60);checkContacts();}
 if(!settled())throw std::runtime_error("gesture did not settle");
 return changedTransform&&jointCount()>=20;
}
