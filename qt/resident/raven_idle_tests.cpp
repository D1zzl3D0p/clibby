#include "application.h"
#include "characters/loader.h"
#include <QApplication>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QScreen>
#include <QSGRendererInterface>
#include <QTimer>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
QQuickWindow* petWindow(){for(auto* w:QGuiApplication::topLevelWindows())if(w->title()=="Clibby")return dynamic_cast<QQuickWindow*>(w);throw std::runtime_error("pet window missing");}
void tick(pets::ResidentApplication& app,double& now,double seconds){for(int i=0;i<int(std::ceil(seconds*120));++i){now+=1./120;auto* timer=app.findChild<QTimer*>();check(timer&&QMetaObject::invokeMethod(timer,"timeout",Qt::DirectConnection),"resident timer missing");}}
}
int main(int argc,char** argv){
 const bool spatial=argc==3 && QString::fromLocal8Bit(argv[2])=="--3d";
 if(spatial){QQuickWindow::setSceneGraphBackend("rhi");QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);}
 QApplication gui(argc,argv);gui.setQuitOnLastWindowClosed(false);if(argc!=2&&!spatial)return 2;try{
 const QString root=QString::fromLocal8Bit(argv[1]);
 QTemporaryDir storage;double now=0,edgeY=-10000;bool windows=true;
 auto surfaces=[&](double stamp){pets::WindowSurfaceSnapshot s;s.available=true;if(windows)s.surfaces.push_back({"idle-window",edgeY,{{-2000,2000}},stamp});return s;};
 pets::ResidentApplication raven(root+(spatial?"/raven-3d":"/raven"),storage.path(),false,[&]{return now;},pets::IpcMode::DisabledForTests,nullptr,false,[]{return std::optional<QPointF>{};},surfaces);
 const int desktopY=petWindow()->position().y();
 const double floorOffset=QGuiApplication::primaryScreen()->availableGeometry().bottom()+1-desktopY;
 edgeY=desktopY+floorOffset-65;
 raven.setGroundTarget(raven.groundedState()->rootX+5);
 bool idleFlight=false;
 for(int frame=0;frame<1440&&!idleFlight;++frame){tick(raven,now,1./120);idleFlight=raven.groundedState()->poweredFlight;}
 check(idleFlight,"settled manual navigation should return to idle perching within twelve seconds");
 for(int frame=0;frame<2400&&raven.groundedState()->phase!=pets::character::DropPhase::Grounded;++frame)tick(raven,now,1./120);
 check(raven.groundedState()->phase==pets::character::DropPhase::Grounded&&petWindow()->position().y()<desktopY-50,"idle perch flight should land on the observed window");
 for(int frame=0;frame<4800;++frame){tick(raven,now,1./120);check(raven.groundedState()->phase==pets::character::DropPhase::Grounded,"perched idle raven should stay instead of repeatedly hopping between windows");}
 // Remove the perch, then prove idle intent does not launch under suppression.
 windows=false;tick(raven,now,12);windows=true;
 edgeY=petWindow()->position().y()+floorOffset-65;
 raven.control(pets::Control::Pause);const auto frozen=*raven.groundedState();tick(raven,now,20);
 check(raven.groundedState()->rootX==frozen.rootX&&raven.groundedState()->elevation==frozen.elevation&&!raven.groundedState()->poweredFlight,"paused raven must not start idle perching");
 raven.control(pets::Control::Resume);idleFlight=false;
 for(int frame=0;frame<1440&&!idleFlight;++frame){tick(raven,now,1./120);idleFlight=raven.groundedState()->poweredFlight;}
 check(idleFlight,"idle perching should resume after pause without a long stale cooldown");
 windows=false;raven.land();tick(raven,now,12);
 for(int frame=0;frame<7200;++frame){tick(raven,now,1./120);check(raven.groundedState()->phase==pets::character::DropPhase::Grounded&&!raven.groundedState()->poweredFlight,"unavailable windows should keep idle retries quiet without fallback hops");}
 windows=true;edgeY=petWindow()->position().y()+floorOffset-65;
 raven.control(pets::Control::Hide);const auto hidden=*raven.groundedState();tick(raven,now,15);
 check(raven.groundedState()->rootX==hidden.rootX&&!raven.groundedState()->poweredFlight,"hidden raven must not start idle perching");
 raven.control(pets::Control::Show);
 raven.control(pets::Control::BeginDrag);tick(raven,now,15);
 check(raven.groundedState()->phase==pets::character::DropPhase::Held&&!raven.groundedState()->poweredFlight,"dragging suppresses overdue idle perch intent");
 raven.control(pets::Control::EndDrag);tick(raven,now,7.5);
 check(raven.handleRequest("session.update",{{"source_id","idle-test"},{"session_id","reaction"},{"label","Done"},{"seq",1},{"status","completed"},{"ttl",120}}).errorCode==0,"idle test reaction accepted");
 tick(raven,now,1);check(!raven.groundedState()->poweredFlight,"reaction suppresses the due idle perch opportunity");
 // Busy reactions defer the due idle opportunity rather than dropping it
 // for the entire normal cooldown; the raven resumes once they finish.
 idleFlight=false;for(int frame=0;frame<1440&&!idleFlight;++frame){tick(raven,now,1./120);idleFlight=raven.groundedState()->poweredFlight;}
 check(idleFlight,"idle perch retries promptly after drag and reaction finish");
 raven.control(pets::Control::Quit);
 std::cout<<"idle perching after manual navigation, perched dwell and suppression passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}return 0;}
