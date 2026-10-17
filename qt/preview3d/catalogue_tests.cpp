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
   if(!asset.catalogueStyle||QString(asset.asset)=="marmalade"||(!only.isEmpty()&&only!=QLatin1String(asset.asset)))continue;
   any=true;const auto name=QString::fromLatin1(asset.asset);
   WolfMotion motion(residentRigPath(name));QQmlEngine engine;QString error;
   std::unique_ptr<QQuickItem> stage(createResidentStage(engine,motion,&error,name));check(bool(stage),name+": "+error);
   check(stage->property("logicalPixelsPerMeter").toDouble()==80,"catalogue pixel density changed");
   auto output=stage->findChild<QObject*>("pixelOutput");check(output&&!output->property("smooth").toBool()&&output->property("textureSize").toSize()==QSize(96,96),"catalogue is not nearest96");
   const auto bias=stage->property("facingBiasDegrees").toDouble()*3.141592653589793/180;
   motion.motion().setFacingBias(bias);motion.motion().setCameraFacingTurns(true);motion.motion().reset(0,{-120,120});motion.setWalkingSpeed(1);motion.advancePresentation(0);
   QFile f(residentRigPath(name));check(f.open(QIODevice::ReadOnly),"rig missing");const auto rig=QJsonDocument::fromJson(f.readAll()).object();
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
    check(!pose.arms[0]&&!pose.arms[1],name+": decorative arms acquired reach");
   };
   using pets::character::Role;
   check(motion.motion().expressions().supports(Role::Tail)==bool(stage->findChild<QObject*>("tail_base")),name+": tail capability does not match anatomy");
   check(motion.motion().expressions().supports(Role::EarLeft)==bool(stage->findChild<QObject*>("ear_left")),name+": ear capability does not match anatomy");
   check(!motion.motion().reach(0,{.2,.5,.1}),name+": unsupported reach accepted");
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
   std::cout<<name.toStdString()<<": live anatomy, contacts, walk/reverse, gaze, drop, quiet rest verified\n";
  }
  check(any,"unknown or unsupported catalogue asset");return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
