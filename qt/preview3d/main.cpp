#include "controller.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QCommandLineParser>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QImage>
#include <QDir>
#include <QTimer>
#include <iostream>
#include <stdexcept>
#include <cmath>
int main(int argc,char**argv){
 QQuickWindow::setDefaultAlphaBuffer(true);
 QGuiApplication app(argc,argv);QCommandLineParser parser;parser.addHelpOption();
 parser.addOption({"character","Built-in character: werewolf, raven or marmalade.","name","werewolf"});
 parser.addOption({"hop","Procedural hop height.","height"});
 parser.addOption({"fly","Flight target x,elevation.","point"});
 parser.addOption({"land-at","Begin landing after seconds.","seconds"});
 parser.addOption({"autonomous","Enable calm curious activities."});
 parser.addOption({"seed","Reproducible activity seed.","integer","1"});
 parser.addOption({"verify-joints","Verify all deform joints and changed transforms without initializing graphics."});
 parser.addOption({"verify-presentation","Verify GPU composition and nearest presentation properties without drawing."});
 parser.addOption({"transparent","Capture with transparent background and without labels."});
 parser.addOption({"capture","Save a native Qt GPU render PNG.","png"});
 parser.addOption({"duration","Capture/exit after seconds.","seconds","2"});
 parser.addOption({"speed","Body travel speed setting (0.5–3); feet follow.","multiplier","2"});
 parser.addOption({"target","Ground target in preview units.","x","0"});
 parser.addOption({"hide-after","Hide and suspend motion after seconds.","seconds"});
 parser.addOption({"reversal","Reverse target halfway through capture."});
 parser.addOption({"impulse","Apply bounded impulse."});parser.process(app);
 try {
  QString rigPath=QStringLiteral(WOLF_RIG_PATH),qmlDir=QStringLiteral(WOLF_QML_DIR),assetType="Werewolf";
  if(parser.value("character")=="raven") {
#ifdef RAVEN_RIG_PATH
   rigPath=QStringLiteral(RAVEN_RIG_PATH);qmlDir=QStringLiteral(RAVEN_QML_DIR);assetType="Raven";
#else
   throw std::runtime_error("Raven 3D asset unavailable");
#endif
  } else if(parser.value("character")=="marmalade") {
#ifdef MARMALADE_RIG_PATH
   rigPath=QStringLiteral(MARMALADE_RIG_PATH);qmlDir=QStringLiteral(MARMALADE_QML_DIR);assetType="Marmalade";
#else
   throw std::runtime_error("Marmalade 3D asset unavailable");
#endif
  } else if(parser.value("character")!="werewolf")throw std::runtime_error("Unknown 3D character");
  WolfMotion motion(rigPath);QQmlApplicationEngine engine;
  bool speedOkay=false;double speed=parser.value("speed").toDouble(&speedOkay);
  if(!speedOkay)throw std::runtime_error("invalid walking speed");
  motion.setWalkingSpeed(speed);
  bool seedOkay=false;auto seed=parser.value("seed").toUInt(&seedOkay);if(!seedOkay)throw std::runtime_error("invalid activity seed");motion.setSeed(seed);
  engine.addImportPath(qmlDir);
  // Generated assets are a trusted local module, never downloaded at runtime.
  engine.rootContext()->setContextProperty("motion",&motion);
  engine.rootContext()->setContextProperty("ravenModel",parser.value("character")=="raven");
  engine.rootContext()->setContextProperty("marmaladeModel",parser.value("character")=="marmalade");
  engine.rootContext()->setContextProperty("captureTransparent",parser.isSet("transparent"));
  engine.rootContext()->setContextProperty("previewVisible",!parser.isSet("verify-presentation"));
  if(parser.isSet("verify-joints")){
   // A dormant preview must start a new quiet interval when toggled later.
   double testNow=100;
   WolfMotion clockTest(rigPath,nullptr,[&]{return testNow;});
   clockTest.setAutonomous(true);clockTest.setAutonomous(false);
   testNow=200;clockTest.setAutonomous(true);
   if(!clockTest.nextBehaviorDeadline()||*clockTest.nextBehaviorDeadline()<208)throw std::runtime_error("delayed autonomy enable skips fresh quiet interval");
   testNow=300;clockTest.setSeed(7);
   if(!clockTest.nextBehaviorDeadline()||*clockTest.nextBehaviorDeadline()<308)throw std::runtime_error("delayed seed change skips fresh quiet interval");
   QQmlComponent component(&engine);
   QByteArray source="import QtQuick\nimport QtQuick3D\nimport \"";
   source+=QUrl::fromLocalFile(qmlDir).toString().toUtf8();
   source+="\"\nNode { Node { objectName: \"avatar\"; position.z: motion.rootOffset; eulerRotation.y: motion.yaw; "+assetType.toUtf8()+" {} } }";
   component.setData(source,QUrl::fromLocalFile(QStringLiteral(WOLF_STAGE_PATH)));
   std::unique_ptr<QObject> scene(component.create());if(!scene){std::cerr<<component.errorString().toStdString();return 1;}
   motion.bind(scene.get());if(!motion.verifyDeformation())throw std::runtime_error("deform transforms did not change");
   std::cout<<motion.jointCount()<<" live deform joints addressable; procedural transforms changed\n";return 0;
  }
  if(qEnvironmentVariable("QT_QUICK_BACKEND")=="software"){
   std::cerr<<"GPU unavailable: software Qt Quick cannot render View3D\n";return 77;
  }
  QFile stage(QStringLiteral(WOLF_STAGE_PATH));if(!stage.open(QIODevice::ReadOnly))throw std::runtime_error("3D stage missing");
  auto source=stage.readAll();if(assetType!="Werewolf")source.replace("Werewolf {",assetType.toUtf8()+" {");source.replace("import WolfAsset",QByteArray("import \"")+QUrl::fromLocalFile(qmlDir).toString().toUtf8()+"\"");
  engine.loadData(source,QUrl::fromLocalFile(QStringLiteral(WOLF_STAGE_PATH)));
  if(engine.rootObjects().isEmpty())throw std::runtime_error("3D stage QML failed");
  auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());if(!window)throw std::runtime_error("3D stage not window");
  motion.bind(window);
  if(parser.isSet("verify-presentation")){
   auto camera=window->findChild<QObject*>("camera");if(!camera||camera->property("clipNear").toDouble()>=1||camera->property("verticalMagnification").toDouble()<20)throw std::runtime_error("meter-scale camera frustum invalid");
   auto view=window->findChild<QObject*>("view3d");auto output=window->findChild<QObject*>("pixelOutput");
   if(!view||!output||view->property("width").toInt()!=192||view->property("height").toInt()!=128||view->property("explicitTextureWidth").toInt()!=192||view->property("explicitTextureHeight").toInt()!=128||output->property("smooth").toBool()||!output->property("hideSource").toBool()||output->property("textureSize").toSize()!=QSize(192,128))throw std::runtime_error("3D nearest presentation properties invalid");
   auto projectTarget=[&](double units){ QVariant projected; QMetaObject::invokeMethod(window, "screenXForTarget", Q_RETURN_ARG(QVariant,projected), Q_ARG(QVariant,QVariant(units))); QVariant actual; QMetaObject::invokeMethod(window,"targetAt",Q_RETURN_ARG(QVariant,actual),Q_ARG(QVariant,projected)); if(!actual.isValid()||std::abs(actual.toDouble()-units)>1e-4)throw std::runtime_error("screen target projection does not follow ground axis"); };
   projectTarget(-75);projectTarget(0);projectTarget(75);
   if(!window->findChild<QObject*>("targetMarker")||!window->findChild<QObject*>("leftContact")||!window->findChild<QObject*>("rightContact"))throw std::runtime_error("ground/contact inspection markers missing");
   auto speedControl=window->findChild<QObject*>("walkingSpeed");
   if(!speedControl || speedControl->property("from").toDouble()!=.5 || speedControl->property("to").toDouble()!=3 || motion.property("walkingSpeed").toDouble()!=2)
    throw std::runtime_error("walking speed control missing bounds or 2x default");
   motion.setProperty("walkingSpeed",.5);
   if(motion.property("walkingSpeed").toDouble()!=.5 || !motion.settled())throw std::runtime_error("speed control does not adjust quietly");
   motion.setProperty("walkingSpeed",1.);
   if(motion.property("walkingSpeed").toDouble()!=1.)throw std::runtime_error("stride control cannot select short steps");
   std::cout<<"192x128 source/output texture, nearest 4x presentation, transparent 3D environment configured\n";return 0;
  }
  bool okay=false;double target=parser.value("target").toDouble(&okay);if(!okay||!std::isfinite(target))throw std::runtime_error("invalid target");
  QTimer previewTimer;previewTimer.setSingleShot(true);
  auto schedule=[&]{
   previewTimer.stop();if(motion.paused())return;
   double delay=motion.settled()?1e9:1./60;
   if(auto deadline=motion.nextBehaviorDeadline())delay=std::min(delay,std::max(.001,*deadline-motion.behaviorTime()));
   if(delay<1e8)previewTimer.start(std::max(1,int(std::ceil(delay*1000))));
  };
  QObject::connect(&previewTimer,&QTimer::timeout,&motion,[&]{
   motion.advanceBehavior(motion.behaviorTime());
   if(!motion.settled())motion.advance(1./60);
   schedule();
  });
  QObject::connect(&motion,&WolfMotion::changed,&previewTimer,schedule);
  motion.target(target);if(parser.isSet("impulse"))motion.impulse();
  motion.setAutonomous(parser.isSet("autonomous"));
  if(parser.isSet("hop"))motion.hop(parser.value("hop").toDouble());
  if(parser.isSet("fly")){auto point=parser.value("fly").split(',');if(point.size()!=2)throw std::runtime_error("invalid flight point");motion.flyTo(point[0].toDouble(),point[1].toDouble());}
  if(parser.isSet("land-at")&&!parser.isSet("capture"))QTimer::singleShot(int(parser.value("land-at").toDouble()*1000),&motion,[&]{motion.land();});schedule();
  bool durationOkay=false;double duration=parser.value("duration").toDouble(&durationOkay);if(!durationOkay||!std::isfinite(duration)||duration<=0||duration>120)throw std::runtime_error("invalid duration");
  if(parser.isSet("reversal")&&!parser.isSet("capture"))QTimer::singleShot(int(duration*500),&motion,[&motion,target]{motion.target(-target);});
  if(parser.isSet("hide-after")){
   bool valid=false;double seconds=parser.value("hide-after").toDouble(&valid);if(!valid||!std::isfinite(seconds)||seconds<0||seconds>120)throw std::runtime_error("invalid hide time");
   QTimer::singleShot(int(seconds*1000),window,[&]{window->hide();motion.pause(true);previewTimer.stop();});
  }
  if(parser.isSet("duration")&&!parser.isSet("capture"))QTimer::singleShot(int(duration*1000),&app,&QCoreApplication::quit);
  if(parser.isSet("capture")) {
   previewTimer.stop();
   const double behaviorStart=motion.behaviorTime();
   bool landed=false,reversed=false;
   const int steps=int(std::ceil(duration*120));
   for(int step=0;step<steps;++step) {
    const double elapsed=step/120.;
    if(parser.isSet("land-at") && !landed && elapsed>=parser.value("land-at").toDouble()){motion.land();landed=true;}
    if(parser.isSet("reversal") && !reversed && elapsed>=duration*.5){motion.target(-target);reversed=true;}
    motion.advanceBehavior(behaviorStart+elapsed);motion.advance(1./120);
   }
   motion.pause(true);previewTimer.stop();
  }
  if(parser.isSet("capture"))QTimer::singleShot(250,&app,[&]{
   auto api=window->rendererInterface()->graphicsApi();
   if(!QSGRendererInterface::isApiRhiBased(api)){std::cerr<<"GPU unavailable: Qt platform selected a non-RHI graphics backend\n";app.exit(77);return;}
   auto image=window->grabWindow();if(image.isNull()){std::cerr<<"GPU unavailable: no native framebuffer\n";app.exit(77);return;}
   if(!image.save(parser.value("capture"))){std::cerr<<"capture save failed\n";app.exit(1);return;}
   QJsonObject meta{{"seed",int(motion.seed())},{"autonomous",motion.autonomous()},{"live_3d",true},{"cpu_readback_per_frame",false},{"capture_readback",true},{"render_width",192},{"render_height",128},{"joint_count",motion.jointCount()},{"walking_speed",motion.walkingSpeed()},{"root_x",motion.rootOffset()*100},{"simulation_time",duration},{"elevation",motion.motion().state().elevation},{"powered_flight",motion.motion().state().poweredFlight},{"phase",int(motion.motion().state().phase)},{"settled",motion.settled()},{"graphics_api",int(window->rendererInterface()->graphicsApi())},{"nearest",true},{"transparent_3d_pass",true},{"transparent_capture",parser.isSet("transparent")},{"native_scaling_verified",false}};
   QFile f(parser.value("capture")+".json");if(!f.open(QIODevice::WriteOnly)){app.exit(1);return;}f.write(QJsonDocument(meta).toJson());app.quit();
  });
  return app.exec();
 }catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}
}
