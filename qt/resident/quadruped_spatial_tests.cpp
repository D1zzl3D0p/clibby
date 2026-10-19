#include "application.h"
#include "spatial_character.h"
#include "desktop_host.h"
#include "characters3d/builtin_assets.h"
#include <QApplication>
#include <QAction>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QTemporaryDir>
#include <QTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(int argc,char** argv){
 QQuickWindow::setSceneGraphBackend("rhi");
 QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
 QApplication app(argc,argv);app.setQuitOnLastWindowClosed(false);
 if(argc!=2)return 2;
 const QString root=QString::fromLocal8Bit(argv[1]);
 try{
  require(pets::isSpatialPackage(root+"/marmalade-3d"),"Marmalade must be recognized as a trusted spatial package");
  {
   pets::DesktopHost host(pets::character::loadCharacter(root+"/soft-creature"));
   pets::SpatialCharacter cat(root+"/marmalade-3d");cat.install(host);cat.reset(host);cat.apply(host);
   const auto footprint=cat.supportFootprint();
   // Forefeet share longitudinal homes; considering just the first pair
   // would omit the hind stance and report only a few pixels of support.
   require(footprint.second-footprint.first>90,"spatial support footprint omits hind paws");
   host.setSupportFootprint(footprint.first,footprint.second);
  }
  // A desktop edge must leave a real path back inward, including after the
  // opaque silhouette changes during a leading-plant turn.
  for(double scale:{.5,1.,2.}) {
   pets::DesktopHost host(pets::character::loadCharacter(root+"/soft-creature"));
   pets::SpatialCharacter biscuit(root+"/biscuit-3d");biscuit.install(host);
   host.setCompanionScale(scale);biscuit.setAppearance(scale,1);biscuit.reset(host);biscuit.apply(host);
   auto tick=[&](){const auto p=biscuit.propose(1./60);const auto actual=host.moveInWorld(p.displacement,p.displacementY);
    biscuit.realize(actual.rootX,actual.blocked,actual.elevation);biscuit.apply(host);};
   for(int i=0;i<240;++i)tick();
   for(int round=0;round<2;++round){
    biscuit.target(host.groundSpan().minimum);for(int i=0;i<1800&&!biscuit.settled();++i)tick();
    const double left=biscuit.state().rootX;
    biscuit.target(host.groundSpan().maximum);for(int i=0;i<1800&&!biscuit.settled();++i)tick();
    require(biscuit.settled()&&biscuit.state().rootX>left+50,"Biscuit cannot move right after reaching the left desktop edge");
   }
  }
  {
   QTemporaryDir edgeStorage;double edgeTime=0;
   pets::ResidentApplication shell(root+"/biscuit-3d",edgeStorage.path(),false,[&]{return edgeTime;},
       pets::IpcMode::DisabledForTests,nullptr,false,[]{return std::optional<QPointF>{};});
   auto *left=shell.findChild<QAction*>("walkLeft"),*right=shell.findChild<QAction*>("walkRight");
   require(left&&right,"Biscuit has both context-menu walking actions");
   auto settle=[&](){for(int i=0;i<1800&&shell.groundMotionActive();++i){
    edgeTime+=1./60;require(QMetaObject::invokeMethod(shell.findChild<QTimer*>(),"timeout",Qt::DirectConnection),"Biscuit edge action timer");}};
   for(int round=0;round<2;++round){
    left->trigger();settle();const double leftRoot=shell.groundedState()->rootX;
    right->trigger();settle();
    require(!shell.groundMotionActive()&&shell.groundedState()->rootX>leftRoot+50,
            "Biscuit context-menu Walk right fails after Walk left");
   }
  }
  QTemporaryDir storage;double now=0;
  {
   pets::ResidentApplication shell(root+"/soft-creature",storage.path(),false,[&]{return now;},pets::IpcMode::DisabledForTests,nullptr,false,[]{return std::optional<QPointF>{};});
   QString error;
   require(shell.selectCharacter(root+"/marmalade-3d",&error),error.toUtf8().constData());
   require(shell.selectedCharacter()=="marmalade-3d","cat selected identity");
   require(shell.groundedState()->feet.size()==4,"resident must expose four actual feet");
   auto tick=[&]{now+=1./60;require(QMetaObject::invokeMethod(shell.findChild<QTimer*>(),"timeout",Qt::DirectConnection),"resident timer callback");};
   auto initial=*shell.groundedState();
   shell.setWalkingSpeed(1);
   shell.setGroundTarget(initial.rootX+75);
   for(int frame=0;frame<120;++frame)tick();
   require(shell.groundedState()->rootX>initial.rootX+10,"spatial cat advances through resident host");
   shell.control(pets::Control::Pause);const auto frozen=*shell.groundedState();
   for(int frame=0;frame<30;++frame)tick();
   const auto paused=*shell.groundedState();
   for(std::size_t i=0;i<4;++i)require(frozen.feet[i].position.x==paused.feet[i].position.x&&frozen.feet[i].position.y==paused.feet[i].position.y,"pause freezes every spatial paw");
   require(!shell.selectCharacter(root+"/unknown-3d",&error)&&shell.selectedCharacter()=="marmalade-3d","failed selection retains live cat");
   require(shell.selectCharacter(root+"/marmalade",&error),"switch back to planar cat");
   require(shell.groundedState()->feet.size()==4,"planar cat retains four feet after spatial switch");
   require(shell.selectCharacter(root+"/marmalade-3d",&error),"switch back to spatial cat");
   int selectedCatalogue=0;
   for(const auto& asset:pets::character3d::builtinAssets){
    if(!asset.catalogueStyle||QString(asset.asset)=="marmalade")continue;
    const auto directory=root+"/"+QString::fromLatin1(asset.id);
    require(pets::isSpatialPackage(directory),"new catalogue descriptor not recognized");
    require(shell.selectCharacter(directory,&error),error.toUtf8().constData());
    require(shell.selectedCharacter()==QLatin1String(asset.id),"new catalogue identity not selected");
    require(shell.groundedState()->feet.size()==(asset.quadruped?4:2),"new catalogue selected wrong anatomy");
    ++selectedCatalogue;
   }
   require(selectedCatalogue==31,"resident did not exercise all thirty-one new catalogue selections");
   require(shell.selectCharacter(root+"/marmalade-3d",&error),"restore spatial cat after catalogue selection");
  }
  {
   pets::ResidentApplication restored(root+"/soft-creature",storage.path(),false,[&]{return now;},pets::IpcMode::DisabledForTests,nullptr,true,[]{return std::optional<QPointF>{};});
   require(restored.selectedCharacter()=="marmalade-3d"&&restored.groundedState()->feet.size()==4,"remembered spatial quadruped restores");
  }
  std::cout<<"resident four-foot cat selection, navigation, pause, switching and restore verified\n";
  return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
