#include "resident_scene.h"
#include "characters3d/builtin_assets.h"
#include <QCoreApplication>
#include <QDir>
#include <QQmlComponent>
#include <QQmlContext>
#include <QUrl>
#include <QMatrix4x4>
#include <QPolygon>
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace {
QString resources() { return QCoreApplication::applicationDirPath()+"/clibby-3d"; }
}
QString residentRigPath(QString asset) {
 if(!pets::character3d::builtinAsset(asset))throw std::runtime_error("Unknown built-in 3D asset");
 return resources()+(asset=="werewolf"?QString():"/"+asset)+"/rig.json";
}
QQuickItem* createResidentStage(QQmlEngine& engine,WolfMotion& motion,QString* error,QString asset) {
 engine.rootContext()->setContextProperty("motion",&motion);
 engine.rootContext()->setContextProperty("ravenModel",asset=="raven");
 engine.rootContext()->setContextProperty("marmaladeModel",asset=="marmalade");
 const auto* info=pets::character3d::builtinAsset(asset);
 if(!info)throw std::runtime_error("Unknown built-in 3D asset");
 engine.rootContext()->setContextProperty("catalogueModel",info->catalogueStyle);
 // A camera target offset is measured in pixels; facing uses the rig's yaw angle.
 engine.rootContext()->setContextProperty("catalogueFacingBias",info->quadruped?motion.authoredFacingBiasDegrees():-35.);
 const auto type=QString::fromLatin1(info->type);
 const QString source=asset=="werewolf"?"/WolfAsset/Werewolf.qml":"/"+asset+"/"+type+"Asset/"+type+".qml";
 engine.rootContext()->setContextProperty("assetSource",QUrl::fromLocalFile(resources()+source));
 QQmlComponent component(&engine,QUrl::fromLocalFile(resources()+"/ResidentStage.qml"));
 auto item=qobject_cast<QQuickItem*>(component.create());
 if(!item) { if(error)*error=component.errorString();return nullptr; }
 try { motion.bind(item); }
 catch(const std::exception& e) { if(error)*error=QString::fromUtf8(e.what());delete item;return nullptr; }
 return item;
}
QPointF WolfMotion::projectAnchor(QQuickItem* stage,QVector3D point) const {
 QVariant projected;
 if(!stage||!QMetaObject::invokeMethod(stage,"project",Q_RETURN_ARG(QVariant,projected),Q_ARG(QVariant,QVariant::fromValue(point))))return {};
 return projected.toPointF();
}
QRegion WolfMotion::projectedVisualRegion(QQuickItem* stage) const {
 if(!stage)return {};
 const int fringe=std::max(2,stage->property("pixelFringe").toInt());
 const double ppm=stage->property("logicalPixelsPerMeter").toDouble();
 QRegion result;
 if(!skinBounds_.empty()) {
  // Blended vertices lie in the convex hull of their influence boxes. A group
  // hull includes the skin bridges without a false all-directions wing radius.
  for(const auto& group:skinBounds_){
   std::vector<QPointF> points;
   for(const auto& bound:group){
    const auto& joint=joints_.at(bound.joint);if(!joint.node)continue;
    const auto transform=joint.node->property("sceneTransform").value<QMatrix4x4>();
    for(int corner=0;corner<8;++corner){
     const QVector3D local(corner&1?bound.maximum.x():bound.minimum.x(),corner&2?bound.maximum.y():bound.minimum.y(),corner&4?bound.maximum.z():bound.minimum.z());
     points.push_back(projectAnchor(stage,transform.map(local)));
    }
   }
   std::sort(points.begin(),points.end(),[](QPointF a,QPointF b){return a.x()<b.x()||(a.x()==b.x()&&a.y()<b.y());});
   points.erase(std::unique(points.begin(),points.end()),points.end());
   if(points.size()<3)continue;
   auto cross=[](QPointF a,QPointF b,QPointF c){return (b.x()-a.x())*(c.y()-a.y())-(b.y()-a.y())*(c.x()-a.x());};
   std::vector<QPointF> hull;
   for(const auto p:points){while(hull.size()>1&&cross(hull[hull.size()-2],hull.back(),p)<=0)hull.pop_back();hull.push_back(p);}
   const auto lower=hull.size();
   for(auto i=points.rbegin()+1;i!=points.rend();++i){while(hull.size()>lower&&cross(hull[hull.size()-2],hull.back(),*i)<=0)hull.pop_back();hull.push_back(*i);}
   QPolygon polygon;for(const auto point:hull)polygon<<point.toPoint();
   const QRegion projected(polygon,Qt::WindingFill);
   // Include the full nearest-sampled pixel block beyond the geometric hull.
   for(const auto &rect:projected) result|=rect.adjusted(-fringe,-fringe,fringe,fringe);
  }
  return result.intersected(QRect(0,0,int(stage->width()),int(stage->height())));
 }
 // Authored mesh envelopes around live deform joints. Broad circles include
 // cheeks/ears, waved paws and the tapered tail without reading GPU pixels.
 for(const auto& [name,joint]:joints_) {
  if(!joint.node)continue;
  double radius=.19;
  if(wings_) {
   if(name=="head")radius=.65;
   else if(name=="spine"||name=="pelvis")radius=.76;
   else if(name.startsWith("wing_wrist"))radius=1.35;
   else if(name.startsWith("wing_"))radius=.58;
   else if(name.startsWith("tail"))radius=.70;
  }
  if(!wings_ && name=="head")radius=.50;
  else if(!wings_ && (name=="spine"||name=="pelvis"))radius=.44;
  else if(!wings_ && name.startsWith("tail"))radius=.27;
  else if(name.startsWith("hand")||name.startsWith("foot"))radius=.25;
  const auto point=projectAnchor(stage,joint.node->property("scenePosition").value<QVector3D>());
  const int r=int(std::ceil(radius*ppm))+fringe;
  result|=QRegion(QRect(int(std::floor(point.x()))-r,int(std::floor(point.y()))-r,2*r+1,2*r+1),QRegion::Ellipse);
  const auto parent=joints_.find(joint.parent);
  if(parent!=joints_.end()&&parent->second.node) {
   const auto end=projectAnchor(stage,parent->second.node->property("scenePosition").value<QVector3D>());
   const double distance=std::hypot(end.x()-point.x(),end.y()-point.y());
   const int count=std::max(1,int(std::ceil(distance/8)));
   const int limbRadius=int(std::ceil(.17*ppm))+fringe;
   for(int sample=1;sample<count;++sample) {
    const auto center=point+(end-point)*(double(sample)/count);
    result|=QRegion(QRect(int(center.x())-limbRadius,int(center.y())-limbRadius,2*limbRadius+1,2*limbRadius+1),QRegion::Ellipse);
   }
  }
 }
 return result.intersected(QRect(0,0,int(stage->width()),int(stage->height())));
}
QRegion WolfMotion::projectedGrabRegion(QQuickItem* stage) const {
 if(!stage)return {};
 const double ppm=stage->property("logicalPixelsPerMeter").toDouble();
 auto pelvis=joints_.find("pelvis");
 if(pelvis==joints_.end()||!pelvis->second.node)return {};
 QRegion result;
 auto envelope=[&](QPointF center,double width,double height){
  const int halfWidth=int(std::ceil(width*ppm)),halfHeight=int(std::ceil(height*ppm));
  result|=QRegion(QRect(int(center.x())-halfWidth,int(center.y())-halfHeight,
                       2*halfWidth,2*halfHeight),QRegion::Ellipse);
 };
 // Retain the established upright torso target, then follow the actual core
 // anatomy. A long quadruped's head is well outside the old pelvis ellipse.
 envelope(projectAnchor(stage,pelvis->second.node->property("scenePosition").value<QVector3D>()
                        +QVector3D(0,.32,0)),.34,.50);
 QPointF previous;bool havePrevious=false;
 for(const char* role:{"pelvis","spine","head"}){
  const auto joint=joints_.find(role);
  if(joint==joints_.end()||!joint->second.node){havePrevious=false;continue;}
  const auto point=projectAnchor(stage,joint->second.node->property("scenePosition").value<QVector3D>());
  const bool head=joint->first=="head";
  envelope(point,head?.42:.34,head?.42:.50);
  if(havePrevious){
   const double distance=std::hypot(point.x()-previous.x(),point.y()-previous.y());
   const int samples=std::clamp(int(std::ceil(distance/std::max(1.,.34*ppm))),1,64);
   for(int sample=1;sample<samples;++sample)
    envelope(previous+(point-previous)*(double(sample)/samples),.34,.42);
  }
  previous=point;havePrevious=true;
 }
 // Deliberately exclude wings, limbs, tails and owned props: their visual
 // envelopes must not turn remote decorations into pickup targets.
 return result.intersected(QRect(0,0,int(stage->width()),int(stage->height())));
}
