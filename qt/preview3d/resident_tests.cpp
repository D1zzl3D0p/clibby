#include "resident_scene.h"
#include <QGuiApplication>
#include <QTimer>
#include <QFile>
#include <QTemporaryFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <cmath>

static void require(bool condition,const char* message) { if(!condition)throw std::runtime_error(message); }
int main(int argc,char** argv) {
 QGuiApplication app(argc,argv);
 try {
  WolfMotion motion(residentRigPath());QQmlEngine engine;QString error;
  std::unique_ptr<QQuickItem> stage(createResidentStage(engine,motion,&error));
  require(bool(stage),error.toUtf8().constData());
  const double facingBias=stage->property("facingBiasDegrees").toDouble()*3.141592653589793/180;
  require(std::abs(facingBias+35*3.141592653589793/180)<1e-12,"resident 3/4 facing bias changed");
  motion.motion().setFacingBias(facingBias);motion.motion().setCameraFacingTurns(true);motion.motion().reset(0,{-100,100});motion.advancePresentation(0);
  require(motion.jointCount()==22&&motion.eyeBlinkSupported(),"resident binds the deform rig and optional eye joints");
  auto *leftEye=stage->findChild<QObject*>("eye_left");
  auto *head=stage->findChild<QObject*>("head");
  require(leftEye&&head,"live eye and head joints are present");
  const auto openScale=leftEye->property("scale").value<QVector3D>();
  const auto headRotation=head->property("rotation").value<QQuaternion>();
  motion.setEyeOpenness(0);motion.apply();
  const auto closedScale=leftEye->property("scale").value<QVector3D>();
  require(std::abs(closedScale.y()-openScale.y()*.05)<1e-6&&closedScale.x()==openScale.x()
          &&closedScale.z()==openScale.z(),"blink closes actual eye geometry vertically");
  require(head->property("rotation").value<QQuaternion>()==headRotation,"eye closure does not change head motion");
  motion.setEyeOpenness(1);motion.apply();
  require(leftEye->property("scale").value<QVector3D>()==openScale,"eye reopening restores authored geometry");
  require(motion.findChildren<QTimer*>().isEmpty(),"scene binding must not own animation timers");
  require(stage->width()==384&&stage->height()==384,"resident canvas changed");
  const double ppm=stage->property("logicalPixelsPerMeter").toDouble();
  require(ppm==120,"resident calibration changed");
  auto camera=stage->findChild<QObject*>("camera");
  require(camera&&camera->property("eulerRotation").value<QVector3D>().x()==-12,"resident view must retain slightly top-down camera pitch");
  const auto origin=motion.projectAnchor(stage.get(),{});
  const auto floor=stage->property("rootFloorAnchor").toPointF();
  require(std::hypot(origin.x()-floor.x(),origin.y()-floor.y())<.001,"floor anchor disagrees with scene projection");
  const auto travel=motion.projectAnchor(stage.get(),QVector3D(1,0,0));
  require(std::abs(travel.y()-origin.y())<.001&&std::abs(travel.x()-origin.x()-ppm)<.001,"ground travel is not horizontally calibrated");
  const auto farther=motion.projectAnchor(stage.get(),QVector3D(4.25,0,0));
  require(std::abs(farther.y()-origin.y())<.001&&std::abs(farther.x()-origin.x()-4.25*ppm)<.001,"tilted view introduces vertical drift across desktop travel");
  const auto raised=motion.projectAnchor(stage.get(),QVector3D(0,1,0));
  require(std::abs(raised.y()-origin.y()+ppm*std::cos(12*3.141592653589793/180))<.001,"top-down height projection disagrees with calibration");
  require(!stage->findChild<QObject*>("targetMarker")&&!stage->findChild<QObject*>("leftContact"),"resident contains preview markers");
  auto output=stage->findChild<QObject*>("pixelOutput");
  require(output&&!output->property("smooth").toBool()&&output->property("textureSize").toSize()==QSize(128,128),"nearest resident presentation invalid");
  require(stage->setProperty("pixelation",2.0),"pixelation property exists");
  require(output->property("textureSize").toSize()==QSize(64,64),"coarser pixelation lowers render resolution");
  require(stage->width()==384 && motion.projectAnchor(stage.get(),QVector3D(1,0,0))==travel,"pixelation preserves size and projection");
  stage->setProperty("pixelation",.25);
  require(output->property("textureSize").toSize()==QSize(512,512),"fine pixelation increases render resolution");
  stage->setProperty("pixelation",1.0);
  auto grab=motion.projectedGrabRegion(stage.get()), visual=motion.projectedVisualRegion(stage.get());
  stage->setProperty("pixelation",4.0);
  const auto chunkyMask=motion.projectedVisualRegion(stage.get());
  require((visual-chunkyMask).isEmpty() && !(chunkyMask-visual).isEmpty(),
          "coarse pixel blocks need a wider visual fringe");
  stage->setProperty("pixelation",1.0);
  require(!grab.isEmpty()&&!visual.isEmpty(),"resident interaction envelopes missing");
  require(!visual.contains(QPoint(0,0))&&!visual.contains(QPoint(383,383)),"conservative mask covers transparent corners");
  for(auto semantic:{QString("look"),QString("nod"),QString("wave"),QString("success"),QString("failure")}) {
   for(int frame=0;frame<=60;++frame) {
    motion.setReaction(semantic,frame/60.);motion.advance(1./60);
    const auto card=motion.projectAnchor(stage.get(),motion.cardAnchor());
    require(card.x()>0&&card.x()<384&&card.y()>0&&card.y()<384,"gesture card anchor leaves resident surface");
    require(motion.projectedVisualRegion(stage.get()).contains(motion.projectAnchor(stage.get(),motion.leftGrip()).toPoint()),"posed paw missing from mask");
   }
  }
  motion.setReaction({},0);
  for(int frame=0;frame<600&&!motion.settled();++frame)motion.advance(1./60);
  require(motion.settled(),"reaction cancellation fails to settle expression followers");
  motion.setWalkingSpeed(.5);require(motion.settled(),"settled speed change wakes animation");
  // Navigation root must never be applied locally a second time.
  motion.motion().reset(45,{-100,100});motion.advancePresentation(0);
  const auto avatar=stage->findChild<QObject*>("avatar");
  require(avatar&&avatar->property("position").value<QVector3D>().length()<.00001,"absolute root applied twice inside resident window");
  QFile rigFile(residentRigPath());require(rigFile.open(QIODevice::ReadOnly),"rig missing during deformation check");
  const auto rig=QJsonDocument::fromJson(rigFile.readAll()).object();
  auto point=[](QJsonArray a){return QVector3D(a[0].toDouble(),a[2].toDouble(),-a[1].toDouble());};
  const auto basis=QQuaternion::fromAxisAndAngle(0,1,0,90);
  std::array<QObject*,2> feet{};std::array<QVector3D,2> offsets{};
  int sideIndex=0;
  for(auto side:{QString("left"),QString("right")}) {
   feet[sideIndex]=stage->findChild<QObject*>("foot_"+side);
   require(feet[sideIndex],"resident paw missing");
   QVector3D ankle;
   for(auto value:rig["joints"].toArray()) {
    auto joint=value.toObject();if(joint["name"].toString()=="foot_"+side)ankle=point(joint["head"].toArray());
   }
   const auto footBasis=basis*QQuaternion::fromAxisAndAngle(0,1,0,float(motion.motion().pose().footYaw[sideIndex]*180/3.141592653589793));
   offsets[sideIndex]=feet[sideIndex]->property("sceneRotation").value<QQuaternion>().conjugated().rotatedVector(footBasis.rotatedVector(point(rig["contacts"].toObject()[side].toArray())-ankle));
   ++sideIndex;
  }
  motion.setWalkingSpeed(3);
  for(double target:{90.,-90.}) {
   motion.target(target);
   int checks=0;
   for(int frame=0;frame<2500&&!motion.settled();++frame) {
    motion.advance(1./60);const auto pose=motion.motion().pose();
    require(std::cos(3.141592653589793/2+pose.yaw)>0,"resident turns its back to the camera during reversal");
    const auto renderedForward=avatar->property("sceneRotation").value<QQuaternion>().rotatedVector(QVector3D(0,0,1));
    require(renderedForward.z()>0,"live resident skin faces away from the camera during reversal");
    for(int side=0;side<2;++side) {
     const auto actual=feet[side]->property("scenePosition").value<QVector3D>()+feet[side]->property("sceneRotation").value<QQuaternion>().rotatedVector(offsets[side]);
     const auto contact=pose.contacts[side];
     const auto expected=basis.rotatedVector(QVector3D(contact.x,contact.y,contact.z-pose.rootX/100));
     require((actual-expected).length()<.0003,"resident rotated-basis sole disagrees with local world contact");
     const auto actualProjected=motion.projectAnchor(stage.get(),actual), expectedProjected=motion.projectAnchor(stage.get(),expected);
     require(std::hypot(actualProjected.x()-expectedProjected.x(),actualProjected.y()-expectedProjected.y())<.04,"tilted-camera sole projection disagrees with realized contact");
    }
    ++checks;
   }
   require(checks>30&&motion.settled(),"resident travel/reversal did not settle");
   const double heading=motion.motion().pose().yaw;
   const double expectedHeading=target>0?facingBias:-3.141592653589793-facingBias;
   require(std::abs(heading-expectedHeading)<.001,"resident model does not retain camera-facing left/right 3/4 headings");
  }
  {
   QFile ravenRigFile(residentRigPath("raven"));require(ravenRigFile.open(QIODevice::ReadOnly),"raven rig missing");
   auto ravenRig=QJsonDocument::fromJson(ravenRigFile.readAll()).object();
   auto groups=ravenRig["skinBounds"].toArray();require(!groups.isEmpty(),"raven measured skin bounds missing");
   auto group=groups[0].toArray();auto badBox=group[0].toObject();
   badBox["minimum"]=QJsonArray{11.,0.,0.};group[0]=badBox;groups[0]=group;ravenRig["skinBounds"]=groups;
   QTemporaryFile malformedRig;require(malformedRig.open(),"temporary malformed rig unavailable");
   malformedRig.write(QJsonDocument(ravenRig).toJson());malformedRig.flush();
   bool rejected=false;try{WolfMotion invalid(malformedRig.fileName());}catch(const std::exception&){rejected=true;}
   require(rejected,"unbounded measured skin box accepted");
   WolfMotion raven(residentRigPath("raven"));QQmlEngine ravenEngine;QString ravenError;
   std::unique_ptr<QQuickItem> ravenStage(createResidentStage(ravenEngine,raven,&ravenError,"raven"));
   require(bool(ravenStage),ravenError.toUtf8().constData());
   raven.motion().reset(0,{-100,100});raven.advancePresentation(0);
   const double ravenPpm=ravenStage->property("logicalPixelsPerMeter").toDouble();
   const auto restBounds=raven.projectedVisualRegion(ravenStage.get()).boundingRect();
   require(restBounds.height()<2.0*ravenPpm,"raven rest mask includes fictitious circular wing height");
   require(raven.flyTo(0,45),"raven mask flight unavailable");
   int extended=0;
   for(int frame=0;frame<240;++frame){
    raven.advance(1./120);
    if(raven.motion().state().wings[0].extension<.8)continue;
    const auto mask=raven.projectedVisualRegion(ravenStage.get());
    require(!mask.isEmpty(),"extended raven skin mask vanished");
    require(mask.boundingRect().height()<2.3*ravenPpm,"horizontal raven wing has fictitious vertical radius");
    ++extended;
   }
   require(extended>100,"raven mask check did not exercise extended flight");
   std::cout<<"raven measured skin bounds rest height "<<restBounds.height()<<" verified\n";
  }
  std::cout<<"timerless 22-joint resident binding with eye closure, horizontal calibration, projected anchors and masks verified\n";
  return 0;
 }catch(const std::exception& e) { std::cerr<<e.what()<<"\n";return 1; }
}
