#pragma once
#include "behavior/curiosity.h"
#include "behavior/blink.h"
#include "characters/character.h"
#include "characters/grounded.h"
#include "ipc.h"
#include "motion.h"
#include "runtime.h"
#include "sessions.h"
#include "settings.h"
#include "window_surfaces.h"
#include <QObject>
#include <QPointF>
#include <QTimer>
#include <functional>
#include <deque>
#include <memory>
#include <stdexcept>

class QAction;
class QMenu;
namespace pets {
class MenuBarIcon;
namespace productivity { class Service; class Panel; }
class DesktopHost;
class SpatialCharacter;
class Instance;
class SleepObserver;
class StatusLabel;
class UtilityToolbox;
enum class UtilityAction;
enum class IpcMode { Enabled, DisabledForTests };
class AlreadyRunning final : public std::runtime_error {
public:
  AlreadyRunning() : std::runtime_error("Clibby is already running") {}
};
class ResidentApplication final : public QObject {
public:
  using Clock = std::function<double()>;
  using WallClock = std::function<qint64()>;
  // Global logical pixel observation; an absent sample means no visible target.
  using CursorSource = std::function<std::optional<QPointF>()>;
  using SurfaceSource = std::function<WindowSurfaceSnapshot(double)>;
  ResidentApplication(const QString &assetsDirectory,
                      const QString &stateDirectory, bool showTray = true,
                      Clock clock = {}, IpcMode ipcMode = IpcMode::Enabled,
                      QObject *parent = nullptr,
                      bool restoreCharacterSelection = true, CursorSource cursorSource = {}, SurfaceSource surfaceSource = {}, WallClock wallClock = {});
  ~ResidentApplication() override;
  void control(Control command);
  const Runtime &runtime() const { return runtime_; }
  bool timerActive() const { return timer_.isActive(); }
  int timerInterval() const { return timer_.interval(); }
  bool petVisible() const;
  RpcReply handleRequest(const QString &method, const QJsonObject &params);
  QString statusText() const;
  QString selectedCharacter() const { return preferences_.character; }
  bool selectCharacter(const QString &directory, QString *error = nullptr);
  character::Pose characterPose() const { return characterPose_; }
  bool springActive() const { return springDeadline_.has_value() || (hasGroundMotion() && groundDeadline_.has_value()); }
  std::optional<character::GroundedState> groundedState() const;
  void setGroundTarget(double worldX);
  bool setWalkingSpeed(double multiplier);
  double walkingSpeed() const { return preferences_.walkingSpeed; }
  bool setCompanionScale(double scale);
  bool setPixelation(double strength);
  bool groundMotionActive() const { return groundDeadline_.has_value(); }
  bool flightCapable() const;
  bool hop(double height = 35);
  bool flyTo(double worldX, double elevation);
  bool land();
  bool flyToWindow();
  bool signature();
  bool signatureActive() const;

private:
  void advance();
  void schedule();
  void persist();
  void updateMenu();
  void updateAppearanceMenu();
  void shutdown();
  void synchronizeSessions(double now);
  void advanceProductivity();
  void showProductivity();
  void signalProductivity(const QJsonObject &item);
  void updateSessionMenu(const SessionSnapshot &snapshot);
  void updateLabel();
  QString dispatchUtilityAction(SessionKey key, std::uint64_t sequence, UtilityAction action);
  QPoint cardAnchor() const;
  void applyReactionPose(double now);
  void applyBasePose();
  void applyCharacterPose(double now, bool allowBaseAnimation = true);
  void dragMotion(QPointF displacement);
  void endDrag(bool released);
  void reconcileGround();
  void configureSupportFootprint();
  bool dropBusy() const;
  void sampleDrag(double now);
  QPointF releaseVelocity(double now) const;
  void clearSpringMotion();
  void rebaseGround(bool preserveSecondary = false);
  void resetCuriosity();
  void advanceCuriosity(double now);
  void advanceBlink(double now);
  void wakeMotion(double now);
  void advanceSurfaces(double now);
  void advanceRaven(double now);
  void advanceSignature(double now);
  bool flyForward(double elevation);
  character::ExpressionMotion *expressions();
  void submitReaction(double now);
  bool hasGroundMotion() const;
  bool groundSettled() const;
  Clock clock_;
  WallClock wallClock_;
  CursorSource cursorSource_;
  SurfaceSource surfaceSource_;
  std::optional<QPointF> cursorPosition_;
  std::optional<QPointF> cursorHeadingPosition_;
  std::optional<motion::Vec3> cursorDirection_;
  std::optional<double> cursorDeadline_;
  double cursorTurnAfter_ = 0;
  Runtime runtime_;
  Sessions sessions_;
  behavior::Curiosity curiosity_{1};
  behavior::Blink blink_{5};
  bool blinkSupported_ = false, blinkDirty_ = false;
  bool manualNavigation_ = false;
  double manualUntil_ = 0;
  bool blockedNavigation_ = false;
  bool reactionSubmitted_ = false;
  std::optional<double> reactionDeadline_;
  QTimer timer_;
  QString settingsFile_;
  Preferences preferences_;
  Rig rig_;
  std::unique_ptr<Motion> motion_;
  std::unique_ptr<character::Controller> characterController_;
  character::Pose characterPose_;
  std::unique_ptr<character::GroundedController> groundedController_;
  std::unique_ptr<SpatialCharacter> spatialCharacter_;
  double lastGroundTime_ = 0;
  std::optional<double> groundDeadline_;
  std::optional<double> surfaceDeadline_, ravenDeadline_;
  std::optional<double> signatureDeadline_;
  std::optional<double> flightLandAt_;
  QString perchTargetId_;
  std::string supportFrameId_;
  QPointF perchApproach_;
  double perchApproachClearance_=50;
  bool perchDescending_ = false;
  bool cruising_ = false;
  int cruiseDirection_ = 1;
  double cruiseElevation_ = 100, cruiseTargetX_ = 0;
  bool realizingGround_ = false;
  struct DragSample { double time; QPointF position; };
  std::deque<DragSample> dragSamples_;
  QString characterRoot_;
  double characterTime_ = 0;
  double lastSpringTime_ = 0;
  std::optional<double> springDeadline_;
  std::unique_ptr<Instance> instance_;
  std::unique_ptr<DesktopHost> host_;
  std::unique_ptr<QMenu> menu_;
  std::unique_ptr<MenuBarIcon> tray_;
  std::unique_ptr<SleepObserver> sleep_;
  std::unique_ptr<IpcServer> ipc_;
  std::unique_ptr<StatusLabel> statusLabel_;
  std::unique_ptr<UtilityToolbox> toolbox_;
  std::unique_ptr<productivity::Service> productivity_;
  std::unique_ptr<productivity::Panel> productivityPanel_;
  QString productivityError_;
  QMenu *sessionsMenu_ = nullptr;
  QList<QAction *> sessionActions_;
  QStringList sessionMenuSignature_;
  std::optional<SessionReaction> reaction_;
  QAction *visibilityAction_ = nullptr;
  QAction *pauseAction_ = nullptr;
  QAction *settingsErrorAction_ = nullptr;
  QAction *characterErrorAction_ = nullptr;
  bool settingsWriteFailed_ = false;
  bool stopped_ = false;
};
} // namespace pets
