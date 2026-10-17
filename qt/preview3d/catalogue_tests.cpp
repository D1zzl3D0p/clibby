#include "resident_scene.h"
#include "characters3d/builtin_assets.h"
#include <QGuiApplication>
#include <QQuickWindow>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QElapsedTimer>
#include <QThread>
#include <QMatrix4x4>
#include <iostream>
#include <memory>
#include <cmath>
#include <stdexcept>

static void check(bool condition,const QString& message){if(!condition)throw std::runtime_error(message.toStdString());}
static QVector3D point(QJsonArray p){return {float(p[0].toDouble()),float(p[2].toDouble()),float(-p[1].toDouble())};}
static QVector3D vector(pets::motion::Vec3 p){return {float(p.x),float(p.y),float(p.z)};}
static void capture(QGuiApplication& app,QQuickWindow& window,QQuickItem* stage,QString path){
 int swaps=0;auto c=QObject::connect(&window,&QQuickWindow::frameSwapped,[&]{++swaps;});
 QMetaObject::invokeMethod(stage,"refresh");window.requestUpdate();QElapsedTimer elapsed;elapsed.start();
 while(swaps<2&&elapsed.elapsed()<2000){app.processEvents();QThread::msleep(2);window.requestUpdate();}
 QObject::disconnect(c);auto image=window.grabWindow();check(!image.isNull(),"native catalogue framebuffer unavailable");
 check(image.save(path+".png"),"capture save failed");
 check(image.scaled(96,96,Qt::IgnoreAspectRatio,Qt::FastTransformation).save(path+"-native.png"),"native capture save failed");
}
int main(int argc,char** argv){
 QQuickWindow::setDefaultAlphaBuffer(true);QGuiApplication app(argc,argv);
 try{
  const auto args=app.arguments();QString only,out;int index=args.indexOf("--asset");if(index>=0){check(index+1<args.size(),"missing asset");only=args[index+1];}
  index=args.indexOf("--capture-dir");if(index>=0){check(index+1<args.size(),"missing capture directory");out=args[index+1];check(QDir().mkpath(out),"capture directory unavailable");}
  bool any=false;
  for(const auto& asset:pets::character3d::builtinAssets){
   if(!asset.catalogueStyle||(!only.isEmpty()&&only!=QLatin1String(asset.asset)))continue;
   any=true;const auto name=QString::fromLatin1(asset.asset);
   WolfMotion motion(residentRigPath(name));QQmlEngine engine;QString error;
   std::unique_ptr<QQuickItem> stage(createResidentStage(engine,motion,&error,name));check(bool(stage),name+": "+error);
   check(stage->property("logicalPixelsPerMeter").toDouble()==80,"catalogue pixel density changed");
   auto output=stage->findChild<QObject*>("pixelOutput");check(output&&!output->property("smooth").toBool()&&output->property("textureSize").toSize()==QSize(96,96),"catalogue is not nearest96");
   const auto bias=stage->property("facingBiasDegrees").toDouble()*3.141592653589793/180;
   motion.motion().setFacingBias(bias);motion.motion().setCameraFacingTurns(true);motion.motion().reset(0,{-120,120});motion.setWalkingSpeed(1);motion.advancePresentation(0);
   QFile f(residentRigPath(name));check(f.open(QIODevice::ReadOnly),"rig missing");const auto rig=QJsonDocument::fromJson(f.readAll()).object();
   const bool hasArms=rig.value("arms").toBool(false);
   check(motion.reachSupported()==hasArms,name+": advertised reach differs from authored arms");
   for(const auto* side:{"left","right"})for(const auto* part:{"upper_arm_","forearm_","hand_"})
    if(hasArms)check(stage->findChild<QObject*>(QString::fromLatin1(part)+QLatin1String(side)),name+": live articulated arm joint missing");
   struct Attachment {QObject* node;QObject* parent;QVector3D localPivot;};
   std::vector<Attachment> attachments;
   for(const auto value:rig["joints"].toArray()){
    const auto joint=value.toObject();const auto jointName=joint["name"].toString();const auto parentName=joint["parent"].toString();
    if(jointName!="held_prop"&&jointName!="fastening"&&parentName!="held_prop")continue;
    auto node=stage->findChild<QObject*>(jointName);auto parent=stage->findChild<QObject*>(parentName);
    check(node&&parent&&node->parent()==parent,name+": owned prop is outside its authored parent hierarchy");
    if(jointName=="held_prop")check(parentName=="hand_left",name+": held prop is not attached to its real hand");
    if(jointName=="fastening")check(parentName=="spine",name+": fastening is not attached to torso");
    bool invertible=false;const auto inverse=parent->property("sceneTransform").value<QMatrix4x4>().inverted(&invertible);
    check(invertible,name+": attachment parent transform is singular");
    attachments.push_back({node,parent,inverse.map(node->property("scenePosition").value<QVector3D>())});
   }
   if(hasArms)check(!attachments.empty(),name+": humanoid signature has no actual owned prop binding");
   std::vector<QString> roles=asset.quadruped?std::vector<QString>{"foreLeft","foreRight","hindLeft","hindRight"}:std::vector<QString>{"left","right"};
   std::vector<QObject*> feet;std::vector<QVector3D> offsets;const auto basis=QQuaternion::fromAxisAndAngle(0,1,0,90);
   for(std::size_t i=0;i<roles.size();++i){
    auto foot=stage->findChild<QObject*>("foot_"+roles[i]);check(foot,"missing live foot");feet.push_back(foot);QVector3D ankle;
    for(const auto j:rig["joints"].toArray())if(j.toObject()["name"].toString()=="foot_"+roles[i])ankle=point(j.toObject()["head"].toArray());
    const auto orientation=basis*QQuaternion::fromAxisAndAngle(0,1,0,float(motion.motion().pose().footYaw[i]*180/3.141592653589793));
    offsets.push_back(foot->property("sceneRotation").value<QQuaternion>().conjugated().rotatedVector(orientation.rotatedVector(point(rig["contacts"].toObject()[roles[i]].toArray())-ankle)));
   }
   auto inspect=[&](bool walking){
    auto pose=motion.motion().pose();check(pose.legs.size()==roles.size(),name+": active leg count");int supported=0;
    for(std::size_t i=0;i<roles.size();++i){
     auto actual=feet[i]->property("scenePosition").value<QVector3D>()+feet[i]->property("sceneRotation").value<QQuaternion>().rotatedVector(offsets[i]);
     auto local=vector(pose.contacts[i]);local.setZ(local.z()-float(pose.rootX/100));auto expected=basis.rotatedVector(local);
     check((actual-expected).length()<.0004,name+": rendered sole differs from actual contact");supported+=pose.grounded[i];
    }
    if(walking)check(supported>=int(roles.size())-1,name+": insufficient stance support");
    check(bool(pose.arms[0])==hasArms&&bool(pose.arms[1])==hasArms,name+": solved arm anatomy differs from rig capability");
    for(const auto& attachment:attachments){
     bool invertible=false;const auto inverse=attachment.parent->property("sceneTransform").value<QMatrix4x4>().inverted(&invertible);
     check(invertible&&(inverse.map(attachment.node->property("scenePosition").value<QVector3D>())-attachment.localPivot).length()<.0001,
       name+": owned prop pivot detached during movement");
    }
   };
   using pets::character::Role;
   check(motion.motion().expressions().supports(Role::Tail)==bool(stage->findChild<QObject*>("tail_base")),name+": tail capability does not match anatomy");
   check(motion.motion().expressions().supports(Role::EarLeft)==bool(stage->findChild<QObject*>("ear_left")),name+": ear capability does not match anatomy");
   check(motion.motion().reach(0,{.2,.5,.1})==hasArms,name+": reach admission differs from actual anatomy");
   motion.motion().clearArmGoal(0);motion.motion().reset(0,{-120,120});motion.advancePresentation(0);
   check(!motion.motion().flightCapable(),name+": decorative wings claim powered flight");
   inspect(true);check(!motion.projectedVisualRegion(stage.get()).isEmpty(),name+": visual mask missing");
   std::unique_ptr<QQuickWindow> window;
   if(!out.isEmpty()){window=std::make_unique<QQuickWindow>();window->setColor(Qt::transparent);window->resize(384,384);stage->setParentItem(window->contentItem());window->show();capture(app,*window,stage.get(),out+"/"+name+"-rest");}
   for(double target:{55.,-55.}){
    motion.target(target);int frames=0;
    for(;frames<6000&&!motion.settled();++frames){motion.advance(1./120);inspect(true);if(frames==25&&window)capture(app,*window,stage.get(),out+"/"+name+(target>0?"-walk":"-turn"));}
    check(motion.settled()&&frames>10,name+": navigation never settles");
   }
   motion.look({.2f,.1f,1});for(int i=0;i<900&&!motion.settled();++i){motion.advance(1./120);inspect(true);}check(motion.settled(),name+": held gaze never settles");
   motion.clearExpression();for(int i=0;i<900&&!motion.settled();++i)motion.advance(1./120);
   motion.motion().pickup(0,45);motion.advancePresentation(0);inspect(false);motion.motion().release({0,0});
   for(int i=0;i<6000&&!motion.settled();++i){motion.advance(1./120);inspect(false);}check(motion.settled(),name+": drop never settles");
   const auto quiet=motion.motion().pose();for(int i=0;i<30;++i)motion.advance(1./120);check(motion.settled()&&motion.motion().pose().rootX==quiet.rootX,name+": quiet rest moves");
   check(motion.signatureSupported()&&!motion.signatureName().isEmpty(),name+": missing signature binding");
   auto actionPose=[&]{
    std::vector<QMatrix4x4> transforms;
    pets::character3d::SignatureMotion definition(asset.asset);
    for(auto joint:definition.requiredJoints()){
     auto node=stage->findChild<QObject*>(QString::fromUtf8(joint.data(),int(joint.size())));check(node,name+": missing action joint");
     transforms.push_back(node->property("sceneTransform").value<QMatrix4x4>());
    }return transforms;
   };
   // Isolate the signature from residual landing/turn followers tested above.
   motion.motion().reset(0,{-120,120});
   for(int i=0;i<600;++i){motion.advance(1./120);inspect(true);}
   const auto resting=actionPose();
   // A global transform change can come solely from an expressive ancestor.
   // Record the arm's own local rotation to prove its additive action survives IK.
   std::vector<std::pair<QObject*,QQuaternion>> restingActionArms;
   if(hasArms){
    pets::character3d::SignatureMotion definition(asset.asset);
    for(const auto required:definition.requiredJoints()){
     const auto jointName=QString::fromUtf8(required.data(),int(required.size()));
     if(!jointName.startsWith("upper_arm_")&&!jointName.startsWith("forearm_"))continue;
     auto node=stage->findChild<QObject*>(jointName);check(node,name+": missing signature arm joint");
     restingActionArms.push_back({node,node->property("rotation").value<QQuaternion>()});
    }
    check(restingActionArms.size()>=2,name+": humanoid signature lacks upper and lower arm channels");
   }
   check(motion.signature()&&!motion.signature(),name+": signature admission failed");
   check(!motion.settled(),name+": action failed to wake");
   for(int i=0;i<180;++i){motion.advance(1./120);inspect(true);}
   const auto expressive=actionPose();check(expressive!=resting,name+": signature does not deform actual joints");
   for(const auto& arm:restingActionArms){
    const auto current=arm.first->property("rotation").value<QQuaternion>().normalized();
    const auto alignment=std::fmin(1.,std::abs(double(QQuaternion::dotProduct(arm.second.normalized(),current))));
    check(2*std::acos(alignment)>.0087,name+": signature arm offset was overwritten by IK: "+arm.first->objectName());
   }
   motion.pause(true);for(int i=0;i<90;++i)motion.advance(1./120);check(actionPose()==expressive&&motion.signatureActive(),name+": pause did not freeze action");motion.pause(false);
   for(int i=0;i<1200&&!motion.settled();++i){motion.advance(1./120);inspect(true);}
   check(motion.settled()&&!motion.signatureActive(),name+": signature never settles");
   const auto finished=actionPose();
   // Quick3D retains tiny near-identity quaternion residuals in node setters.
   // Core channels reach exact zero; live matrices allow <1 mm / .06 degrees.
   for(std::size_t i=0;i<resting.size();++i)for(int n=0;n<16;++n)
    check(std::abs(resting[i].constData()[n]-finished[i].constData()[n])<.001,name+": signature did not restore rest joint "+QString::number(i)+" element "+QString::number(n)+" before "+QString::number(resting[i].constData()[n], 'g', 9)+" after "+QString::number(finished[i].constData()[n], 'g', 9));
   check(motion.signature(),name+": signature did not restart");for(int i=0;i<150;++i)motion.advance(1./120);
   motion.target(25);for(int i=0;i<60;++i){motion.advance(1./120);inspect(true);}check(!motion.signatureActive(),name+": walking did not cancel action");
   for(int i=0;i<6000&&!motion.settled();++i)motion.advance(1./120);check(motion.settled(),name+": cancellation did not settle");
   if(window&&args.contains("--capture-signature")){
    motion.motion().reset(0,{-120,120});motion.clearExpression();for(int i=0;i<900&&!motion.settled();++i)motion.advance(1./120);
    const auto directory=out+"/"+name+"-signature";check(QDir().mkpath(directory),"signature directory unavailable");
    check(motion.signature(),name+": capture action refused");
    for(int frame=0;frame<105;++frame){
     for(int step=0;step<8;++step){motion.advance(1./120);inspect(true);}
     capture(app,*window,stage.get(),directory+"/"+QString::number(frame).rightJustified(4,'0'));
    }
    check(motion.settled(),name+": captured signature not quiet");
   }
   if(window)stage->setParentItem(nullptr);
   if(window&&args.contains("--capture-animation")){
    stage->setParentItem(window->contentItem());const auto directory=out+"/"+name;check(QDir().mkpath(directory),"animation directory unavailable");
    motion.motion().reset(0,{-120,120});motion.advancePresentation(0);
    for(int frame=0;frame<60;++frame){
     if(frame==8)motion.target(35);if(frame==30)motion.target(-20);
     for(int step=0;step<8;++step){motion.advance(1./120);inspect(true);}
     capture(app,*window,stage.get(),directory+"/"+QString::number(frame).rightJustified(4,'0'));
    }
    stage->setParentItem(nullptr);
   }
   std::cout<<name.toStdString()<<": live anatomy, contacts, walk/reverse, gaze, drop, signature, pause/cancel and quiet rest verified\n";
  }
  check(any,"unknown or unsupported catalogue asset");return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
