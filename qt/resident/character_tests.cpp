#include "application.h"
#include "characters/fixtures.h"
#include "characters/loader.h"
#include "desktop_host.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMouseEvent>
#include <QQuickWindow>
#include <QScreen>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
QQuickWindow *window() {
  for (auto *w : QGuiApplication::topLevelWindows())
    if (w->title() == "Better Pets")
      return dynamic_cast<QQuickWindow *>(w);
  throw std::runtime_error("pet window not found");
}
void mouse(QEvent::Type type, QPoint global, Qt::MouseButton button,
           Qt::MouseButtons buttons) {
  auto *w = window();
  const QPoint local = global - w->position();
  QMouseEvent e(type, QPointF(local), QPointF(global), button, buttons,
                Qt::NoModifier);
  QApplication::sendEvent(w, &e);
}
void tick(pets::ResidentApplication &shell) {
  auto *timer = shell.findChild<QTimer *>();
  check(timer &&
            QMetaObject::invokeMethod(timer, "timeout", Qt::DirectConnection),
        "actual timer callback runs");
}
bool equalPose(const pets::character::Pose &a, const pets::character::Pose &b) {
  if (a.eyeOpenness != b.eyeOpenness || a.root.x != b.root.x || a.root.y != b.root.y ||
      a.bones.size() != b.bones.size())
    return false;
  for (size_t i = 0; i < a.bones.size(); ++i) {
    const auto &x = a.bones[i];
    const auto &y = b.bones[i];
    if (x.x != y.x || x.y != y.y || x.rotation != y.rotation ||
        x.scaleX != y.scaleX || x.scaleY != y.scaleY)
      return false;
  }
  return true;
}
void copyPack(const QString &source, const QString &destination) {
  check(QDir().mkpath(destination), "copy pack destination");
  for (const auto &name : QDir(source).entryList(QDir::Files))
    check(QFile::copy(source + "/" + name, destination + "/" + name),
          "copy package files");
}
void writeManifest(const QString &path, const QJsonObject &object) {
  QFile file(path + "/manifest.json");
  check(file.open(QIODevice::WriteOnly | QIODevice::Truncate),
        "manifest writable");
  const auto bytes = QJsonDocument(object).toJson();
  check(file.write(bytes) == bytes.size(), "manifest writes completely");
}
} // namespace
int main(int argc, char **argv) {
  QApplication app(argc, argv);
  app.setQuitOnLastWindowClosed(false);
  const bool dropOnly=argc==3&&QString::fromLocal8Bit(argv[2])=="--drop-only";
  if (argc != 2 && !dropOnly)
    return 2;
  const QString root = QString::fromLocal8Bit(argv[1]);
  int failures = 0;
  const auto test = [&](const char *name, const auto &f) {
    if(dropOnly && !QString::fromLatin1(name).startsWith("elevated drop") &&
        !QString::fromLatin1(name).startsWith("pickup interrupts")) return;
    try {
      f();
      std::cout << "PASS " << name << '\n';
    } catch (const std::exception &e) {
      ++failures;
      std::cerr << "FAIL " << name << ": " << e.what() << '\n';
    }
  };
  test("quadruped selection, travel and suppression preserve four real paws", [&] {
    QTemporaryDir state; double now=0;
    pets::ResidentApplication shell(root+"/marmalade",state.path(),false,[&]{return now;},
        pets::IpcMode::DisabledForTests,nullptr,false,[]{return std::optional<QPointF>{};});
    check(shell.groundedState()->feet.size()==4,"cat exposes four active paws");
    const auto start=shell.groundedState()->rootX;
    shell.setGroundTarget(start+45);
    std::array<bool,4> stepped{};
    for(int frame=0;frame<600;++frame) {
      now+=1./60; tick(shell);
      const auto actual=shell.groundedState().value();
      int swinging=0;
      for(std::size_t i=0;i<actual.feet.size();++i) {
        if(actual.feet[i].swing) {stepped[i]=true;++swinging;}
      }
      check(swinging<=1,"resident ordinary walk has at most one lifted paw");
      if(frame>60&&!shell.groundMotionActive())break;
    }
    check(shell.groundedState()->rootX>start+35,"resident realizes cat travel");
    for(bool moved:stepped)check(moved,"every cat paw participates in travel");
    check(!shell.groundMotionActive(),"cat arrival stops motion scheduling");
    shell.setGroundTarget(shell.groundedState()->rootX-40);
    now+=1./60;tick(shell);
    shell.control(pets::Control::Pause);
    const auto before=shell.groundedState().value();
    const auto frozen=shell.characterPose();
    now+=50;tick(shell);
    const auto after=shell.groundedState().value();
    check(equalPose(frozen,shell.characterPose())&&!shell.groundMotionActive(),"pause freezes quadruped pose and motion deadlines");
    for(std::size_t i=0;i<4;++i)check(before.feet[i].position.x==after.feet[i].position.x&&
        before.feet[i].position.y==after.feet[i].position.y,"pause freezes every paw");
    shell.control(pets::Control::Resume);
    check(shell.selectCharacter(root+"/soft-creature"),"switch quadruped to biped");
    check(shell.groundedState()->feet.size()==2,"biped selection has no phantom paws");
    check(shell.selectCharacter(root+"/marmalade"),"switch back to quadruped");
    check(shell.groundedState()->feet.size()==4,"cat selection restores four-paw state");
    QTemporaryDir malformed;
    const QString bad=malformed.path()+"/bad";
    copyPack(root+"/marmalade",bad);
    QFile manifest(bad+"/manifest.json");check(manifest.open(QIODevice::ReadOnly),"read malformed fixture");
    auto object=QJsonDocument::fromJson(manifest.readAll()).object();manifest.close();
    auto motion=object["locomotion"].toObject();auto legs=motion["legs"].toArray();legs.removeLast();
    motion["legs"]=legs;object["locomotion"]=motion;writeManifest(bad,object);
    QString error;
    check(!shell.selectCharacter(bad,&error)&&!error.isEmpty(),"invalid quadruped selection rejects visibly");
    check(shell.selectedCharacter()=="marmalade"&&shell.groundedState()->feet.size()==4,"failed selection preserves the live cat");
  });
  test("quadruped support requires the complete four-paw footprint", [&] {
    for(bool wide:{false,true}) {
      QTemporaryDir state;double now=0;
      std::vector<pets::character::SupportSurface> surfaces;
      pets::ResidentApplication shell(root+"/marmalade",state.path(),false,[&]{return now;},
          pets::IpcMode::DisabledForTests,nullptr,false,[]{return std::optional<QPointF>{};},
          [&](double time){auto observed=surfaces;for(auto& edge:observed)edge.observedAt=time;
            return pets::WindowSurfaceSnapshot{true,std::move(observed)};});
      const auto loaded=pets::character::loadCharacter(root+"/marmalade");
      const auto initial=shell.groundedState().value();
      double left=initial.feet[0].position.x,right=left;
      const std::size_t count=wide?4:2;
      for(std::size_t i=0;i<count;++i){left=std::min(left,initial.feet[i].position.x);right=std::max(right,initial.feet[i].position.x);}
      const double edgeY=QGuiApplication::primaryScreen()->availableGeometry().top()+250;
      surfaces={{"test:cat-window",edgeY,{{left-3,right+3}},now}};
      now=.6;tick(shell);
      const int margin=int(std::ceil(loaded.definition.motionEnvelope));QPolygon grab;
      for(const auto p:loaded.definition.interaction)grab<<QPoint(qRound(p.x)+margin,qRound(p.y)+margin);
      const auto hit=QRegion(grab).boundingRect().center();
      const auto start=window()->position()+hit;
      mouse(QEvent::MouseButtonPress,start,Qt::LeftButton,Qt::LeftButton);
      now+=.1;
      mouse(QEvent::MouseMove,start+QPoint(0,qRound(edgeY-initial.feet[0].position.y)),Qt::NoButton,Qt::LeftButton);
      now+=.5;tick(shell);
      mouse(QEvent::MouseButtonRelease,window()->position()+hit,Qt::LeftButton,Qt::NoButton);
      const auto released=shell.groundedState().value();
      if(wide) {
        check(released.phase==pets::character::DropPhase::Grounded&&released.elevation==0,"wide window supports all four paws");
        for(const auto& foot:released.feet)check(std::abs(foot.position.y-edgeY)<1,"four paws share selected window floor");
        const auto previous=released;
        surfaces[0].topY+=7;surfaces[0].segments[0].minimum+=9;surfaces[0].segments[0].maximum+=9;
        now+=.6;tick(shell);
        const auto moved=shell.groundedState().value();
        for(std::size_t i=0;i<4;++i)check(std::abs(moved.feet[i].position.x-previous.feet[i].position.x-9)<1&&
            std::abs(moved.feet[i].position.y-previous.feet[i].position.y-7)<1,"moving support carries every paw once");
        surfaces.clear();now+=.6;tick(shell);
        check(shell.groundedState()->phase!=pets::character::DropPhase::Grounded,"lost window releases quadruped to gravity");
      } else check(released.elevation>0&&released.phase!=pets::character::DropPhase::Grounded,
          "window fitting forepaws alone cannot support quadruped");
    }
  });
  test("elevated drop preserves release height and suspends without catch-up", [&] {
    QTemporaryDir state; double now=0;
    pets::ResidentApplication shell(root+"/soft-creature",state.path(),false,
        [&]{return now;},pets::IpcMode::DisabledForTests,nullptr,true,
        []{return std::optional<QPointF>{};});
    const auto loaded=pets::character::loadCharacter(root+"/soft-creature");
    const int margin=int(std::ceil(loaded.definition.motionEnvelope));
    QPolygon grab;
    for(const auto p:loaded.definition.interaction) grab << QPoint(qRound(p.x)+margin,qRound(p.y)+margin);
    const auto hit=QRegion(grab).boundingRect().center();
    const QPoint start=window()->position()+hit;
    mouse(QEvent::MouseButtonPress,start,Qt::LeftButton,Qt::LeftButton);
    now=.1;
    mouse(QEvent::MouseMove,start+QPoint(0,-120),Qt::NoButton,Qt::LeftButton);
    const auto raised=window()->position();
    now=.5; // A stationary hold must not throw the pet upward.
    mouse(QEvent::MouseButtonRelease,raised+hit,Qt::LeftButton,Qt::NoButton);
    check(window()->position()==raised,"release preserves height instead of snapping to floor");
    now+=1./60;tick(shell);
    check(window()->position().y()>=raised.y()&&shell.groundMotionActive(),"drop starts with gravity and a physics deadline");
    shell.control(pets::Control::Pause);
    const auto frozen=window()->position();
    const auto frozenPose=shell.characterPose();
    now+=100;tick(shell);
    check(window()->position()==frozen&&equalPose(frozenPose,shell.characterPose()),"pause freezes fall and pose");
    shell.control(pets::Control::Resume);
    check(window()->position()==frozen,"resume preserves position before the next step");
    now+=1./60;tick(shell);
    check(window()->position().y()-frozen.y()<8,"resume discards elapsed suppressed time");
    shell.control(pets::Control::Hide);
    const auto hidden=window()->position();
    now+=100;shell.control(pets::Control::Show);
    check(window()->position()==hidden,"show preserves suspended fall height");
    for(int i=0;i<900&&shell.groundMotionActive();++i){now+=1./60;tick(shell);}
    const auto area=QGuiApplication::primaryScreen()->availableGeometry();
    const auto floorAnchor=margin+loaded.definition.locomotion->groundY;
    check(std::abs(window()->position().y()+floorAnchor-(area.y()+area.height()))<1,
        "drop lands at actual screen floor");
    check(!shell.groundMotionActive(),"landing recovery becomes quiet");
  });
  test("pickup interrupts falling and cancelled or equal-time input cannot throw", [&] {
    for(const auto id:{"soft-creature","werewolf"}) {
      QTemporaryDir state; double now=0;
      pets::ResidentApplication shell(root+"/"+id,state.path(),false,[&]{return now;},
          pets::IpcMode::DisabledForTests,nullptr,true,[]{return std::optional<QPointF>{};});
      const auto loaded=pets::character::loadCharacter(root+"/"+id);
      QPolygon grab; const int margin=int(std::ceil(loaded.definition.motionEnvelope));
      for(const auto p:loaded.definition.interaction) grab << QPoint(qRound(p.x)+margin,qRound(p.y)+margin);
      const auto hit=QRegion(grab).boundingRect().center();
      auto start=window()->position()+hit;
      mouse(QEvent::MouseButtonPress,start,Qt::LeftButton,Qt::LeftButton);
      // All events share a timestamp. No velocity interval exists.
      mouse(QEvent::MouseMove,start+QPoint(35,-140),Qt::NoButton,Qt::LeftButton);
      mouse(QEvent::MouseButtonRelease,window()->position()+hit,Qt::LeftButton,Qt::NoButton);
      const auto afterRelease=window()->position();
      for(int i=0;i<8;++i){now+=1./60;tick(shell);}
      check(window()->position().x()==afterRelease.x(),"equal timestamps cannot create horizontal throw");
      const auto falling=window()->position();
      mouse(QEvent::MouseButtonPress,falling+hit,Qt::LeftButton,Qt::LeftButton);
      check(window()->position()==falling&&shell.groundedState()->phase==pets::character::DropPhase::Held,
          "re-grab preserves falling body position");
      start=window()->position()+hit;
      now+=.03;mouse(QEvent::MouseMove,start+QPoint(20,-20),Qt::NoButton,Qt::LeftButton);
      QEvent ungrab(QEvent::UngrabMouse);QApplication::sendEvent(window(),&ungrab);
      const auto cancelled=window()->position();
      check(!shell.runtime().state().dragging,"lost grab ends direct control");
      for(int i=0;i<8;++i){now+=1./60;tick(shell);}
      check(window()->position().x()==cancelled.x(),"cancelled drag cannot throw horizontally");
      // A genuine recent movement does retain horizontal momentum.
      start=window()->position()+hit;
      mouse(QEvent::MouseButtonPress,start,Qt::LeftButton,Qt::LeftButton);
      now+=.04;mouse(QEvent::MouseMove,start+QPoint(64,-30),Qt::NoButton,Qt::LeftButton);
      now+=.005;mouse(QEvent::MouseButtonRelease,window()->position()+hit,Qt::LeftButton,Qt::NoButton);
      const auto initialThrowSpeed=shell.groundedState()->velocity;
      check(std::abs(initialThrowSpeed-64/.045)<.01,
          "release retains measured drag speed before continuous air resistance");
      const auto thrown=window()->position();now+=1./60;tick(shell);
      check(window()->position().x()>thrown.x(),"real recent drag retains release momentum");
      check(shell.groundedState()->velocity>0 && shell.groundedState()->velocity<initialThrowSpeed,
          "air resistance slows an unobstructed throw continuously after release");
      for(int i=0;i<1200&&shell.groundMotionActive();++i){now+=1./60;tick(shell);}
      check(!shell.groundMotionActive()&&shell.groundedState()->phase==pets::character::DropPhase::Grounded,
          "both production anatomies recover and become quiet");
      start=window()->position()+hit;
      mouse(QEvent::MouseButtonPress,start,Qt::LeftButton,Qt::LeftButton);
      mouse(QEvent::MouseMove,start+QPoint(0,-160),Qt::NoButton,Qt::LeftButton);
      now+=.01;mouse(QEvent::MouseMove,start+QPoint(-40,-190),Qt::NoButton,Qt::LeftButton);
      now+=.005;mouse(QEvent::MouseButtonRelease,window()->position()+hit,Qt::LeftButton,Qt::NoButton);
      const auto fling=shell.groundedState().value();
      check(std::abs(std::hypot(fling.velocity,fling.verticalVelocity)-50/.015)<.01,
          "faster fling starts faster without a hard cap");
      check(std::abs(fling.verticalVelocity/fling.velocity-.75)<.001,
          "release preserves diagonal fling direction before air resistance");
    }
  });
  test("occasional eye blinks freeze under pause and settle fully open", [&] {
    QTemporaryDir state;double now=0;
    pets::ResidentApplication shell(root+"/soft-creature",state.path(),false,
        [&]{return now;},pets::IpcMode::DisabledForTests,nullptr,true,
        []{return std::optional<QPointF>{QPointF(-100000,-100000)};});
    double minimum=1; bool pausedBlink=false;
    for(int i=0;i<1500;++i) {
      now+=1./60;tick(shell);minimum=std::min(minimum,shell.characterPose().eyeOpenness);
      if(!pausedBlink&&shell.characterPose().eyeOpenness<.5) {
        const double held=shell.characterPose().eyeOpenness;
        shell.control(pets::Control::Pause);now+=40;tick(shell);
        check(shell.characterPose().eyeOpenness==held&&!shell.timerActive(),
              "paused blink retains eyelid position without deadlines");
        shell.control(pets::Control::Resume);
        check(shell.characterPose().eyeOpenness==held,"resumed blink has no elapsed-time jump");
        pausedBlink=true;
      }
    }
    check(minimum<.1&&pausedBlink,"visible resident performs an occasional complete blink");
    shell.control(pets::Control::Hide);
    check(!shell.timerActive(),"hidden blinks add no polling");
  });
  test("cursor lateral gaze follows screen direction after planar reversal", [&] {
    for(bool reversed:{false,true}) {
      QTemporaryDir state; double now=0;
      std::optional<QPointF> cursor=QPointF(-100000,-100000);
      pets::ResidentApplication shell(root+"/soft-creature",state.path(),false,
          [&]{return now;},pets::IpcMode::DisabledForTests,nullptr,true,[&]{return cursor;});
      if(reversed) {
        shell.setGroundTarget(shell.groundedState()->rootX-60);
        for(int i=0;i<600;++i){now+=1./60;tick(shell);}
        check(shell.groundedState()->facing<0,"fixture turns the pet left");
      }
      const auto loaded=pets::character::loadCharacter(root+"/soft-creature");
      const int head=loaded.definition.bindings.at("head");
      const auto headWorld=pets::character::worldTransforms(loaded.definition,shell.characterPose())[head];
      const double margin=std::ceil(loaded.definition.motionEnvelope);
      const QPointF center=QPointF(window()->position())+QPointF(headWorld.tx+margin,headWorld.ty+margin);
      const double gazeOffset=.25*loaded.definition.width;
      cursor=center+QPointF(-gazeOffset,0);
      for(int i=0;i<120;++i){now+=1./60;tick(shell);}
      const double left=shell.characterPose().bones[head].rotation;
      cursor=center+QPointF(gazeOffset,0);
      for(int i=0;i<120;++i){now+=1./60;tick(shell);}
      const double right=shell.characterPose().bones[head].rotation;
      check((right-left)*(reversed?-1:1)>.1,"rightward pointer change tilts toward screen right in both facings");
    }
  });
  test("planar cursor attention turns in place and ignores center jitter", [&] {
    for(const auto id:{"soft-creature","werewolf"}) {
      QTemporaryDir state; double now=0;
      std::optional<QPointF> cursor=QPointF(-100000,-100000);
      pets::ResidentApplication shell(root+"/"+id,state.path(),false,
          [&]{return now;},pets::IpcMode::DisabledForTests,nullptr,true,[&]{return cursor;});
      const auto loaded=pets::character::loadCharacter(root+"/"+id);
      const double margin=std::ceil(loaded.definition.motionEnvelope);
      const auto headCenter=[&] {
        const auto anchors=pets::character::utilityAnchors(loaded.definition,shell.characterPose());
        return QPointF(window()->position())+QPointF(anchors.head.x+margin,anchors.head.y+margin);
      };
      const auto startPosition=window()->position();
      const auto initial=shell.groundedState().value();
      const auto advancePinned=[&](double dt=1./60) {
        now+=dt;tick(shell);
        const auto actual=shell.groundedState().value();
        check(window()->position()==startPosition&&std::abs(actual.rootX-initial.rootX)<.01
              &&std::abs(actual.targetX-initial.targetX)<.01,
              "planar cursor facing preserves root, window and navigation target");
        for(int foot=0;foot<2;++foot) {
          check(!actual.feet[foot].swing,"planar cursor turn does not launch foot swings");
          check(std::hypot(actual.feet[foot].position.x-initial.feet[foot].position.x,
                           actual.feet[foot].position.y-initial.feet[foot].position.y)<.01,
                "planar cursor turn keeps both world foot pins");
        }
      };
      for(int facing:{-1,1}) {
        // Clear lateral targets are beyond the head-relative turn deadband.
        cursor=headCenter()+QPointF(facing*.65*loaded.definition.width,0);
        for(int frame=0;frame<300;++frame)advancePinned();
        check(shell.groundedState()->facing==facing&&shell.groundedState()->facingBlend==facing,
              "cursor behind planar pet completes its procedural facing turn");
        check(!shell.groundMotionActive(),"held planar cursor turn stops motion scheduling");
        // The fixed frame endpoint can precede the next sparse cursor poll.
        // Observe its next deadline before checking the full quiet interval.
        advancePinned(shell.timerInterval()/1000.);
        check(!shell.groundMotionActive()&&shell.timerInterval()>=80,
              "held planar cursor turn settles into sparse observation");
      }
      for(int frame=0;frame<180;++frame) {
        cursor=headCenter()+QPointF((frame%2 ? -1 : 1)*.2*loaded.definition.width,0);
        advancePinned();
        check(shell.groundedState()->facing==1&&shell.groundedState()->facingBlend==1,
              "near-center cursor noise preserves planar heading without intermediate flips");
      }
    }
  });
  test("nearby cursor drives smooth gaze and releases without walking", [&] {
    QTemporaryDir state; double now=0; int cursorSamples=0;
    std::optional<QPointF> cursor=QPointF(-100000,-100000);
    pets::ResidentApplication shell(root+"/soft-creature",state.path(),false,
        [&]{return now;},pets::IpcMode::DisabledForTests,nullptr,true,[&]{++cursorSamples;return cursor;});
    const auto baseline=shell.characterPose();
    const auto rootBefore=shell.groundedState()->rootX;
    // A target just above and right of the visible pet, in global logical pixels.
    cursor=QPointF(window()->position()+QPoint(window()->width()/2+60,window()->height()/3-40));
    now=.6; tick(shell);
    check(equalPose(baseline,shell.characterPose()),"cursor submits goals without snapping the realized pose");
    for(int frame=0;frame<120;++frame){now+=1./60;tick(shell);}
    check(!equalPose(baseline,shell.characterPose()),"nearby cursor produces a realized head response");
    check(std::abs(shell.groundedState()->rootX-rootBefore)<.01,"looking does not initiate travel");
    check(!shell.groundMotionActive()&&cursorSamples<=25,"stationary cursor settles and sampling stays below animation frequency");
    now+=shell.timerInterval()/1000.; tick(shell);
    check(!shell.groundMotionActive()&&shell.timerInterval()>=80,"settled cursor poll schedules a sparse future observation");
    const auto held=shell.characterPose();
    shell.control(pets::Control::Pause); cursor=QPointF(-100000,-100000);
    now+=10;tick(shell);
    check(equalPose(held,shell.characterPose())&&!shell.timerActive(),"pause freezes cursor attention without polling");
    shell.control(pets::Control::Resume);
    check(equalPose(held,shell.characterPose()),"cursor leaving clears through followers rather than snapping");
    for(int frame=0;frame<120;++frame){now+=1./60;tick(shell);}
    check(equalPose(baseline,shell.characterPose()),"out of range cursor returns to neutral");
    shell.control(pets::Control::Hide);
    check(!shell.timerActive(),"hidden cursor attention has no polling deadline");
  });
  test("autonomous idle initiates procedural expression before the old ambient deadline", [&] {
    QTemporaryDir state; double now = 0;
    pets::ResidentApplication shell(root + "/soft-creature", state.path(), false,
        [&] { return now; }, pets::IpcMode::DisabledForTests);
    const auto baseline = shell.characterPose();
    double maximum = 0;
    for (int frame = 0; frame < 1440; ++frame) {
      now += 1.0/60; tick(shell);
      const auto posed = shell.characterPose();
      for (size_t bone = 0; bone < posed.bones.size(); ++bone)
        maximum = std::max(maximum, std::abs(posed.bones[bone].rotation-baseline.bones[bone].rotation));
    }
    check(maximum > .015, "pet chooses a visible expression without user input");
    shell.control(pets::Control::Pause);
    const auto frozen = shell.characterPose();
    now += 120; tick(shell);
    check(equalPose(frozen, shell.characterPose()) && !shell.timerActive(),
        "paused autonomy has no ticking or pose changes");
    shell.control(pets::Control::Hide);
    shell.control(pets::Control::Resume);
    check(!shell.timerActive(), "hidden autonomy has no deadline");
    shell.control(pets::Control::Show);
    check(shell.timerActive() && shell.timerInterval() >= 100 && !shell.groundMotionActive(),
        "show starts sparse cursor observation without replay or animation");
  });
  test("nongrounded expression retains continuity across pause and resume", [&] {
    QTemporaryDir catalog,state;
    copyPack(root + "/soft-creature",catalog.path()+"/soft-creature");
    QFile manifest(catalog.path()+"/soft-creature/manifest.json");
    check(manifest.open(QIODevice::ReadOnly),"nongrounded manifest reads");
    auto object=QJsonDocument::fromJson(manifest.readAll()).object(); manifest.close();
    object.remove("locomotion"); writeManifest(catalog.path()+"/soft-creature",object);
    double now=0;
    pets::ResidentApplication shell(catalog.path()+"/soft-creature",state.path(),false,
        [&] { return now; },pets::IpcMode::DisabledForTests);
    for(int frame=0;frame<787;++frame) { now+=1./60; tick(shell); }
    const auto posed=shell.characterPose();
    shell.control(pets::Control::Pause); now+=10;
    shell.control(pets::Control::Resume);
    check(equalPose(posed,shell.characterPose()),"resume keeps realized expressive pose until motion advances");
  });
  test("settled notification hold sleeps until its next procedural phase", [&] {
    QTemporaryDir state; double now=0;
    pets::ResidentApplication shell(root + "/soft-creature",state.path(),false,
        [&] { return now; },pets::IpcMode::DisabledForTests);
    check(shell.handleRequest("session.update",{{"source_id","test"},{"session_id","hold"},
        {"label","Done"},{"seq",1},{"status","completed"},{"ttl",120}}).errorCode==0,
        "notification starts");
    for(int frame=0;frame<90;++frame) { now+=1./60; tick(shell); }
    check(!shell.groundMotionActive() && shell.timerInterval()>100,
        "settled expression hold has no animation-frame deadline");
    const auto held=shell.characterPose();
    shell.control(pets::Control::Pause);
    now+=20; tick(shell);
    check(equalPose(held,shell.characterPose()),"pause retains held expression");
  });
  test("3D unavailable renderer gives actionable rollback", [&] {
    QTemporaryDir state;
    pets::ResidentApplication shell(root + "/soft-creature", state.path(), false,
        {}, pets::IpcMode::DisabledForTests);
    const auto position = window()->position();
    const auto mask = window()->mask();
    QString error;
    check(!shell.selectCharacter(root + "/werewolf-3d", &error), "software renderer rejects 3D");
    check(error.contains("3D") || error.contains("GPU"), "3D failure is actionable");
    check(shell.selectedCharacter() == "soft-creature" && window()->position() == position
        && window()->mask() == mask, "3D rejection retains renderer and placement");
  });
  test("oversized transparent canvas preserves grounded navigation", [&] {
    auto loaded=pets::character::loadCharacter(root + "/soft-creature");
    loaded.definition.height=QGuiApplication::primaryScreen()->availableGeometry().height()+20;
    pets::DesktopHost host(loaded);
    host.rebaseGround();
    const auto span=host.groundSpan();
    check(span.minimum<span.maximum, "empty canvas padding does not disable navigation");
    check(!host.moveOnGround(10).blocked, "visible pet moves within display despite oversized canvas");
  });
  test("session reactions preserve navigation contact anchors", [&] {
    QTemporaryDir state; double now=0;
    pets::ResidentApplication shell(root + "/soft-creature", state.path(), false,
        [&] { return now; }, pets::IpcMode::DisabledForTests);
    shell.setGroundTarget(shell.groundedState()->rootX+60);
    for(int i=0;i<20;i++) { now+=1.0/60; tick(shell); }
    const auto feet=shell.groundedState()->feet;
    check(shell.handleRequest("session.update", {{"source_id","test"},{"session_id","reaction"},
        {"label","Done"},{"seq",1},{"status","completed"},{"ttl",120}}).errorCode==0,
        "session reaction accepted during navigation");
    const auto after=shell.groundedState()->feet;
    for(int i=0;i<2;i++) check(feet[i].position.x==after[i].position.x && feet[i].position.y==after[i].position.y,
        "reaction pose publication cannot advance feet");
    const auto moving=shell.groundedState();
    shell.control(pets::Control::Pause);
    check(shell.groundedState()->rootX==moving->rootX && shell.groundedState()->velocity==moving->velocity &&
          shell.groundedState()->targetX==moving->targetX, "pause freezes simulation state");
    shell.control(pets::Control::Resume);
    check(shell.groundMotionActive(), "resume restarts outstanding motion");
    shell.control(pets::Control::Suspend); now+=100; shell.control(pets::Control::Wake);
    check(!shell.groundMotionActive() && shell.groundedState()->velocity==0, "wake rebases without stale navigation");
  });
  test("ground host accumulates fractions and clamps presentation", [&] {
    pets::DesktopHost host(pets::character::loadCharacter(root + "/soft-creature"));
    host.rebaseGround();
    const auto span = host.groundSpan();
    const auto before = host.groundRootX();
    for (int i=0; i<10; ++i) host.moveOnGround(.1);
    check(std::abs(host.groundRootX()-before-1) < .001, "ten subpixel proposals yield one realized pixel");
    for (int i=0; i<100; ++i) host.moveOnGround(100);
    const auto result = host.moveOnGround(20);
    check(result.blocked && std::abs(result.rootX-span.maximum)<.001, "display edge is authoritative");
    const auto available = QGuiApplication::primaryScreen()->availableGeometry();
    check(std::abs(host.groundWorldY()-(available.y()+available.height()))<.001, "authored foot contact maps to floor");
  });
  test("grounded targets realize integer placement and become quiet", [&] {
    QTemporaryDir state;
    double now = 0;
    pets::ResidentApplication shell(root + "/soft-creature", state.path(), false,
        [&] { return now; }, pets::IpcMode::DisabledForTests);
    check(shell.groundedState().has_value(), "grounded package chooses navigation controller");
    const auto before = shell.groundedState()->rootX;
    shell.setGroundTarget(before + 40);
    for (int i=0; i<1000; ++i) { now += 1.0/60; tick(shell); }
    check(shell.groundedState()->rootX > before + 30, "target moves actual host root");
    check(!shell.groundMotionActive(), "arrival removes navigation deadline");
    shell.setGroundTarget(before);
    shell.control(pets::Control::Pause);
    const auto frozen = shell.characterPose();
    const auto position = window()->position();
    now += 100; tick(shell);
    check(window()->position() == position && equalPose(frozen, shell.characterPose()), "pause freezes movement and pose");
    shell.control(pets::Control::Resume);
    now += 1.0/60; tick(shell);
    check(std::abs(window()->position().x()-position.x()) < 3, "resume discards stale time");
    shell.control(pets::Control::Hide);
    check(!shell.groundMotionActive(), "hide discards movement");
  });
  test("character host uses its own bounds and stable interaction geometry",
       [&] {
         const auto loaded =
             pets::character::loadCharacter(root + "/soft-creature");
         pets::DesktopHost host(loaded);
         const int margin = int(std::ceil(loaded.definition.motionEnvelope));
         check(window()->size() == QSize(loaded.definition.width + 2 * margin,
                                         loaded.definition.height + 2 * margin),
               "window follows character envelope");
         host.setPetVisible(true);
         const auto position = host.position();
         const auto mask = host.interactionRegion();
         pets::character::Controller controller(loaded.definition);
         host.applyPose(
             controller.pose(pets::character::Action::Celebrate, 0.4));
         check(host.position() == position && host.interactionRegion() == mask,
               "visual pose never changes placement or grab geometry");
       });
  test("transactional switching keeps sessions and failed pack keeps live pet",
       [&] {
         QTemporaryDir state;
         double now = 0;
         pets::ResidentApplication shell(
             root + "/soft-creature", state.path(), false, [&] { return now; },
             pets::IpcMode::DisabledForTests);
         check(shell.selectedCharacter() == "soft-creature",
               "production character selected");
         check(shell.handleRequest("session.update", {{"source_id", "test"},
                                                      {"session_id", "task"},
                                                      {"label", "Build"},
                                                      {"seq", 1},
                                                      {"status", "working"},
                                                      {"ttl", 120}})
                       .errorCode == 0,
               "unchanged public API drives production character");
         const auto oldPosition = window()->position();
         QString error;
         check(!shell.selectCharacter(root + "/missing", &error) &&
                   !error.isEmpty(),
               "invalid selection reports useful error");
         check(shell.selectedCharacter() == "soft-creature" &&
                   window()->position() == oldPosition && shell.petVisible(),
               "failed selection preserves pet");
         check(shell.selectCharacter(root + "/werewolf", &error),
               "different anatomy swaps successfully");
         check(shell.selectedCharacter() == "werewolf" &&
                   shell.handleRequest("state.get", {})
                           .result["sessions"]
                           .toArray()
                           .size() == 1,
               "switch preserves sessions and identity");
         shell.control(pets::Control::Pause);
         const auto frozen = shell.characterPose();
         now = 0.5;
         tick(shell);
         check(equalPose(frozen, shell.characterPose()),
               "pause freezes new character pose");
       });
  test("drag reaction wakes briefly, pause wins, hide clears deadlines", [&] {
    QTemporaryDir state;
    double now = 0;
    pets::ResidentApplication shell(
        root + "/soft-creature", state.path(), false, [&] { return now; },
        pets::IpcMode::DisabledForTests);
    const auto baseline = shell.characterPose();
    const QPoint hit = window()->mask().boundingRect().center();
    const QPoint start = window()->position() + hit;
    mouse(QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
    now = 0.03;
    mouse(QEvent::MouseMove, start + QPoint(20, 0), Qt::NoButton,
          Qt::LeftButton);
    check(shell.runtime().state().dragging && shell.timerActive(),
          "drag drives bounded secondary timer");
    now += 0.02;
    tick(shell);
    check(!equalPose(baseline, shell.characterPose()),
          "force changes local bones");
    shell.control(pets::Control::Pause);
    const auto frozen = shell.characterPose();
    now += 0.1;
    mouse(QEvent::MouseMove, start + QPoint(40, 0), Qt::NoButton,
          Qt::LeftButton);
    tick(shell);
    check(equalPose(frozen, shell.characterPose()) && !shell.timerActive(),
          "pause suppresses drag springs");
    shell.control(pets::Control::Hide);
    check(!shell.timerActive() && !shell.petVisible(),
          "empty hidden runtime has no spring timer");
    shell.control(pets::Control::Resume);
    shell.control(pets::Control::Show);
    check(!shell.runtime().state().dragging,
          "restore never replays a lost grab");
    const auto nextStart = window()->position() + hit;
    mouse(QEvent::MouseButtonPress, nextStart, Qt::LeftButton, Qt::LeftButton);
    now += 0.03;
    mouse(QEvent::MouseMove, nextStart + QPoint(20, 0), Qt::NoButton,
          Qt::LeftButton);
    mouse(QEvent::MouseButtonRelease, nextStart + QPoint(20, 0), Qt::LeftButton,
          Qt::NoButton);
    check(shell.springActive(), "release retains bounded follow-through");
    for (int i = 0; i < 600; ++i) {
      now += 1.0 / 60;
      tick(shell);
    }
    check(!shell.springActive(), "spring animation eventually settles");
    check(shell.timerInterval() > 100,
          "settled springs leave a sparse behavior deadline rather than frame ticks");
  });
  test("saved character selection restores within configured character root",
       [&] {
         QTemporaryDir state;
         double now = 0;
         {
           pets::ResidentApplication shell(
               root + "/soft-creature", state.path(), false,
               [&] { return now; }, pets::IpcMode::DisabledForTests);
           check(shell.selectCharacter(root + "/werewolf"),
                 "selected pack persists");
           shell.control(pets::Control::Pause);
         }
         pets::ResidentApplication restored(
             root + "/soft-creature", state.path(), false, [&] { return now; },
             pets::IpcMode::DisabledForTests);
         check(restored.selectedCharacter() == "werewolf" &&
                   restored.runtime().state().paused,
               "identity and pause survive restart");
       });
  test("frequent drag input does not postpone the spring deadline", [&] {
    QTemporaryDir state;
    double now = 0;
    pets::ResidentApplication shell(
        root + "/soft-creature", state.path(), false, [&] { return now; },
        pets::IpcMode::DisabledForTests);
    const QPoint start =
        window()->position() + window()->mask().boundingRect().center();
    mouse(QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
    now = .003;
    mouse(QEvent::MouseMove, start + QPoint(20, 0), Qt::NoButton,
          Qt::LeftButton);
    now = .007;
    mouse(QEvent::MouseMove, start + QPoint(22, 0), Qt::NoButton,
          Qt::LeftButton);
    check(shell.timerInterval() < 17,
          "input cannot push an existing spring deadline forward");
  });
  test("manifest identity restores independently from directory name", [&] {
    QTemporaryDir catalog, state;
    copyPack(root + "/soft-creature", catalog.path() + "/mallow-art");
    copyPack(root + "/werewolf", catalog.path() + "/wolf-v2");
    double now = 0;
    {
      pets::ResidentApplication shell(
          catalog.path() + "/mallow-art", state.path(), false,
          [&] { return now; }, pets::IpcMode::DisabledForTests);
      check(shell.selectCharacter(catalog.path() + "/wolf-v2"),
            "nonmatching directory selectable");
    }
    pets::ResidentApplication restored(
        catalog.path() + "/mallow-art", state.path(), false,
        [&] { return now; }, pets::IpcMode::DisabledForTests);
    check(restored.selectedCharacter() == "werewolf",
          "restore resolves manifest identity");
  });
  test("explicit character choice overrides remembered selection", [&] {
    QTemporaryDir state;
    double now = 0;
    {
      pets::ResidentApplication shell(
          root + "/soft-creature", state.path(), false, [&] { return now; },
          pets::IpcMode::DisabledForTests);
      check(shell.selectCharacter(root + "/werewolf"), "remember a sibling");
    }
    pets::ResidentApplication chosen(
        root + "/soft-creature", state.path(), false, [&] { return now; },
        pets::IpcMode::DisabledForTests, nullptr, false);
    check(chosen.selectedCharacter() == "soft-creature",
          "explicit choice wins over preferences");
  });
  test("unreachable interaction cannot replace live character", [&] {
    QTemporaryDir catalog, state;
    copyPack(root + "/soft-creature", catalog.path() + "/soft-creature");
    copyPack(root + "/werewolf", catalog.path() + "/bad");
    QFile manifest(catalog.path() + "/bad/manifest.json");
    check(manifest.open(QIODevice::ReadOnly), "manifest opens");
    auto object = QJsonDocument::fromJson(manifest.readAll()).object();
    manifest.close();
    object["interaction"] = QJsonArray{
        QJsonArray{5000, 5000}, QJsonArray{5010, 5000}, QJsonArray{5010, 5010}};
    writeManifest(catalog.path() + "/bad", object);
    double now = 0;
    pets::ResidentApplication shell(
        catalog.path() + "/soft-creature", state.path(), false,
        [&] { return now; }, pets::IpcMode::DisabledForTests);
    const auto oldPosition = window()->position();
    const auto oldMask = window()->mask();
    check(!shell.selectCharacter(catalog.path() + "/bad"),
          "outside interaction rejected");
    check(shell.selectedCharacter() == "soft-creature" &&
              window()->position() == oldPosition &&
              window()->mask() == oldMask,
          "rejection preserves live pet and geometry");
  });
  return failures ? 1 : 0;
}
