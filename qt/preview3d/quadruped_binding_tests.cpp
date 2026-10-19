#include "controller.h"
#include "characters3d/builtin_assets.h"
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <stdexcept>
#include <iostream>

namespace {
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
QJsonArray point(double x,double y,double z){return {x,y,z};}
QJsonObject fixture(){
 QJsonArray joints;
 auto joint=[&](QString name,QString parent,QJsonArray head){joints.append(QJsonObject{{"name",name},{"parent",parent},{"head",head}});};
 joint("root","",point(0,0,0));joint("pelvis","root",point(0,0,.8));
 joint("spine","pelvis",point(.2,0,.85));joint("head","spine",point(.8,0,1.2));
 joint("tail_base","pelvis",point(-.8,0,.9));
 QJsonObject contacts;
 for(auto role:{QString("foreLeft"),QString("foreRight"),QString("hindLeft"),QString("hindRight")}){
  double x=role.startsWith("fore")?.55:-.55,y=role.endsWith("Left")?-.2:.2;
  joint("thigh_"+role,"pelvis",point(x,y,.8));
  joint("shin_"+role,"thigh_"+role,point(x-.12,y,.4));
  joint("foot_"+role,"shin_"+role,point(x,y,.1));
  contacts[role]=point(x,y,0);
 }
 return {{"gait","quadrupedWalk"},{"skin","MarmaladeSkin"},{"joints",joints},{"contacts",contacts}};
}
void save(QString path,QJsonObject rig){QFile file(path);require(file.open(QIODevice::WriteOnly),"fixture write failed");file.write(QJsonDocument(rig).toJson());}
void rejects(QString path,QJsonObject rig,const char* message){save(path,rig);bool rejected=false;try{WolfMotion motion(path);}catch(const std::exception&){rejected=true;}require(rejected,message);}
}
int main(int argc,char** argv){
 QCoreApplication app(argc,argv);
 try{
  QTemporaryDir directory;require(directory.isValid(),"temporary directory unavailable");const auto path=directory.filePath("rig.json");
  auto rig=fixture();save(path,rig);WolfMotion cat(path);
  require(cat.motion().pose().legs.size()==4,"quadruped rig did not load all four chains");
  require(cat.leftGrip()==cat.headAnchor(),"quadruped grip must fall back to head");
  cat.gesture("wave");for(int i=0;i<200;++i)cat.advance(1./120);
  require(!cat.motion().pose().arms[0]&&!cat.motion().pose().arms[1],"cat gesture acquired human arms");
  cat.reach(0,{.1f,1.f,.2f});require(!cat.motion().pose().arms[0],"cat reach acquired an arm");
  // A loaded rig must expose real paired travel/turn support, rather than a
  // test-only four-chain pose that never participates in coordinated stepping.
  bool sawPair=false;
  for(double target:{35.,-20.}){
   cat.target(target);
   for(int tick=0;tick<6000&&!cat.settled();++tick){
    cat.advance(1./120);const auto pose=cat.motion().pose();int lifted=0,supported=0;
    for(int i=0;i<4;++i){lifted+=pose.swing[i];supported+=pose.grounded[i];}
    require(lifted<=2&&supported>=2,"loaded quadruped loses coordinated support");
    if(lifted==2){require(!((pose.swing[0]&&pose.swing[2])||(pose.swing[1]&&pose.swing[3])),"loaded quadruped lifts both paws on one side");sawPair=true;}
   }
   require(cat.settled(),"loaded quadruped travel/reversal never settles");
  }
  require(sawPair,"loaded quadruped never exposes paired placements");
  QFile legacyFile(QCoreApplication::applicationDirPath()+"/../clibby-3d/rig.json");
  require(legacyFile.open(QIODevice::ReadOnly),"legacy biped fixture unavailable");
  auto legacyRig=QJsonDocument::fromJson(legacyFile.readAll()).object();legacyRig.remove("gait");save(path,legacyRig);
  WolfMotion legacy(path);require(legacy.motion().pose().legs.size()==2,"omitted gait no longer defaults to legacy biped");
  auto objectRig=legacyRig;objectRig["arms"]=false;objectRig["skin"]="SprigSkin";
  QJsonArray objectJoints;
  for(const auto value:objectRig["joints"].toArray()){
   const auto name=value.toObject()["name"].toString();
   if(name.startsWith("upper_arm_")||name.startsWith("forearm_")||name.startsWith("hand_")||name.startsWith("ear_")||name.startsWith("tail_"))continue;
   objectJoints.append(value);
  }
  objectRig["joints"]=objectJoints;save(path,objectRig);WolfMotion object(path);
  require(!object.motion().expressions().supports(pets::character::Role::EarLeft)&&!object.motion().expressions().supports(pets::character::Role::Tail),"absent object anatomy advertised as expressive capabilities");
  require(!object.motion().reach(0,{.2,.5,.1}),"armless biped acquired reach");
  object.gesture("wave");for(int i=0;i<300;++i)object.advance(1./120);
  require(!object.motion().pose().arms[0],"object gesture activated absent arm");
  require(pets::character3d::builtinAsset("pip")&&pets::character3d::builtinIdentity("rivet-3d"),"trusted catalogue identity missing");
  require(!pets::character3d::builtinAsset("../rivet")&&!pets::character3d::builtinIdentity("velvet-3d"),"path or experimental family admitted as production built-in");
  auto explicitNull=rig;explicitNull["gait"]=QJsonValue::Null;rejects(path,explicitNull,"explicit null gait accepted");
  auto invalid=rig;invalid["gait"]="crawl";rejects(path,invalid,"unknown gait accepted");
  invalid=rig;auto contacts=invalid["contacts"].toObject();contacts.remove("hindRight");invalid["contacts"]=contacts;rejects(path,invalid,"missing support role accepted");
  invalid=rig;contacts=invalid["contacts"].toObject();contacts["left"]=point(0,0,0);invalid["contacts"]=contacts;rejects(path,invalid,"extra support role accepted");
  invalid=rig;auto joints=invalid["joints"].toArray();joints.append(joints[0]);invalid["joints"]=joints;rejects(path,invalid,"duplicate joint accepted");
  invalid=rig;joints=invalid["joints"].toArray();auto item=joints[0].toObject();item["head"]=QJsonArray{"bad",0,0};joints[0]=item;invalid["joints"]=joints;rejects(path,invalid,"nonnumeric point accepted");
  invalid=rig;joints=invalid["joints"].toArray();item=joints[0].toObject();item["parent"]="missing";joints[0]=item;invalid["joints"]=joints;rejects(path,invalid,"unknown parent accepted");
  invalid=rig;invalid["skin"]="UnknownSkin";rejects(path,invalid,"mismatched quadruped skin accepted");
  std::cout<<"quadruped binding validation passed\n";return 0;
 }catch(const std::exception& error){std::cerr<<error.what()<<"\n";return 1;}
}
