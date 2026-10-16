#include "application.h"
#include "settings.h"
#include "spatial_character.h"
#include "desktop_host.h"
#include <QQuaternion>
#include <QApplication>
#include <QJsonArray>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QTemporaryDir>
#include <QEventLoop>
#include <QImage>
#include <QScreen>
#include <QMouseEvent>
#include <iostream>
#include <cmath>
#include <stdexcept>

void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
QQuickWindow *window() {
  for (auto *w : QGuiApplication::topLevelWindows())
    if (w->title() == "Better Pets") return static_cast<QQuickWindow*>(w);
  throw std::runtime_error("resident window unavailable");
}
void tick(pets::ResidentApplication &shell) {
  check(QMetaObject::invokeMethod(shell.findChild<QTimer*>(), "timeout", Qt::DirectConnection), "resident timer callback");
}
int main(int argc, char **argv) {
  // Exercise prepared GPU scenes without requesting an offscreen framebuffer.
  const bool leftCapture = argc==4 && QString::fromLocal8Bit(argv[2])=="--native-left-capture";
  const bool nativeCapture = leftCapture || (argc==4 && QString::fromLocal8Bit(argv[2])=="--native-capture");
  if (!nativeCapture) {
    QQuickWindow::setSceneGraphBackend("rhi");
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
  }
  QApplication app(argc,argv); app.setQuitOnLastWindowClosed(false);
  if (argc != 2 && !nativeCapture) return 2;
  const QString root = QString::fromLocal8Bit(argv[1]);
  try {
    // The gaze mapping follows the actual camera and body scene frames, even
    // while grounded turning changes the current heading.
    {
      pets::DesktopHost host(pets::character::loadCharacter(root+"/soft-creature"));
      pets::SpatialCharacter spatial;
      spatial.install(host); spatial.reset(host); spatial.apply(host);
      auto *stage = window()->findChild<QObject*>("resident3dStage");
      auto *camera = stage->findChild<QObject*>("camera");
      auto *avatar = stage->findChild<QObject*>("avatar");
      auto verifyGaze = [&] {
        const auto cameraRotation=camera->property("sceneRotation").value<QQuaternion>();
        const auto avatarRotation=avatar->property("sceneRotation").value<QQuaternion>();
        for (const pets::motion::Vec3 screen : {pets::motion::Vec3{.4,.3,1},pets::motion::Vec3{-.4,-.3,1}}) {
          const auto local=spatial.cursorLookDirection(screen);
          const auto recovered=cameraRotation.conjugated().rotatedVector(avatarRotation.rotatedVector(
              QVector3D(float(local.x),float(local.y),float(local.z-screen.z))));
          check((recovered-QVector3D(float(screen.x),float(screen.y),0)).length()<1e-5,
                "cursor gaze preserves screen direction across model heading");
        }
        const auto center=spatial.cursorLookDirection({0,0,1});
        check(std::abs(center.x)+std::abs(center.y)+std::abs(center.z-1)<1e-8,
              "centered cursor retains neutral head range");
        for(double offset : {-.4,.4}) {
          const auto gaze=spatial.cursorLookDirection({offset,0,1});
          const double yaw=std::atan2(gaze.x,gaze.z);
          check(yaw*offset>0 && std::abs(yaw)<.65,
                "cursor is responsive in both directions within bounded head range");
        }
        check(spatial.headAnchor().x()>0 && spatial.headAnchor().y()>0
              && spatial.headAnchor().y()<spatial.cardAnchor().y()+60,
              "cursor attention uses projected live head anchor");
      };
      verifyGaze();
      const pets::motion::Vec3 heldCursor{.4,0,1};
      const auto initial=spatial.cursorLookDirection(heldCursor);
      spatial.expressions().look(initial);
      spatial.target(host.groundRootX()-80);
      for(int i=0;i<1200 && !spatial.settled();++i) {
        spatial.expressions().look(spatial.cursorLookDirection(heldCursor));
        const auto proposal=spatial.propose(1./60);
        const auto realized=host.moveOnGround(proposal.displacement);
        spatial.realize(realized.rootX,realized.blocked); spatial.apply(host);
        if(i%120==0) verifyGaze();
      }
      const auto finalGoal=spatial.cursorLookDirection(heldCursor);
      const double initialYaw=std::atan2(initial.x,initial.z), finalYaw=std::atan2(finalGoal.x,finalGoal.z);
      check(std::abs(finalYaw-initialYaw)>.03,
            "held cursor direction changes local goal with body heading");
      check(std::abs(spatial.expressions().value(pets::character::Role::Head).y-finalYaw)<1e-5,
            "held canonical cursor gaze follows current body frame through turn");
      verifyGaze();
    }
    // Nearby attention may rotate the body, but must never turn into navigation.
    {
      QTemporaryDir cursorState; double cursorTime=0;
      std::optional<QPointF> cursor=QPointF(-100000,-100000);
      pets::ResidentApplication shell(root+"/werewolf-3d",cursorState.path(),false,
          [&]{return cursorTime;},pets::IpcMode::DisabledForTests,nullptr,false,[&]{return cursor;});
      auto *avatar=window()->findChild<QObject*>("avatar");
      check(avatar,"cursor turn has a live 3D body");
      const auto startRotation=avatar->property("sceneRotation").value<QQuaternion>();
      const auto startPosition=window()->position();
      const double startRoot=shell.groundedState()->rootX;
      cursor=QPointF(startPosition)+QPointF(82,160);
      for(int i=0;i<420;++i){cursorTime+=1./60;tick(shell);}
      const auto leftRotation=avatar->property("sceneRotation").value<QQuaternion>();
      check(std::abs(QQuaternion::dotProduct(startRotation,leftRotation))<.85,
            "cursor behind the 3D pet turns its body to face that side");
      check(window()->position()==startPosition&&std::abs(shell.groundedState()->rootX-startRoot)<.01
            &&std::abs(shell.groundedState()->targetX-startRoot)<.01,
            "cursor facing turns in place without requesting travel");
      check(!shell.groundMotionActive(),"held cursor-facing turn eventually stops frame scheduling");
      cursor=QPointF(startPosition)+QPointF(302,160);
      for(int i=0;i<420;++i){cursorTime+=1./60;tick(shell);}
      check(std::abs(QQuaternion::dotProduct(startRotation,avatar->property("sceneRotation").value<QQuaternion>()))>.99,
            "cursor across the pet requests the opposite procedural facing");
      check(!shell.groundMotionActive(),"opposite held turn settles");
    }
    {
      QTemporaryDir blinkState;double blinkTime=0;
      pets::ResidentApplication shell(root+"/werewolf-3d",blinkState.path(),false,
          [&]{return blinkTime;},pets::IpcMode::DisabledForTests,nullptr,false,
          []{return std::optional<QPointF>{QPointF(-100000,-100000)};});
      auto *eye=window()->findChild<QObject*>("eye_left");
      check(eye,"resident blink binds the eye joint");
      double minimum=1;bool pausedBlink=false;
      for(int i=0;i<1500;++i) {
        blinkTime+=1./60;tick(shell);
        const double open=eye->property("scale").value<QVector3D>().y();minimum=std::min(minimum,open);
        if(!pausedBlink&&open<.5) {
          shell.control(pets::Control::Pause);blinkTime+=30;tick(shell);
          check(eye->property("scale").value<QVector3D>().y()==open&&!shell.timerActive(),
                "3D eyelids freeze without animation while paused");
          shell.control(pets::Control::Resume);
          check(eye->property("scale").value<QVector3D>().y()==open,"3D blink resumes without a jump");
          pausedBlink=true;
        }
      }
      check(minimum<.1&&pausedBlink,"resident schedules and renders an occasional 3D blink");
    }
    QTemporaryDir state; double now=0;
    QPoint savedPosition;
    {
      pets::ResidentApplication shell(root+"/soft-creature",state.path(),false,[&]{return now;},pets::IpcMode::DisabledForTests);
      QString error;
      check(shell.selectCharacter(root+"/werewolf-3d",&error),error.toUtf8().constData());
      check(shell.selectedCharacter()=="werewolf-3d" && window()->size()==QSize(384,384),"3D resident selection");
      check(shell.setWalkingSpeed(3),"3D speed preference");
      const auto start=window()->position();
      shell.setGroundTarget(shell.groundedState()->rootX+80);
      for(int i=0;i<1200&&shell.groundMotionActive();++i) {now+=1./60;tick(shell);}
      check(window()->position().x()>start.x()+30 && window()->position().y()==start.y(),"horizontal realized 3D travel");
      check(!shell.groundMotionActive(),"3D motion settles without timer");
      {
        auto *stage=window()->findChild<QObject*>("resident3dStage");
        QVariant projected;
        check(QMetaObject::invokeMethod(stage,"project",Q_RETURN_ARG(QVariant,projected),
            Q_ARG(QVariant,QVariant::fromValue(QVector3D(0,1.5,0)))),"project 3D grab point");
        const auto hit=projected.toPointF().toPoint();
        auto sendMouse=[&](QEvent::Type type,QPoint global,Qt::MouseButton button,Qt::MouseButtons buttons){
          QMouseEvent event(type,QPointF(global-window()->position()),QPointF(global),button,buttons,Qt::NoModifier);
          QApplication::sendEvent(window(),&event);
        };
        const auto floorPosition=window()->position();
        const auto grab=floorPosition+hit;
        sendMouse(QEvent::MouseButtonPress,grab,Qt::LeftButton,Qt::LeftButton);
        check(shell.runtime().state().dragging,"3D model grab starts pickup");
        now+=.05;
        sendMouse(QEvent::MouseMove,grab-QPoint(0,120),Qt::NoButton,Qt::LeftButton);
        const auto raised=window()->position();
        now+=.3;
        sendMouse(QEvent::MouseButtonRelease,raised+hit,Qt::LeftButton,Qt::NoButton);
        check(window()->position()==raised&&shell.groundedState()->elevation>100,"3D release preserves projected height");
        now+=1./60;tick(shell);
        check(window()->position().y()>=raised.y()&&shell.groundMotionActive(),"3D root follows gravity downward");
        if(nativeCapture) {
          for(int frame=0;frame<5;++frame){now+=1./60;tick(shell);}
          check(shell.groundedState()->phase==pets::character::DropPhase::Airborne,"native falling pose is airborne");
          window()->update();
          QEventLoop present;QTimer::singleShot(80,&present,&QEventLoop::quit);present.exec();
          check(window()->grabWindow().save(QString::fromLocal8Bit(argv[3])+".fall.png"),"native falling pose captured");
        }
        for(int i=0;i<1200&&shell.groundMotionActive();++i){now+=1./60;tick(shell);}
        check(!shell.groundMotionActive()&&shell.groundedState()->phase==pets::character::DropPhase::Grounded,
            "3D landing and recovery settle shared dynamics");
        check(window()->position()==floorPosition,"3D fall elevation applies once and lands at original floor");
      }
      if (leftCapture) {
        const auto rightPosition=window()->position();
        shell.setGroundTarget(shell.groundedState()->rootX-80);
        for(int i=0;i<1200&&shell.groundMotionActive();++i) {now+=1./60;tick(shell);}
        check(window()->position().x()<rightPosition.x()-30 && window()->position().y()==rightPosition.y(),"horizontal realized 3D reverse travel");
        check(!shell.groundMotionActive(),"3D reverse motion settles without timer");
      }
      if (nativeCapture) {
        QCoreApplication::processEvents();
        const auto frame = window()->grabWindow();
        check(!frame.isNull(), "native 3D framebuffer available");
        int opaque=0;
        for(int y=0;y<frame.height();++y) for(int x=0;x<frame.width();++x)
          if(qAlpha(frame.pixel(x,y))>0) ++opaque;
        check(opaque>200 && opaque<frame.width()*frame.height()/2, "native character and transparent surrounding pixels");
        check(frame.save(QString::fromLocal8Bit(argv[3])), "native capture saved");
        int swaps=0;
        auto connection=QObject::connect(window(), &QQuickWindow::frameSwapped, [&]{++swaps;});
        QEventLoop settle; QTimer::singleShot(100,&settle,&QEventLoop::quit); settle.exec();
        swaps=0;
        QEventLoop quiet; QTimer::singleShot(500,&quiet,&QEventLoop::quit); quiet.exec();
        QObject::disconnect(connection);
        check(swaps<=1,"settled native 3D does not continuously redraw");
        std::cout<<"Native framebuffer: "<<opaque<<" visible pixels; settled redraws over 500ms: "<<swaps<<"\\n";
      }
      check(shell.handleRequest("session.update",{{"source_id","test"},{"session_id","task"},{"label","Build"},{"seq",1},{"status","completed"},{"ttl",120}}).errorCode==0,"3D uses shared sessions");
      for(int i=0;i<15;++i){now+=1./60;tick(shell);}
      const auto frozen=window()->position();
      shell.control(pets::Control::Pause);
      check(shell.timerInterval()>1000,"pause leaves only expiry timer");
      check(!window()->findChild<QObject*>("resident3dStage")->property("renderingEnabled").toBool(),"pause freezes live texture updates");
      now+=5;tick(shell);
      check(window()->position()==frozen&&!shell.groundMotionActive(),"paused 3D pose placement frozen");
      shell.control(pets::Control::Hide);
      check(!shell.petVisible()&&!shell.groundMotionActive(),"hidden 3D no animation deadline");
      shell.control(pets::Control::Show);shell.control(pets::Control::Resume);
      window()->sceneGraphError(QQuickWindow::ContextNotAvailable, "synthetic 3D failure");
      QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
      check(!shell.petVisible() && !shell.runtime().state().quitting, "3D graphics failure retains resident controls");
      check(shell.handleRequest("state.get",{}).result["sessions"].toArray().size()==1, "renderer failure retains sessions");
      check(shell.selectCharacter(root+"/werewolf"),"switch 3D to 2D");
      shell.control(pets::Control::Show);
      check(shell.handleRequest("state.get",{}).result["sessions"].toArray().size()==1,"switch preserves sessions");
      check(shell.selectCharacter(root+"/werewolf-3d"),"switch back to 3D");
      shell.control(pets::Control::Pause);
      savedPosition = window()->position();
    }
    {
      pets::ResidentApplication restored(root+"/soft-creature",state.path(),false,[&]{return now;},pets::IpcMode::DisabledForTests);
      check(restored.selectedCharacter()=="werewolf-3d"&&restored.runtime().state().paused&&restored.walkingSpeed()==3,"restores 3D identity speed and pause");
      check(window()->position()==savedPosition,"restored 3D placement remains exact");
      check(!restored.groundMotionActive(),"restored pause has no animation");
    }
    {
      QTemporaryDir explicitState;
      pets::ResidentApplication direct(root+"/werewolf-3d",explicitState.path(),false,[&]{return now;},pets::IpcMode::DisabledForTests,nullptr,false);
      check(direct.selectedCharacter()=="werewolf-3d","explicit 3D startup");
    }
    // A reachable torso does not mean the taller character's floor is visible.
    // Cover both a saved 2D placement and an already-persisted bad 3D placement.
    for (const auto identity : {QString("soft-creature"), QString("werewolf-3d")}) {
      QTemporaryDir oldState;
      const auto *screen=QGuiApplication::primaryScreen();
      const auto area=screen->availableGeometry();
      pets::Preferences old;
      old.character=identity; old.hasPosition=true; old.display=screen->name();
      old.position={area.center().x()-192, area.y()+area.height()-200};
      check(pets::savePreferences(oldState.path()+"/settings.json",old),"seed old shorter-character placement");
      const bool remembered=identity=="werewolf-3d";
      pets::ResidentApplication direct(root+(remembered?"/soft-creature":"/werewolf-3d"),oldState.path(),false,[&]{return now;},pets::IpcMode::DisabledForTests,nullptr,remembered);
      check(direct.selectedCharacter()=="werewolf-3d","restores requested or remembered 3D character");
      const auto stage=window()->findChild<QObject*>("resident3dStage");
      const double floorY=window()->position().y()+stage->property("rootFloorAnchor").toPointF().y();
      check(floorY<=area.y()+area.height(),"restored 3D floor must not fall below desktop");
      check(window()->position().x()==old.position.x(),"floor repair preserves horizontal saved position");
    }
    std::cout<<"3D resident selection, realized travel, suppression, sessions and restoration passed\n";
    return 0;
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
