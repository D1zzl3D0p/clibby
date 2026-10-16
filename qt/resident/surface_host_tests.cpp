#include "desktop_host.h"
#include <QApplication>
#include <QQuickItem>
#include <QScreen>
#include <iostream>
#include <stdexcept>
using namespace pets;
void check(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
int main(int argc,char** argv) {
  QApplication app(argc,argv);
  try {
    check(argc==2,"character directory required");
    auto loaded=character::loadCharacter(QString::fromLocal8Bit(argv[1]));
    DesktopHost host(loaded);
    const auto area=QGuiApplication::primaryScreen()->availableGeometry();
    const double edgeY=area.top()+300;
    host.restorePlacement({area.left()+150,qRound(edgeY-host.localGroundAnchor().y()+5)},host.displayName());
    const double root=host.groundRootX();
    const double desktop=host.floorWorldY();
    character::SupportSurface edge{"synthetic:window",edgeY,{{root-100,root+100}},1};
    host.setSurfaceObservations({edge},1);
    check(host.acquireSupportAt(1),"ordinary nonflying pet accepts manual window placement");
    check(host.floorWorldY()==edgeY && host.elevation()==0,"selected edge is the support reference");
    auto walk=host.moveOnGround(12);
    check(walk.rootX==root+12 && !walk.blocked,"nonflying pet walks on edge");
    auto moved=edge; moved.topY+=10; moved.segments[0].minimum+=15; moved.segments[0].maximum+=15; moved.observedAt=1.5;
    const auto before=host.position();
    host.setSurfaceObservations({moved},1.5);
    check(host.position()==before+QPoint(15,10),"window translation moves pet exactly once");
    host.setSurfaceObservations({moved},1.6);
    check(host.position()==before+QPoint(15,10),"unchanged sample cannot double-translate pet");
    const auto perched=host.position();
    int lost=0; host.supportChanged=[&](bool removed){if(removed) ++lost;};
    host.setSurfaceObservations({},2);
    check(lost==1 && host.position()==perched && host.floorWorldY()==desktop && host.elevation()>0,"support removal preserves release height above real desktop");
    edge.observedAt=2; host.setSurfaceObservations({edge},2);
    check(!host.acquireDescendingSupport(edgeY+20,edgeY-20,2),"ascending motion passes through top");
    check(host.acquireDescendingSupport(edgeY-20,edgeY+20,2),"descending swept feet acquire edge");
    host.moveInWorld(0,100);
    check(host.groundWorldY()==edgeY,"crossing is clamped to chosen support");
    host.setSurfaceObservations({edge},4);
    check(host.selectedSurfaceId().empty(),"stale observation releases support");
    edge.observedAt=4; edge.segments={{host.groundRootX()-10,host.groundRootX()+10}};
    host.setSurfaceObservations({edge},4);
    check(!host.acquireSupportAt(4),"edge narrower than stance declines placement");
    const double diagonalStart=host.groundRootX();
    edge.segments={{diagonalStart+50,diagonalStart+130}};
    host.setSurfaceObservations({edge},4);
    check(host.acquireDescendingSupport(edgeY-20,edgeY+20,4,diagonalStart+180),"diagonal sweep acquires by crossing X instead of initial X");
    host.moveInWorld(180,40);
    check(host.groundRootX()==diagonalStart+90 && host.groundWorldY()==edgeY,"swept landing realizes crossing position once");
    // A tall synthetic support frame exercises the host realization path
    // independently of the machine's physical display height.
    const double tallFloor=host.groundWorldY()+1600;
    edge.topY=tallFloor;edge.segments={{host.groundRootX()-100,host.groundRootX()+100}};
    host.setSurfaceObservations({edge},4);
    check(host.acquireDescendingSupport(tallFloor-1600,tallFloor,4),"large swept travel selects its actual support");
    host.moveInWorld(0,1600);
    check(host.groundWorldY()==tallFloor && host.elevation()==0,
          "host realizes a large swept touchdown without an artificial per-frame stop");
    DesktopHost ceilingHost(loaded);
    ceilingHost.moveInWorld(0,-2000);
    const double ceilingY=ceilingHost.minimumGroundWorldY();
    character::SupportSurface above{"synthetic:above-ceiling",ceilingY-8,
        {{ceilingHost.groundRootX()-100,ceilingHost.groundRootX()+100}},5};
    ceilingHost.setSurfaceObservations({above},5);
    check(!ceilingHost.acquireSupportAt(5),"manual snap tolerance cannot select support above reachable ceiling");
    DesktopHost fractional(loaded);
    fractional.setPresentation(new QQuickItem,{200,200},{100,100.5},
        QRegion(QRect(90,30,20,130)),QRegion(QRect(50,20,100,160)));
    const double fractionalEdge=area.top()+400;
    fractional.restorePlacement({area.left()+200,qRound(fractionalEdge-101)},fractional.displayName());
    fractional.moveInWorld(0,.4);
    check(fractional.groundWorldY()==fractionalEdge-.5,"fractional anchor holds its first subpixel request");
    character::SupportSurface fractionalSurface{"synthetic:fractional",fractionalEdge,
        {{fractional.groundRootX()-50,fractional.groundRootX()+50}},6};
    fractional.setSurfaceObservations({fractionalSurface},6);
    check(!fractional.acquireDescendingSupport(fractional.groundWorldY(),fractional.groundWorldY()+.2,6),
          "raw displacement alone misses the fractional presentation crossing");
    const auto predicted=fractional.predictedWorldPosition(0,.2);
    check(predicted.y()==fractionalEdge+.5,"prediction includes accumulated fractional motion and pixel rounding");
    check(fractional.acquireDescendingSupport(fractional.groundWorldY(),predicted.y(),6,predicted.x()),
          "descending sweep acquires edge crossed by actual quantized presentation");
    fractional.moveInWorld(0,.2);
    check(fractional.selectedSurfaceId()==fractionalSurface.id && fractional.elevation()==0,
          "fractional anchor lands on window instead of passing through to desktop");
    fractional.clearSupport();
    fractional.restorePlacement({area.left()+200,qRound(fractionalEdge-100)},fractional.displayName());
    fractionalSurface.topY=fractionalEdge+.75;
    fractional.setSurfaceObservations({fractionalSurface},6);
    check(fractional.acquireDescendingSupport(fractionalEdge+.5,fractionalEdge+1.5,6),
          "subpixel surface accepts a real descending crossing");
    fractional.moveInWorld(0,1);
    check(fractional.elevation()==0,"fractional surface contact starts supported");
    fractional.moveInWorld(0,0);
    check(fractional.elevation()==0,"stationary host cannot round contact back above fractional support");
    fractionalSurface.topY+=1;fractionalSurface.observedAt=6.5;
    fractional.setSurfaceObservations({fractionalSurface},6.5);
    check(fractional.elevation()==0,"moving fractional support cannot round a planted pet into airborne state");
    fractional.rebaseGround();
    check(fractional.elevation()==0,"rebase preserves contact on fractional support");
    fractional.reconcileScreens();
    check(fractional.elevation()==0,"screen repair preserves contact on fractional support");
    DesktopHost visualHost(loaded);
    visualHost.setPresentation(new QQuickItem,{600,600},{300,500},
        QRegion(QRect(280,300,40,150)),QRegion(QRect(280,250,180,250)));
    visualHost.setPetVisible(true);
    app.processEvents();
    const auto rightSpan=visualHost.desktopSpan();
    check(rightSpan.minimum==area.left()+20 && rightSpan.maximum==area.x()+area.width()-160,
          "travel bounds use actual right-facing silhouette instead of transparent canvas");
    visualHost.moveInWorld(10000,0);
    check(visualHost.position().x()+460<=area.x()+area.width(),"right-facing head stays inside screen edge");
    visualHost.restorePlacement({area.x()+area.width()-320,area.top()+20},visualHost.displayName());
    check(visualHost.position().x()+460<=area.x()+area.width(),"manual placement also keeps a right-facing head on screen");
    visualHost.updatePresentationMask(QRegion(QRect(140,250,180,250)));
    visualHost.moveInWorld(-10000,0);
    check(visualHost.position().x()+140>=area.left(),"left-facing head stays inside screen edge");
    visualHost.restorePlacement({area.left()-220,area.top()+20},visualHost.displayName());
    check(visualHost.position().x()+140>=area.left(),"manual placement also keeps a left-facing head on screen");
    visualHost.moveInWorld(0,-10000);
    app.processEvents();
    check(std::abs(visualHost.groundWorldY()-(area.top()+250))<1,
          "transparent top padding does not limit near-top placement");
    DesktopHost oversized(loaded);
    oversized.setPresentation(new QQuickItem,{300,area.height()+100},{150,500},
        QRegion(QRect(100,100,50,100)),QRegion(QRect(50,0,200,area.height()+10)));
    const auto huge=oversized.groundSpan();
    check(huge.minimum==huge.maximum,"a genuinely oversized silhouette disables grounded navigation");
    std::cout<<"PASS shared window placement, walking, movement, removal and swept landing\n";
    return 0;
  } catch(const std::exception& e) { std::cerr<<"FAIL "<<e.what()<<'\n'; return 1; }
}
