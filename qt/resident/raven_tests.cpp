#include "application.h"
#include "characters/loader.h"
#include <QApplication>
#include <QTemporaryDir>
#include <QTimer>
#include <QQuickWindow>
#include <QScreen>
#include <QSGRendererInterface>
#include <QProcess>
#include <QAction>
#include <algorithm>
#include <QDir>
#include <QMouseEvent>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}
QQuickWindow *petWindow() {
  for(auto *w:QGuiApplication::topLevelWindows()) if(w->title()=="Clibby") return dynamic_cast<QQuickWindow*>(w);
  throw std::runtime_error("pet window missing");
}
void mouse(QEvent::Type type,QPoint global,Qt::MouseButton button,Qt::MouseButtons buttons) {
  auto *w=petWindow();
  QMouseEvent e(type,QPointF(global-w->position()),QPointF(global),button,buttons,Qt::NoModifier);
  QApplication::sendEvent(w,&e);
}
void tick(pets::ResidentApplication &shell, double &now, double seconds) {
  for (int i=0; i<int(std::ceil(seconds*120)); ++i) {
    now+=1./120;
    auto *timer=shell.findChild<QTimer*>();
    check(timer && QMetaObject::invokeMethod(timer,"timeout",Qt::DirectConnection),"resident timer exists");
  }
}
}
int main(int argc,char **argv) {
  bool spatial=false,nativeWindow=false;
  for(int i=2;i<argc;++i) {spatial|=QString::fromLocal8Bit(argv[i])=="--3d";nativeWindow|=QString::fromLocal8Bit(argv[i])=="--native-window";}
  if(spatial) {
    QQuickWindow::setSceneGraphBackend("rhi");
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
  }
  QApplication app(argc,argv); app.setQuitOnLastWindowClosed(false);
  if(argc!=2 && !spatial && !nativeWindow) return 2;
  const QString root=QString::fromLocal8Bit(argv[1]);
  const QString ravenDirectory=root+(spatial ? "/raven-3d" : "/raven");
  try {
    QProcess fixture;
    if(nativeWindow) {
      fixture.start(QCoreApplication::applicationDirPath()+"/window_surfaces_tests",{"--fixture"});
      check(fixture.waitForStarted(5000) && fixture.waitForReadyRead(5000),"native perch fixture opens");
      check(fixture.readAllStandardOutput().contains("READY"),"native perch fixture is ready");
    }
    QTemporaryDir storage; double now=0;
    bool nativePerchEnabled=false;
    pets::ResidentApplication::SurfaceSource nativeSource;
    if(nativeWindow) nativeSource=[&](double time) {
      auto snapshot=pets::observeWindowSurfaces(time);
      if(!nativePerchEnabled){snapshot.surfaces.clear();return snapshot;}
      const auto prefix="mac-window:"+std::to_string(fixture.processId())+":";
      snapshot.surfaces.erase(std::remove_if(snapshot.surfaces.begin(),snapshot.surfaces.end(),
          [&](const auto& edge){return edge.id.rfind(prefix,0)!=0;}),snapshot.surfaces.end());
      return snapshot;
    };
    pets::ResidentApplication raven(ravenDirectory,storage.path(),false,[&]{return now;},
        pets::IpcMode::DisabledForTests,nullptr,false,[]{return std::optional<QPointF>{};},nativeSource);
    check(raven.flightCapable(),"raven exposes flight capability");
    if(nativeWindow) {
      const int startingY=petWindow()->position().y();
      raven.control(pets::Control::Pause);
      const QPoint grab=petWindow()->position()+petWindow()->mask().boundingRect().center();
      mouse(QEvent::MouseButtonPress,grab,Qt::LeftButton,Qt::LeftButton);
      now+=.1;
      const QPoint high=grab-QPoint(0,startingY+120);
      mouse(QEvent::MouseMove,high,Qt::NoButton,Qt::LeftButton);
      check(petWindow()->position().y()<QGuiApplication::primaryScreen()->availableGeometry().top(),
            "native dragging allows transparent padding above the screen");
      mouse(QEvent::MouseButtonRelease,high,Qt::LeftButton,Qt::NoButton);
      raven.control(pets::Control::Resume);tick(raven,now,12);
      const double footOffset=QGuiApplication::primaryScreen()->availableGeometry().bottom()+1-startingY;
      auto* fly=raven.findChild<QAction*>("fly"); auto* land=raven.findChild<QAction*>("land");
      check(fly && land,"resident flight actions exist");
      const int flightStartX=petWindow()->position().x();
      fly->trigger(); double nativeTravel=0;
      for(int i=0;i<80;++i){tick(raven,now,.05);nativeTravel=std::max(nativeTravel,double(std::abs(petWindow()->position().x()-flightStartX)));}
      check(nativeTravel>15,"native flight visibly moves forward");
      std::cerr<<"NATIVE cruise height "<<startingY-petWindow()->position().y()<<" phase "<<int(raven.groundedState()->phase)<<" root "<<raven.groundedState()->rootX<<"\n";
      check(startingY-petWindow()->position().y()>70,"Fly up action visibly raises native pet");
      land->trigger(); tick(raven,now,8);
      check(raven.groundedState()->phase==pets::character::DropPhase::Grounded,"Land action reaches native desktop");
      auto observations=pets::observeWindowSurfaces(now);
      const auto prefix="mac-window:"+std::to_string(fixture.processId())+":";
      const auto edge=std::find_if(observations.surfaces.begin(),observations.surfaces.end(),[&](const auto& s){return s.id.rfind(prefix,0)==0;});
      check(edge!=observations.surfaces.end(),"separate native window is discovered");
      const double top=edge->topY;
      nativePerchEnabled=true;tick(raven,now,.6);
      const double nativePerchStart=now;
      check(raven.flyToWindow(),"native window flight is accepted");
      for(int i=0;i<3600 && raven.groundedState()->phase!=pets::character::DropPhase::Grounded;++i)tick(raven,now,1./120);
      check(raven.groundedState()->phase==pets::character::DropPhase::Grounded &&
          std::abs(petWindow()->position().y()+footOffset-top)<3,"raven lands on discovered native window top");
      std::cout<<"native window transfer settled seconds "<<now-nativePerchStart<<"\n";
      check(now-nativePerchStart<5,"native higher-window transfer settles within five seconds");
      raven.control(pets::Control::Quit); fixture.terminate();fixture.waitForFinished(2000);
      std::cout<<"PASS native Fly up, Land, and window perch actions\n";return 0;
    }
    check(raven.hop(35),"supported raven accepts hop");
    tick(raven,now,.14);
    check(raven.groundedState()->elevation>2,"hop moves the resident above its floor");
    raven.control(pets::Control::Pause);
    const auto frozen=*raven.groundedState(); tick(raven,now,2);
    check(raven.groundedState()->elevation==frozen.elevation,"pause freezes airborne state");
    check(!raven.flyTo(frozen.rootX,60),"paused raven rejects a flight command");
    raven.control(pets::Control::Resume); tick(raven,now,8);
    check(raven.groundedState()->phase==pets::character::DropPhase::Grounded,"hop lands and recovers");
    const double cruiseStart=raven.groundedState()->rootX;
    auto* cruiseAction=raven.findChild<QAction*>("fly");
    check(cruiseAction,"forward flight action exists");cruiseAction->trigger();
    double cruiseTravel=0;
    for(int i=0;i<240;++i){tick(raven,now,1./120);cruiseTravel=std::max(cruiseTravel,std::abs(raven.groundedState()->rootX-cruiseStart));}
    check(cruiseTravel>15,
          "Fly up travels forward rather than hovering in place");
    raven.control(pets::Control::Pause);
    check(!raven.land(),"paused cruise rejects landing without losing flight intent");
    raven.control(pets::Control::Resume);
    bool flewLeft=false,flewRight=false;
    for(int i=0;i<60;++i) {tick(raven,now,.5);const auto state=*raven.groundedState();
      flewLeft|=state.velocity<-2;flewRight|=state.velocity>2;
      check(state.phase==pets::character::DropPhase::Airborne,"cruise turns remain airborne");
    }
    check(flewLeft&&flewRight,"cruise redirects flight before the screen boundary");
    raven.land();tick(raven,now,12);
    const double x=raven.groundedState()->rootX;
    check(raven.flyTo(x+45,70),"raven accepts powered flight");
    tick(raven,now,2);
    check(raven.groundedState()->elevation>10,"wing-powered flight gains altitude");
    raven.control(pets::Control::Hide);
    const double hidden=raven.groundedState()->elevation; tick(raven,now,5);
    check(raven.groundedState()->elevation==hidden,"hidden flight does not catch up");
    raven.control(pets::Control::Show);
    check(raven.land(),"landing command clears powered flight"); tick(raven,now,12);
    check(raven.groundedState()->phase==pets::character::DropPhase::Grounded,"powered flight lands");
    check(!raven.groundMotionActive(),"landed raven removes animation deadline");
    QString error;
    check(raven.selectCharacter(root+"/soft-creature",&error),"existing pet remains selectable");
    check(!raven.flightCapable(),"wingless pet has no flight capability");
    check(!raven.flyTo(x,70),"wingless pet rejects powered flight");
    raven.control(pets::Control::Quit);
    QTemporaryDir otherStorage; double otherNow=0, edgeY=0, edgeX=0; bool edgePresent=true;
    bool lowerPresent=false; double lowerY=0, lowerX=0, lowerMaximum=0, upperMaximum=2000;
    auto surfaces=[&](double stamp) {
      pets::WindowSurfaceSnapshot result; result.available=true;
      if(edgePresent) result.surfaces.push_back({"synthetic:window",edgeY,{{edgeX,edgeX+upperMaximum}},stamp});
      if(lowerPresent) result.surfaces.push_back({"synthetic:lower",lowerY,{{lowerX,lowerMaximum}},stamp});
      return result;
    };
    pets::ResidentApplication walker(root+"/soft-creature",otherStorage.path(),false,[&]{return otherNow;},
        pets::IpcMode::DisabledForTests,nullptr,false,[]{return std::optional<QPointF>{};},surfaces);
    const auto definition=pets::character::loadCharacter(root+"/soft-creature").definition;
    auto *w=petWindow();
    const QPoint original=w->position();
    const int margin=int(std::ceil(definition.motionEnvelope));
    edgeY=original.y()+margin+definition.locomotion->groundY-100;
    tick(walker,otherNow,.6);
    const QPoint hit=w->mask().boundingRect().center();
    const QPoint start=w->position()+hit;
    mouse(QEvent::MouseButtonPress,start,Qt::LeftButton,Qt::LeftButton);
    otherNow+=.1;
    mouse(QEvent::MouseMove,start-QPoint(0,100),Qt::NoButton,Qt::LeftButton);
    mouse(QEvent::MouseButtonRelease,start-QPoint(0,100),Qt::LeftButton,Qt::NoButton);
    tick(walker,otherNow,5);
    check(walker.groundedState()->elevation==0,"wingless pet is supported on manually selected window");
    check(petWindow()->position().y()<original.y()-80,"perched pet remains above desktop floor");
    const QPoint perched=petWindow()->position();
    edgeY+=20;edgeX+=10;tick(walker,otherNow,.7);
    check(petWindow()->position()==perched+QPoint(10,20),"moving window carries support once");
    tick(walker,otherNow,2);
    check(petWindow()->position()==perched+QPoint(10,20),"idle pet keeps its destination relative to moving support");
    walker.setGroundTarget(walker.groundedState()->rootX+25);tick(walker,otherNow,4);
    check(petWindow()->position().x()>perched.x()+20,"wingless pet walks along window edge");
    edgePresent=false;tick(walker,otherNow,.8);
    check(walker.groundedState()->phase!=pets::character::DropPhase::Grounded,"removed window releases support");
    tick(walker,otherNow,10);
    check(walker.groundedState()->phase==pets::character::DropPhase::Grounded,"wingless pet lands on desktop after support loss");
    edgePresent=true;edgeY=original.y()+margin+definition.locomotion->groundY-150;
    tick(walker,otherNow,.7);
    check(walker.selectCharacter(ravenDirectory,&error),"raven switches into shared surface world");
    edgeX=walker.groundedState()->rootX-40; upperMaximum=80;
    tick(walker,otherNow,.7);
    const double perchFloorAnchor=QGuiApplication::primaryScreen()->availableGeometry().bottom()+1-petWindow()->position().y();
    const double upperFlightStart=otherNow;
    check(walker.flyToWindow(),"raven plans a flight to observed window edge");
    for(int i=0;i<2160 && walker.groundedState()->phase==pets::character::DropPhase::Airborne;++i) {
      tick(walker,otherNow,1./120);
    }
    check(walker.groundedState()->phase==pets::character::DropPhase::Landing ||
          walker.groundedState()->phase==pets::character::DropPhase::Recovering,
          "perch touchdown exposes supported recovery");
    std::cout<<"upper perch touchdown seconds "<<otherNow-upperFlightStart<<"\n";
    check(otherNow-upperFlightStart<2.5,"nearby window touchdown completes promptly");
    check(std::abs(petWindow()->position().y()+perchFloorAnchor-edgeY)<2,"first quick touchdown must be on the requested window");
    const auto recovering=*walker.groundedState();
    const auto recoveryPosition=petWindow()->position();
    check(!walker.flyTo(recovering.rootX+25,50),"landing recovery rejects departure before detaching support");
    check(walker.groundedState()->phase==recovering.phase && petWindow()->position()==recoveryPosition,
          "rejected recovery departure preserves supported state");
    const double recoveryRoot=walker.groundedState()->rootX;
    edgeX+=10; edgeY+=10;
    tick(walker,otherNow,.7);
    tick(walker,otherNow,9.3);
    check(std::abs(walker.groundedState()->rootX-recoveryRoot-10)<2,
          "moving window during recovery retains relative walking destination");
    check(walker.groundedState()->phase==pets::character::DropPhase::Grounded,"perch approach lands and recovers");
    check(petWindow()->position().y()<original.y()-80,"raven lands on the elevated window instead of desktop");
    const int upperPerchY=petWindow()->position().y();
    lowerPresent=true; lowerY=edgeY+65;
    lowerX=walker.groundedState()->rootX-130; lowerMaximum=lowerX+80;
    tick(walker,otherNow,.7);
    walker.control(pets::Control::Pause);
    const auto pausedPerch=*walker.groundedState();
    const QPoint pausedPosition=petWindow()->position();
    check(!walker.flyToWindow(),"paused perched raven rejects flight without detaching support");
    tick(walker,otherNow,1);
    check(walker.groundedState()->phase==pausedPerch.phase &&
          walker.groundedState()->elevation==pausedPerch.elevation &&
          petWindow()->position()==pausedPosition,"paused perch state remains frozen");
    walker.control(pets::Control::Resume);
    const double lowerFlightStart=otherNow;
    check(walker.flyToWindow(),"perched raven can choose a lower window");
    double lowerTouchdown=-1;
    for(int i=0;i<2160 && walker.groundedState()->phase!=pets::character::DropPhase::Grounded;++i) {
      tick(walker,otherNow,1./120);
      if(lowerTouchdown<0 && walker.groundedState()->phase!=pets::character::DropPhase::Airborne)lowerTouchdown=otherNow-lowerFlightStart;
    }
    std::cout<<"lower perch touchdown seconds "<<lowerTouchdown<<" settled seconds "<<otherNow-lowerFlightStart<<"\n";
    check(lowerTouchdown>0 && lowerTouchdown<2.5,"nearby lower-window touchdown completes promptly");
    check(otherNow-lowerFlightStart<4.5,"quick transfer retains a brief supported recovery");
    check(walker.groundedState()->phase==pets::character::DropPhase::Grounded,
          "lower window approach lands and recovers");
    check(std::abs(petWindow()->position().y()-upperPerchY-65)<2,
          "raven reaches lower support rather than returning to upper edge");
    const double floorAnchor=lowerY-petWindow()->position().y();
    const int lowerPerchY=petWindow()->position().y();
    walker.findChild<QAction*>("fly")->trigger();
    for(int i=0;i<48;++i) {
      tick(walker,otherNow,.5);
      if(i>4)check(petWindow()->position().y()<lowerPerchY-35,
          "cruise keeps desktop-relative altitude through reversals after perched takeoff");
    }
    walker.land();tick(walker,otherNow,12);
    const double ceiling=QGuiApplication::primaryScreen()->availableGeometry().top()
        +floorAnchor-petWindow()->mask().boundingRect().bottom();
    lowerPresent=false; edgePresent=true; edgeY=ceiling+49;
    tick(walker,otherNow,.7);
    check(!walker.flyToWindow(),"perch planner rejects an approach above reachable ceiling");
    walker.control(pets::Control::Quit);
    std::cout<<"PASS raven flight and universal window support integration\n";
    return 0;
  } catch(const std::exception &e) { std::cerr<<"FAIL "<<e.what()<<'\n'; return 1; }
}
