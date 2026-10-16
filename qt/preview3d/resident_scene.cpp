#include "resident_scene.h"
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
QString resources() { return QCoreApplication::applicationDirPath()+"/better-pets-3d"; }
}
QString residentRigPath(QString asset) { return resources()+(asset=="raven"?"/raven":"")+"/rig.json"; }
QQuickItem* createResidentStage(QQmlEngine& engine,WolfMotion& motion,QString* error,QString asset) {
 engine.rootContext()->setContextProperty("motion",&motion);
 engine.rootContext()->setContextProperty("ravenModel",asset=="raven");
 engine.rootContext()->setContextProperty("assetSource",QUrl::fromLocalFile(resources()+(asset=="raven"?"/raven/RavenAsset/Raven.qml":"/WolfAsset/Werewolf.qml")));
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
 const double ppm=stage->property("logicalPixelsPerMeter").toDouble();
 QRegion result;
 if(wings_&&!skinBounds_.empty()) {
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
   // Two logical pixels cover corner rounding and the nearest sampled output.
   for(int x=-2;x<=2;++x)for(int y=-2;y<=2;++y)result|=projected.translated(x,y);
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
  const int r=int(std::ceil(radius*ppm))+2;
  result|=QRegion(QRect(int(std::floor(point.x()))-r,int(std::floor(point.y()))-r,2*r+1,2*r+1),QRegion::Ellipse);
  const auto parent=joints_.find(joint.parent);
  if(parent!=joints_.end()&&parent->second.node) {
   const auto end=projectAnchor(stage,parent->second.node->property("scenePosition").value<QVector3D>());
   const double distance=std::hypot(end.x()-point.x(),end.y()-point.y());
   const int count=std::max(1,int(std::ceil(distance/8)));
   const int limbRadius=int(std::ceil(.17*ppm))+2;
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
 const auto center=projectAnchor(stage,pelvis->second.node->property("scenePosition").value<QVector3D>()+QVector3D(0,.32,0));
 const int halfWidth=int(std::ceil(.34*ppm)),halfHeight=int(std::ceil(.50*ppm));
 return QRegion(QRect(int(center.x())-halfWidth,int(center.y())-halfHeight,2*halfWidth,2*halfHeight),QRegion::Ellipse);
}
