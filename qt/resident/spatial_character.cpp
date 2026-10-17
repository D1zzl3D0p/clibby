#include "spatial_character.h"
#include "desktop_host.h"
#include "characters3d/builtin_assets.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <stdexcept>
#include <cmath>
#ifdef PET_RESIDENT_3D
#include "preview3d/controller.h"
#include "preview3d/resident_scene.h"
#include <QQmlEngine>
#include <QPointer>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSGRendererInterface>
#endif

namespace pets {
bool isSpatialPackage(const QString &directory) {
  QFileInfo info(directory + "/manifest.json");
  if (!info.isFile() || info.isSymLink() || info.size() > 1024*1024) return false;
  QFile file(info.absoluteFilePath());
  if (!file.open(QIODevice::ReadOnly)) return false;
  const auto object = QJsonDocument::fromJson(file.read(1024*1024+1)).object();
  if (object.value("kind").toString() != "builtin-3d") return false;
  const auto* asset=character3d::builtinIdentity(object.value("id").toString());
  if (object.value("version").toInt() != 1 || !asset || object.value("asset").toString()!=QLatin1String(asset->asset))
    throw std::runtime_error("Unsupported built-in 3D character descriptor");
  return true;
}
static QString builtinAsset(const QString& directory) {
 if(directory.isEmpty())return "werewolf";
 if(!isSpatialPackage(directory))throw std::runtime_error("Not a built-in 3D character descriptor");
 QFile file(directory+"/manifest.json");if(!file.open(QIODevice::ReadOnly))throw std::runtime_error("Cannot read 3D character descriptor");
 return QJsonDocument::fromJson(file.read(1024*1024+1)).object().value("asset").toString();
}
#ifdef PET_RESIDENT_3D
struct SpatialCharacter::Impl {
  QQmlEngine engine;
  WolfMotion binding;
  QPointer<QQuickItem> item;
  QPointer<QObject> camera;
  QPointer<QObject> avatar;
  std::unique_ptr<QQuickItem> prepared;
  double pixelsPerCentimeter = 1.2;
  double verticalPixelsPerCentimeter = 1.2;
  Impl(const QString& asset) : binding(residentRigPath(asset)) {
    QString error;
    prepared.reset(createResidentStage(engine, binding, &error, asset));
    if (!prepared) throw std::runtime_error(error.toStdString());
    item = prepared.get();
    camera = item->findChild<QObject*>("camera");
    avatar = item->findChild<QObject*>("avatar");
    if (!camera || !avatar) throw std::runtime_error("Missing 3D gaze coordinate frame");
    binding.motion().setFacingBias(item->property("facingBiasDegrees").toDouble()*3.141592653589793/180.);
    binding.motion().setCameraFacingTurns(true);
    pixelsPerCentimeter = item->property("logicalPixelsPerMeter").toDouble()/100.;
    if (!std::isfinite(pixelsPerCentimeter) || pixelsPerCentimeter <= 0)
      throw std::runtime_error("Invalid 3D projection calibration");
    // World-up projects shorter under the pitched orthographic camera.
    const auto tilt=item->property("cameraTiltDegrees").toDouble()*3.141592653589793/180.;
    verticalPixelsPerCentimeter=pixelsPerCentimeter*std::cos(tilt);
    if (!std::isfinite(verticalPixelsPerCentimeter)||verticalPixelsPerCentimeter<=0)
      throw std::runtime_error("Invalid 3D vertical projection calibration");
  }
};
SpatialCharacter::SpatialCharacter(QString directory) {
  if (qEnvironmentVariable("QT_QUICK_BACKEND") == "software" || QQuickWindow::sceneGraphBackend() == "software"
      || QQuickWindow::graphicsApi() == QSGRendererInterface::Software)
    throw std::runtime_error("3D requires a GPU renderer; the software renderer keeps the current pet");
  impl_ = std::make_unique<Impl>(builtinAsset(directory));
}
SpatialCharacter::~SpatialCharacter() = default;
void SpatialCharacter::install(DesktopHost &host) {
  host.setPresentation(impl_->prepared.get(), {qRound(impl_->item->width()),qRound(impl_->item->height())},
      impl_->item->property("rootFloorAnchor").toPointF(),
      impl_->binding.projectedGrabRegion(impl_->item), impl_->binding.projectedVisualRegion(impl_->item));
  impl_->prepared.release();
}
void SpatialCharacter::reset(const DesktopHost &host) {
  impl_->binding.cancelSignature(true);
  auto span = host.groundSpan();
  const auto scale = impl_->pixelsPerCentimeter;
  impl_->binding.motion().reset(host.groundRootX()/scale, {span.minimum/scale, span.maximum/scale},host.floorWorldY()/impl_->verticalPixelsPerCentimeter);
  impl_->binding.apply();
}
void SpatialCharacter::pickup(double logicalX,double elevation) {
  impl_->binding.cancelSignature(true);
  impl_->binding.motion().pickup(logicalX/impl_->pixelsPerCentimeter,
                                 elevation/impl_->verticalPixelsPerCentimeter);
}
void SpatialCharacter::hold(double logicalX,double elevation) {
  impl_->binding.motion().hold(logicalX/impl_->pixelsPerCentimeter,
                               elevation/impl_->verticalPixelsPerCentimeter);
}
void SpatialCharacter::release(QPointF velocity) {
  impl_->binding.motion().release({velocity.x()/impl_->pixelsPerCentimeter,
                                   velocity.y()/impl_->verticalPixelsPerCentimeter});
}
void SpatialCharacter::reconcile(const DesktopHost &host) {
  const auto span=host.groundSpan();
  impl_->binding.motion().reconcile(host.groundRootX()/impl_->pixelsPerCentimeter,
      {span.minimum/impl_->pixelsPerCentimeter,span.maximum/impl_->pixelsPerCentimeter},
      std::max(0.,host.floorWorldY()-host.groundWorldY())/impl_->verticalPixelsPerCentimeter,host.floorWorldY()/impl_->verticalPixelsPerCentimeter);
}
void SpatialCharacter::setSupportFrame(const DesktopHost& host) {
 const auto span=host.groundSpan();const auto scale=impl_->pixelsPerCentimeter;
 impl_->binding.motion().setSupportFrame({span.minimum/scale,span.maximum/scale},host.floorWorldY()/impl_->verticalPixelsPerCentimeter);
 setSupportBounds(host.supportBounds());
}
std::pair<double,double> SpatialCharacter::supportFootprint()const {
 const auto pose=impl_->binding.motion().pose();
 double left=0,right=0;
 for(std::size_t side=0;side<pose.contacts.size();++side) {
  // Resident basis maps the motion Z travel plane to scene X. The camera's
  // pitch changes vertical calibration but leaves this lateral axis exact.
  const double offset=(pose.contacts[side].z*100-pose.rootX)*impl_->pixelsPerCentimeter;
  if(side==0)left=right=offset;else {left=std::min(left,offset);right=std::max(right,offset);}
 }
 return {left-2,right+2};
}
bool SpatialCharacter::flightCapable()const {return impl_->binding.motion().flightCapable();}
bool SpatialCharacter::hop(double height){return impl_->binding.motion().hop(height/impl_->verticalPixelsPerCentimeter);}
bool SpatialCharacter::flyTo(double x,double elevation){return impl_->binding.motion().flyTo(x/impl_->pixelsPerCentimeter,elevation/impl_->verticalPixelsPerCentimeter);}
void SpatialCharacter::land(){impl_->binding.motion().land();}
void SpatialCharacter::cancelFlight(){impl_->binding.motion().cancelFlight();}
void SpatialCharacter::setSupportBounds(std::optional<character::GroundSpan> logicalBounds){
 if(logicalBounds){
  if(!std::isfinite(logicalBounds->minimum)||!std::isfinite(logicalBounds->maximum)||logicalBounds->minimum>logicalBounds->maximum)throw std::runtime_error("invalid spatial support bounds");
  // Footprint includes a two-pixel sole radius. Keep contact centers inside
  // that same inset, otherwise a valid touchdown immediately loses support.
  const double center=(logicalBounds->minimum+logicalBounds->maximum)/2;
  logicalBounds->minimum=std::min(center,logicalBounds->minimum+2);
  logicalBounds->maximum=std::max(center,logicalBounds->maximum-2);
  logicalBounds->minimum/=impl_->pixelsPerCentimeter;
  logicalBounds->maximum/=impl_->pixelsPerCentimeter;
 }
 impl_->binding.motion().setSupportBounds(logicalBounds);
}
void SpatialCharacter::target(double logicalX) { impl_->binding.cancelSignature(); impl_->binding.motion().target(logicalX/impl_->pixelsPerCentimeter); }
bool SpatialCharacter::face(double logicalX) { impl_->binding.cancelSignature(); return impl_->binding.motion().face(logicalX/impl_->pixelsPerCentimeter); }
bool SpatialCharacter::blinkSupported() const { return impl_->binding.eyeBlinkSupported(); }
void SpatialCharacter::setEyeOpenness(double openness) { impl_->binding.setEyeOpenness(openness); }
character::MotionProposal SpatialCharacter::propose(double dt) {
  auto p = impl_->binding.motion().propose(dt);
  // Realization below owns the frame's single presentation update.
  impl_->binding.advancePresentation(dt,false);
  p.displacement *= impl_->pixelsPerCentimeter; p.targetX *= impl_->pixelsPerCentimeter;
  p.displacementY *= impl_->verticalPixelsPerCentimeter;
  return p;
}
void SpatialCharacter::realize(double logicalX, bool blocked,std::optional<double> elevation) {
  if(elevation)*elevation/=impl_->verticalPixelsPerCentimeter;
  impl_->binding.motion().realize(logicalX/impl_->pixelsPerCentimeter, blocked,elevation,
      1./impl_->verticalPixelsPerCentimeter+1e-6);
}
character::GroundedState SpatialCharacter::state() const {
  auto s=impl_->binding.motion().state();
  s.rootX *= impl_->pixelsPerCentimeter; s.targetX *= impl_->pixelsPerCentimeter;
  s.velocity *= impl_->pixelsPerCentimeter;
  s.elevation *= impl_->verticalPixelsPerCentimeter;
  s.verticalVelocity *= impl_->verticalPixelsPerCentimeter;
  s.verticalAcceleration *= impl_->verticalPixelsPerCentimeter;
  for(auto &foot:s.feet){
    foot.position.x*=impl_->pixelsPerCentimeter;foot.landing.x*=impl_->pixelsPerCentimeter;
    foot.position.y*=impl_->verticalPixelsPerCentimeter;foot.landing.y*=impl_->verticalPixelsPerCentimeter;
  }
  return s;
}
character::ExpressionMotion &SpatialCharacter::expressions() { return impl_->binding.motion().expressions(); }
bool SpatialCharacter::reach(int side, motion::Vec3 target) { return impl_->binding.motion().reach(side, target); }
void SpatialCharacter::releaseReach(int side) { impl_->binding.motion().clearArmGoal(side); }
bool SpatialCharacter::settled() const { return impl_->binding.settled(); }
bool SpatialCharacter::signatureSupported() const { return impl_->binding.signatureSupported(); }
QString SpatialCharacter::signatureName() const { return impl_->binding.signatureName(); }
bool SpatialCharacter::signature() { return impl_->binding.signature(); }
void SpatialCharacter::cancelSignature(bool immediate) { impl_->binding.cancelSignature(immediate); }
bool SpatialCharacter::signatureActive() const { return impl_->binding.signatureActive(); }
void SpatialCharacter::setSpeed(double multiplier) { impl_->binding.setWalkingSpeed(multiplier); }
void SpatialCharacter::reaction(const QString &semantic, double progress) { impl_->binding.setReaction(semantic,progress); }
void SpatialCharacter::apply(DesktopHost &host) {
  impl_->binding.apply();
  host.updatePresentationMask(impl_->binding.projectedVisualRegion(impl_->item), impl_->binding.projectedGrabRegion(impl_->item));
  if (impl_->item) QMetaObject::invokeMethod(impl_->item, "refresh", Qt::DirectConnection);
}
void SpatialCharacter::setRenderingEnabled(bool enabled) {
  if (impl_->item && impl_->item->property("renderingEnabled").toBool() != enabled)
    impl_->item->setProperty("renderingEnabled", enabled);
}
QPoint SpatialCharacter::cardAnchor() const {
  if (!impl_->item) return {};
  return impl_->binding.projectAnchor(impl_->item,impl_->binding.cardAnchor()).toPoint();
}
QPoint SpatialCharacter::headAnchor() const {
  if (!impl_->item) return {};
  return impl_->binding.projectAnchor(impl_->item,impl_->binding.headAnchor()).toPoint();
}
motion::Vec3 SpatialCharacter::cursorLookDirection(motion::Vec3 direction) const {
  if (!impl_->camera || !impl_->avatar || !std::isfinite(direction.x)
      || !std::isfinite(direction.y) || !std::isfinite(direction.z)) return {0,0,1};
  const auto camera = impl_->camera->property("sceneRotation").value<QQuaternion>();
  const auto avatar = impl_->avatar->property("sceneRotation").value<QQuaternion>();
  // Relative gaze keeps a centered cursor neutral. Absolute camera-facing
  // gaze would consume the entire head range at the body's ±55° presentation
  // angle, clipping nearby cursor motion before it could express interest.
  const auto local = avatar.conjugated().rotatedVector(camera.rotatedVector(
      QVector3D(float(direction.x),float(direction.y),0)));
  return {local.x(),local.y(),local.z()+direction.z};
}
#else
struct SpatialCharacter::Impl {};
SpatialCharacter::SpatialCharacter(QString) { throw std::runtime_error("3D rendering is unavailable in this build; enable Qt Quick 3D"); }
SpatialCharacter::~SpatialCharacter() = default;
void SpatialCharacter::install(DesktopHost &) {}
void SpatialCharacter::reset(const DesktopHost &) {}
void SpatialCharacter::pickup(double,double) {}
void SpatialCharacter::hold(double,double) {}
void SpatialCharacter::release(QPointF) {}
void SpatialCharacter::reconcile(const DesktopHost &) {}
void SpatialCharacter::setSupportFrame(const DesktopHost &) {}
std::pair<double,double> SpatialCharacter::supportFootprint() const { return {0,0}; }
bool SpatialCharacter::flightCapable() const { return false; }
bool SpatialCharacter::hop(double) { return false; }
bool SpatialCharacter::flyTo(double,double) { return false; }
void SpatialCharacter::land() {}
void SpatialCharacter::cancelFlight() {}
void SpatialCharacter::setSupportBounds(std::optional<character::GroundSpan>) {}
void SpatialCharacter::target(double) {}
bool SpatialCharacter::face(double) { return false; }
bool SpatialCharacter::blinkSupported() const { return false; }
void SpatialCharacter::setEyeOpenness(double) {}
character::MotionProposal SpatialCharacter::propose(double) { return {}; }
void SpatialCharacter::realize(double,bool,std::optional<double>) {}
character::GroundedState SpatialCharacter::state() const { return {}; }
bool SpatialCharacter::settled() const { return true; }
bool SpatialCharacter::signatureSupported() const { return false; }
QString SpatialCharacter::signatureName() const { return {}; }
bool SpatialCharacter::signature() { return false; }
void SpatialCharacter::cancelSignature(bool) {}
bool SpatialCharacter::signatureActive() const { return false; }
void SpatialCharacter::setSpeed(double) {}
character::ExpressionMotion &SpatialCharacter::expressions() { static character::ExpressionMotion unavailable; return unavailable; }
bool SpatialCharacter::reach(int, motion::Vec3) { return false; }
void SpatialCharacter::releaseReach(int) {}
void SpatialCharacter::reaction(const QString &,double) {}
void SpatialCharacter::apply(DesktopHost &) {}
void SpatialCharacter::setRenderingEnabled(bool) {}
QPoint SpatialCharacter::cardAnchor() const { return {}; }
QPoint SpatialCharacter::headAnchor() const { return {}; }
motion::Vec3 SpatialCharacter::cursorLookDirection(motion::Vec3 direction) const { return direction; }
#endif
}
