#include "resident_scene.h"
#include <QGuiApplication>
#include <QQuickWindow>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <QTimer>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
constexpr double pi=3.141592653589793;
const std::array<QString,4> roles{"foreLeft","foreRight","hindLeft","hindRight"};
void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
QVector3D vector(pets::character3d::Vec3 p){return {float(p.x),float(p.y),float(p.z)};}
QVector3D rigPoint(QJsonArray point){return {float(point[0].toDouble()),float(point[2].toDouble()),float(-point[1].toDouble())};}

struct Soles {
 std::array<QObject*,4> feet{};
 std::array<QVector3D,4> offsets{};
 const QQuaternion basis=QQuaternion::fromAxisAndAngle(0,1,0,90);
 Soles(QQuickItem* stage,WolfMotion& motion,const QJsonObject& rig){
  const auto pose=motion.motion().pose();
  for(std::size_t i=0;i<roles.size();++i){
   feet[i]=stage->findChild<QObject*>("foot_"+roles[i]);require(feet[i],"quadruped live foot missing");
   QVector3D ankle;bool found=false;
   for(const auto value:rig["joints"].toArray()){
    const auto joint=value.toObject();if(joint["name"].toString()=="foot_"+roles[i]){ankle=rigPoint(joint["head"].toArray());found=true;}
   }
   require(found,"quadruped authored ankle missing");
   const auto orientation=basis*QQuaternion::fromAxisAndAngle(0,1,0,float(pose.footYaw[i]*180/pi));
   offsets[i]=feet[i]->property("sceneRotation").value<QQuaternion>().conjugated().rotatedVector(
    orientation.rotatedVector(rigPoint(rig["contacts"].toObject()[roles[i]].toArray())-ankle));
  }
 }
 void check(QQuickItem* stage,WolfMotion& motion,bool walking)const{
  const auto pose=motion.motion().pose();require(pose.contacts.size()==4&&pose.legs.size()==4,"quadruped pose omitted active legs");
  int supported=0;
  for(std::size_t i=0;i<roles.size();++i){
   const auto actual=feet[i]->property("scenePosition").value<QVector3D>()+
    feet[i]->property("sceneRotation").value<QQuaternion>().rotatedVector(offsets[i]);
   auto local=vector(pose.contacts[i]);local.setZ(local.z()-float(pose.rootX/100));
   const auto expected=basis.rotatedVector(local);
   require((actual-expected).length()<.0003f,"resident quadruped rendered sole disagrees with solved contact");
   const auto actualPixel=motion.projectAnchor(stage,actual),expectedPixel=motion.projectAnchor(stage,expected);
   require(std::hypot(actualPixel.x()-expectedPixel.x(),actualPixel.y()-expectedPixel.y())<.04,"resident quadruped projected sole drifts from contact");
   if(pose.grounded[i])++supported;
   require(std::isfinite(actual.x())&&std::isfinite(actual.y())&&std::isfinite(actual.z()),"quadruped presented nonfinite sole");
  }
  if(walking)require(supported>=2,"resident quadruped walk lost its declared contact support");
  require(!pose.arms[0]&&!pose.arms[1],"resident quadruped acquired human arm chains");
 }
};

void capture(QGuiApplication& app,QQuickWindow& window,QQuickItem* stage,const QString& directory,const QString& name){
 int swaps=0;const auto connection=QObject::connect(&window,&QQuickWindow::frameSwapped,[&]{++swaps;});
 QMetaObject::invokeMethod(stage,"refresh");window.requestUpdate();
 QElapsedTimer elapsed;elapsed.start();
 while(swaps<2&&elapsed.elapsed()<2000){app.processEvents();QThread::msleep(2);window.requestUpdate();}
 QObject::disconnect(connection);
 const auto image=window.grabWindow();require(!image.isNull(),"native quadruped capture failed");
 require(image.save(directory+"/"+name+".png"),"quadruped capture save failed");
 require(image.scaled(96,96,Qt::IgnoreAspectRatio,Qt::FastTransformation).save(directory+"/"+name+"-native.png"),"native-grid capture save failed");
}
}

int main(int argc,char** argv){
 QGuiApplication app(argc,argv);
 try{
  QString captureDirectory;
  const auto arguments=app.arguments();const int captureIndex=arguments.indexOf("--capture-dir");
  const bool captureAnimation=arguments.contains("--capture-animation");
  if(captureIndex>=0){require(captureIndex+1<arguments.size(),"capture directory argument missing");captureDirectory=arguments[captureIndex+1];require(QDir().mkpath(captureDirectory),"capture directory unavailable");}
  require(!captureAnimation||!captureDirectory.isEmpty(),"animation capture requires --capture-dir");
  // This window is created only for explicit native captures. Offscreen tests
  // validate live transforms and projection without initializing a renderer.
  std::unique_ptr<QQuickWindow> window;
  WolfMotion motion(residentRigPath("marmalade"));QQmlEngine engine;QString error;
  std::unique_ptr<QQuickItem> stage(createResidentStage(engine,motion,&error,"marmalade"));
  require(bool(stage),error.toUtf8().constData());
  require(stage->width()==384&&stage->height()==384,"quadruped resident canvas changed");
  require(stage->property("logicalPixelsPerMeter").toDouble()==80,"quadruped pixel density changed");
  require(std::abs(stage->property("cameraTiltDegrees").toDouble()-15.4)<1e-6,"quadruped camera elevation changed");
  const auto bias=stage->property("facingBiasDegrees").toDouble()*pi/180;
  require(std::abs(bias+35*pi/180)<1e-6,"quadruped facing does not use the authored default angle");
  const auto output=stage->findChild<QObject*>("pixelOutput");
  require(output&&!output->property("smooth").toBool()&&output->property("textureSize").toSize()==QSize(96,96),"quadruped presentation is not nearest 96-pixel output");
  require(motion.findChildren<QTimer*>().isEmpty(),"quadruped binding owns an animation timer");
  require(motion.eyeBlinkSupported(),"quadruped eyes are not bound");
  motion.motion().setFacingBias(bias);motion.motion().setCameraFacingTurns(true);motion.motion().reset(0,{-100,100});motion.advancePresentation(0);
  const auto avatar=stage->findChild<QObject*>("avatar");require(avatar,"quadruped avatar missing");
  require(avatar->property("position").value<QVector3D>().length()<1e-6,"resident quadruped double-applies navigation root");
  const auto eye=stage->findChild<QObject*>("eye_left"),head=stage->findChild<QObject*>("head");require(eye&&head,"quadruped head/eye nodes missing");
  const auto eyeOpen=eye->property("scale").value<QVector3D>();const auto headRest=head->property("rotation").value<QQuaternion>();
  motion.setEyeOpenness(0);motion.apply();
  require(std::abs(eye->property("scale").value<QVector3D>().y()-eyeOpen.y()*.05)<1e-6,"quadruped blink failed");
  require(head->property("rotation").value<QQuaternion>()==headRest,"quadruped blink moves the head");
  motion.setEyeOpenness(1);motion.apply();require(eye->property("scale").value<QVector3D>()==eyeOpen,"quadruped eye reopening failed");
  QFile rigFile(residentRigPath("marmalade"));require(rigFile.open(QIODevice::ReadOnly),"quadruped rig unavailable");
  const auto rig=QJsonDocument::fromJson(rigFile.readAll()).object();require(!rig["skinBounds"].toArray().isEmpty(),"quadruped measured skin bounds missing");
  const Soles soles(stage.get(),motion,rig);soles.check(stage.get(),motion,true);
  const auto floor=stage->property("rootFloorAnchor").toPointF(),origin=motion.projectAnchor(stage.get(),{});
  require(std::hypot(floor.x()-origin.x(),floor.y()-origin.y())<.001,"quadruped floor anchor is not calibrated");
  const auto visual=motion.projectedVisualRegion(stage.get());require(!visual.isEmpty()&&!visual.contains({0,0})&&!visual.contains({383,383}),"quadruped measured mask missing or oversized");
  require(!motion.projectedGrabRegion(stage.get()).isEmpty(),"quadruped grab region missing");
  if(!captureDirectory.isEmpty()){
   window=std::make_unique<QQuickWindow>();window->setColor(Qt::transparent);window->resize(384,384);stage->setParentItem(window->contentItem());window->show();
   capture(app,*window,stage.get(),captureDirectory,"rest");
  }
  motion.setWalkingSpeed(1.5);
  for(double destination:{65.,-65.}){
   motion.target(destination);int checked=0;
   for(int frame=0;frame<3000&&!motion.settled();++frame){
    motion.advance(1./120);soles.check(stage.get(),motion,true);
    const auto forward=avatar->property("sceneRotation").value<QQuaternion>().rotatedVector({0,0,1});require(forward.z()>0,"quadruped resident turns away from camera");
    if(frame==24&&window)capture(app,*window,stage.get(),captureDirectory,destination>0?"walk":"turn");
    ++checked;
   }
   require(checked>30&&motion.settled(),"quadruped resident travel failed to settle");
   const auto expected=destination>0?bias:-pi-bias;
   require(std::abs(motion.motion().pose().yaw-expected)<.001,"quadruped resident heading calibration changed");
  }
  for(const auto semantic:{QString("look"),QString("wave"),QString("failure")}){
   motion.gesture(semantic);
   for(int frame=0;frame<900&&!motion.settled();++frame){motion.advance(1./120);soles.check(stage.get(),motion,true);}
   require(motion.settled(),"quadruped semantic expression failed to settle");
  }
  motion.roleGoal("tail",{.12f,.15f,.15f});motion.roleGoal("earLeft",{.08f,0,.1f});motion.look({.3f,.1f,1});
  for(int frame=0;frame<900&&!motion.settled();++frame){motion.advance(1./120);soles.check(stage.get(),motion,true);}
  require(motion.settled(),"quadruped role expressions failed to settle");
  if(window)capture(app,*window,stage.get(),captureDirectory,"expression");
  motion.clearExpression();for(int frame=0;frame<900&&!motion.settled();++frame)motion.advance(1./120);
  motion.motion().pickup(10,45);motion.advancePresentation(0);soles.check(stage.get(),motion,false);
  motion.motion().hold(20,50);motion.advancePresentation(0);soles.check(stage.get(),motion,false);
  if(window)capture(app,*window,stage.get(),captureDirectory,"held");
  motion.motion().release({15,0});bool airborne=false;
  for(int frame=0;frame<3000&&!motion.settled();++frame){
   motion.advance(1./120);soles.check(stage.get(),motion,false);
   airborne|=motion.motion().state().phase==pets::character::DropPhase::Airborne;
   if(frame==20&&window)capture(app,*window,stage.get(),captureDirectory,"drop");
  }
  require(airborne&&motion.settled()&&motion.motion().state().phase==pets::character::DropPhase::Grounded,"quadruped drop failed to land and settle");
  soles.check(stage.get(),motion,true);
  const auto rest=motion.motion().pose();const auto restHead=head->property("rotation").value<QQuaternion>();
  for(int frame=0;frame<120;++frame)motion.advance(1./120);
  const auto afterRest=motion.motion().pose();
  const auto afterHead=head->property("rotation").value<QQuaternion>();
  const auto headResidual=std::abs(afterHead.scalar()-restHead.scalar())+std::abs(afterHead.x()-restHead.x())+
   std::abs(afterHead.y()-restHead.y())+std::abs(afterHead.z()-restHead.z());
  // Live float transforms can renormalize a quaternion at machine precision.
  require(motion.settled()&&afterRest.rootX==rest.rootX&&headResidual<1e-5,"quadruped quiet rest keeps moving");
  if(window){
   capture(app,*window,stage.get(),captureDirectory,"landed");
   if(captureAnimation){
    const auto directory=captureDirectory+"/animation";require(QDir().mkpath(directory),"animation directory unavailable");
    motion.motion().reset(0,{-100,100});motion.setWalkingSpeed(1.5);motion.advancePresentation(0);
    QJsonArray frames;
    for(int frame=0;frame<120;++frame){
     if(frame==12)motion.target(55);
     if(frame==50)motion.target(-40);
     for(int substep=0;substep<8;++substep){motion.advance(1./120);soles.check(stage.get(),motion,true);}
     capture(app,*window,stage.get(),directory,QString::number(frame).rightJustified(4,'0'));
     const auto pose=motion.motion().pose();
     frames.append(QJsonObject{{"frame",frame},{"time",frame/15.},{"rootX",pose.rootX},{"yaw",pose.yaw},{"settled",motion.settled()}});
    }
    QFile metadata(directory+"/capture.json");require(metadata.open(QIODevice::WriteOnly),"animation metadata unavailable");
    metadata.write(QJsonDocument(QJsonObject{{"fps",15},{"frameCount",120},{"durationSeconds",8},{"nativeGrid",96},{"windowSize",384},{"renderer","live Qt Quick 3D resident stage"},{"frames",frames}}).toJson());
   }
   stage->setParentItem(nullptr);
  }
  std::cout<<"resident quadruped: four live soles, calibrated 96-pixel stage, blink, reversal, expressions, pickup/drop and quiet rest verified\n";return 0;
 }catch(const std::exception& error){std::cerr<<error.what()<<"\n";return 1;}
}
