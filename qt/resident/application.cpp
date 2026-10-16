#include "application.h"
#include "spatial_character.h"
#include "characters/loader.h"
#include "characters/fixtures.h"
#include "desktop_host.h"
#include "instance.h"
#include "reaction_motion.h"
#include "sleep_observer.h"
#include "status_label.h"
#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QCursor>
#include <QDir>
#include <QUrl>
#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMenu>
#include <QPixmap>
#include <QScreen>
#include <QSystemTrayIcon>
#include <QToolTip>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
namespace pets {
namespace {
std::optional<character::GroundSpan> soleBounds(const DesktopHost &host) {
  auto bounds=host.supportBounds();
  if(bounds) {
    const double margin=std::min(2.,(bounds->maximum-bounds->minimum)/2);
    bounds->minimum+=margin; bounds->maximum-=margin;
  }
  return bounds;
}
QString characterDirectoryForId(const QString &root, const QString &id) {
  QString found;
  const auto directories =
      QDir(root).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
  if (directories.size() > 128)
    throw std::runtime_error("character catalog exceeds 128 directories");
  for (const auto &info : directories) {
    if (info.isSymLink())
      continue;
    const QFileInfo manifest(info.absoluteFilePath() + "/manifest.json");
    if (!manifest.isFile() || manifest.isSymLink() ||
        manifest.size() > 1024 * 1024)
      continue;
    QFile file(manifest.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly))
      continue;
    const auto bytes = file.read(1024 * 1024 + 1);
    if (bytes.size() > 1024 * 1024)
      continue;
    const auto value = QJsonDocument::fromJson(bytes).object().value("id");
    if (!value.isString() || value.toString() != id)
      continue;
    if (!found.isEmpty())
      throw std::runtime_error("duplicate character identity in catalog");
    found = info.absoluteFilePath();
  }
  if (found.isEmpty())
    throw std::runtime_error("saved character identity is unavailable");
  return found;
}
} // namespace
ResidentApplication::ResidentApplication(const QString &assets,
                                         const QString &stateDirectory,
                                         bool showTray, Clock clock,
                                         IpcMode ipcMode, QObject *parent,
                                         bool restoreCharacterSelection, CursorSource cursorSource, SurfaceSource surfaceSource)
    : QObject(parent), clock_(clock ? std::move(clock) : Clock([] {
        return std::chrono::duration<double>(
                   std::chrono::steady_clock::now().time_since_epoch())
            .count();
      })),
      cursorSource_(cursorSource ? std::move(cursorSource) : CursorSource([] { return std::optional<QPointF>{QCursor::pos()}; })),
      surfaceSource_(std::move(surfaceSource)),
      runtime_(clock_()), sessions_(clock_()), timer_(this),
      settingsFile_(stateDirectory + "/settings.json") {
  instance_ = std::make_unique<Instance>(stateDirectory);
  if (instance_->result() == Instance::Result::AlreadyRunning)
    throw AlreadyRunning();
  if (instance_->result() != Instance::Result::Acquired)
    throw std::runtime_error(instance_->error().toStdString());
  if (showTray && !QSystemTrayIcon::isSystemTrayAvailable())
    throw std::runtime_error(
        "menu-bar/system-tray controls are unavailable on this desktop");
  preferences_ = loadPreferences(settingsFile_);
  const auto restoredPlacement = preferences_;
  if (preferences_.paused)
    runtime_.control(Control::Pause, clock_());
  const bool suppliedSpatial = isSpatialPackage(assets);
  QString spatialToRestore = suppliedSpatial ? assets : QString();
  const QString initialAssets = suppliedSpatial ? QFileInfo(assets).absolutePath() + "/soft-creature" : assets;
  if (QFileInfo::exists(initialAssets + "/manifest.json")) {
    auto loaded = character::loadCharacter(initialAssets);
    characterRoot_ =
        QFileInfo(QFileInfo(initialAssets).absolutePath()).canonicalFilePath();
    if (restoreCharacterSelection &&
        preferences_.character !=
            QString::fromStdString(loaded.definition.id) &&
        preferences_.character != "fixture-animal") {
      try {
        const auto remembered = characterDirectoryForId(characterRoot_, preferences_.character);
        if (!suppliedSpatial && isSpatialPackage(remembered)) spatialToRestore = remembered;
        else if (!suppliedSpatial) loaded = character::loadCharacter(remembered);
      } catch (const std::exception &e) {
        qWarning() << "Saved character unavailable; using supplied character:"
                   << e.what();
      }
    }
    preferences_.character = QString::fromStdString(loaded.definition.id);
    characterController_ =
        std::make_unique<character::Controller>(loaded.definition);
    characterPose_ = characterController_->pose(character::Action::Rest, 0);
    host_ = std::make_unique<DesktopHost>(loaded);
    if (loaded.definition.locomotion)
      groundedController_ = std::make_unique<character::GroundedController>(loaded.definition);
    if (groundedController_) groundedController_->setSpeedMultiplier(preferences_.walkingSpeed);
    characterPose_.eyeOpenness=blinkSupported_ ? blink_.openness() : 1;
    host_->applyPose(characterPose_);
  } else {
    rig_ = loadRig(assets);
    QImage atlas(rig_.atlas);
    if (atlas.isNull())
      throw std::runtime_error("cannot decode fixture atlas: " +
                               rig_.atlas.toStdString());
    motion_ = std::make_unique<Motion>(rig_);
    preferences_.character = "fixture-animal";
    host_ = std::make_unique<DesktopHost>(rig_, atlas);
  }
  statusLabel_ = std::make_unique<StatusLabel>();
  toolbox_ = std::make_unique<UtilityToolbox>();
  const auto dispatch = [this](SessionKey key, std::uint64_t sequence, UtilityAction action) { return dispatchUtilityAction(key, sequence, action); };
  statusLabel_->dispatch = dispatch;
  toolbox_->dispatch = dispatch;
  toolbox_->setWalkingSpeed(preferences_.walkingSpeed, hasGroundMotion());
  toolbox_->walkingSpeedChanged = [this](double value) { setWalkingSpeed(value); };
  if (preferences_.hasPosition)
    host_->restorePlacement(preferences_.position, preferences_.display);
  rebaseGround();
  host_->groundChanged = [this] { if (!realizingGround_) { reconcileGround(); schedule(); } };
  host_->draggingChanged = [this](bool dragging) {
    if (dragging) control(Control::BeginDrag);
  };
  host_->dragEnded = [this](bool released) { endDrag(released); };
  host_->dragDisplacement = [this](QPointF displacement) {
    dragMotion(displacement);
  };
  host_->placementChanged = [this] {
    preferences_.position = host_->position();
    preferences_.display = host_->displayName();
    preferences_.hasPosition = true;
    if (statusLabel_ && !realizingGround_)
      statusLabel_->placeBeside(host_->position(), cardAnchor());
    if (!runtime_.state().dragging && !realizingGround_)
      persist();
  };
  host_->renderingError = [this](const QString &message) {
    qCritical().noquote() << "error: pet renderer failed:" << message;
    if (spatialCharacter_) {
      control(Control::Hide);
      if (characterErrorAction_) {
        characterErrorAction_->setText("3D renderer failed; choose a 2D character: " + message.left(100));
        characterErrorAction_->setVisible(true);
      }
      return; // Keep tray, toolbox, preferences and sessions available.
    }
    control(Control::Quit);
    QCoreApplication::exit(1);
  };
  menu_ = std::make_unique<QMenu>();
  auto addAction = [this](const QString &text) {
    auto *action = new QAction(text, this);
    menu_->addAction(action);
    return action;
  };
  visibilityAction_ = addAction("Hide");
  pauseAction_ = addAction("Pause");
  connect(visibilityAction_, &QAction::triggered, this, [this] {
    control(runtime_.state().visible ? Control::Hide : Control::Show);
  });
  connect(pauseAction_, &QAction::triggered, this, [this] {
    control(runtime_.state().paused ? Control::Resume : Control::Pause);
  });
  menu_->addSeparator();
  auto *left = addAction("Walk left");
  auto *right = addAction("Walk right");
  auto *stop = addAction("Stop walking");
  left->setObjectName("walkLeft"); right->setObjectName("walkRight"); stop->setObjectName("stopWalking");
  connect(left, &QAction::triggered, this, [this] { if (hasGroundMotion()) setGroundTarget(host_->groundSpan().minimum); });
  connect(right, &QAction::triggered, this, [this] { if (hasGroundMotion()) setGroundTarget(host_->groundSpan().maximum); });
  connect(stop, &QAction::triggered, this, [this] { if (hasGroundMotion()) setGroundTarget(host_->groundRootX()); });
  toolbox_->addControl(pauseAction_, "toolboxPause");
  toolbox_->addControl(left, "toolboxWalkLeft");
  toolbox_->addControl(right, "toolboxWalkRight");
  toolbox_->addControl(stop, "toolboxStop");
  auto *hopAction=addAction("Hop");
  auto *flyAction=addAction("Fly up");
  auto *landAction=addAction("Land");
  auto *perchAction=addAction("Fly to a window");
  hopAction->setObjectName("hop"); flyAction->setObjectName("fly");
  landAction->setObjectName("land"); perchAction->setObjectName("perch");
  connect(hopAction,&QAction::triggered,this,[this]{hop();});
  connect(flyAction,&QAction::triggered,this,[this]{flyForward(host_->elevation()+100);});
  connect(landAction,&QAction::triggered,this,[this]{land();});
  connect(perchAction,&QAction::triggered,this,[this]{
    if(!flyToWindow())QToolTip::showText(QCursor::pos(),
        "No window top with room to perch. Move a window lower, then try again.");
  });
  toolbox_->addControl(hopAction,"toolboxHop"); toolbox_->addControl(flyAction,"toolboxFly");
  toolbox_->addControl(landAction,"toolboxLand"); toolbox_->addControl(perchAction,"toolboxPerch");
  left->setEnabled(hasGroundMotion()); right->setEnabled(hasGroundMotion()); stop->setEnabled(hasGroundMotion());
  if (characterController_) {
    auto *characters = menu_->addMenu("Character");
    const QDir root(characterRoot_);
    for (const auto &info :
         root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
      if (info.isSymLink() ||
          !QFileInfo::exists(info.absoluteFilePath() + "/manifest.json"))
        continue;
      auto *action = characters->addAction(info.fileName());
      action->setObjectName("selectCharacter");
      connect(action, &QAction::triggered, this,
              [this, path = info.absoluteFilePath()] {
                QString error;
                if (!selectCharacter(path, &error)) {
                  characterErrorAction_->setText(
                      "Character could not be loaded: " + error.left(120));
                  characterErrorAction_->setVisible(true);
                }
              });
    }
    characterErrorAction_ = addAction("Character could not be loaded");
    characterErrorAction_->setEnabled(false);
    characterErrorAction_->setVisible(false);
    menu_->addSeparator();
  }
  auto* summon = addAction("Toolbox…");
  summon->setObjectName("summonToolbox");
  connect(summon, &QAction::triggered, this, [this] {
    if (stopped_ || runtime_.state().suspended || !runtime_.state().visible) return;
    toolbox_->setRecords(sessions_.snapshot(clock_()).active);
    toolbox_->show(); toolbox_->raise(); toolbox_->activateWindow();
  });
  sessionsMenu_ = menu_->addMenu("Sessions");
  settingsErrorAction_ = addAction("Settings could not be saved");
  settingsErrorAction_->setEnabled(false);
  settingsErrorAction_->setVisible(false);
  menu_->addSeparator();
  connect(addAction("Quit"), &QAction::triggered, this,
          [this] { control(Control::Quit); });
  host_->contextMenuRequested = [this](QPoint global) {
    if (!stopped_)
      menu_->popup(global);
  };
  if (showTray) {
    // A separate, small application mark avoids reading/uploading artwork on
    // ticks.
    QPixmap mark(32, 32);
    mark.fill(QColor(105, 74, 48));
    tray_ = std::make_unique<QSystemTrayIcon>(QIcon(mark));
    tray_->setToolTip("Better Pets");
    tray_->setContextMenu(menu_.get());
    tray_->show();
  }
  timer_.setTimerType(Qt::PreciseTimer);
  timer_.setSingleShot(true);
  connect(&timer_, &QTimer::timeout, this, [this] { advance(); });
  sleep_ = observeSleep(this, [this](Control command) { control(command); });
  if (ipcMode == IpcMode::Enabled)
    ipc_ = std::make_unique<IpcServer>(
        stateDirectory,
        [this](const QString &method, const QJsonObject &params) {
          return handleRequest(method, params);
        });
  synchronizeSessions(clock_());
  updateMenu();
  host_->setPetVisible(true);
  if (groundedController_) applyBasePose();
  if (!spatialToRestore.isEmpty()) {
    QString error;
    if (!selectCharacter(spatialToRestore, &error)) {
      if (suppliedSpatial) throw std::runtime_error(error.toStdString());
      qWarning() << "Saved 3D character unavailable; retaining 2D pet:" << error;
    } else if (restoredPlacement.hasPosition) {
      host_->restorePlacement(restoredPlacement.position, restoredPlacement.display);
      rebaseGround(); spatialCharacter_->apply(*host_); updateLabel();
    }
  }
  resetCuriosity();
  advanceSurfaces(clock_());
  persist();
  schedule();
}
ResidentApplication::~ResidentApplication() { shutdown(); }
bool ResidentApplication::petVisible() const {
  return host_ && host_->isPetVisible();
}
QString ResidentApplication::statusText() const {
  return statusLabel_ ? statusLabel_->text() : QString();
}
void ResidentApplication::control(Control command) {
  if (stopped_)
    return;
  if (command == Control::Hide || command == Control::Suspend ||
      command == Control::Quit)
    host_->cancelDrag();
  if (command == Control::Wake || command == Control::Show)
    host_->reconcileScreens();
  const double now = clock_();
  runtime_.control(command, now);
  if(command==Control::Pause || command==Control::Hide || command==Control::Suspend || command==Control::Quit) {
    surfaceDeadline_.reset(); ravenDeadline_.reset(); flightLandAt_.reset();
  }
  if(command==Control::Resume || command==Control::Show || command==Control::Wake) {
    advanceSurfaces(now);
    if(!perchTargetId_.isEmpty()) flightLandAt_=now+12;
    if(flightCapable()) ravenDeadline_=now+8;
  }
  advanceCuriosity(now);
  if (command == Control::Hide || command == Control::Suspend || command == Control::Quit) toolbox_->hide();
  if (command == Control::Pause) { groundDeadline_.reset(); springDeadline_.reset(); lastGroundTime_ = now; lastSpringTime_ = now; }
  if (command == Control::Resume && hasGroundMotion() && !groundSettled()) {
    lastGroundTime_ = now; groundDeadline_ = now + 1.0/60;
  }
  if (command == Control::BeginDrag && hasGroundMotion()) {
    perchTargetId_.clear(); flightLandAt_.reset(); perchDescending_=false; cruising_=false;
    host_->clearSupport();
    reconcileGround();
    dragSamples_.clear(); sampleDrag(now);
    manualNavigation_ = false;
    if (spatialCharacter_) spatialCharacter_->pickup(host_->groundRootX(),host_->elevation());
    else groundedController_->pickup(host_->groundRootX(),host_->elevation());
    runtime_.cancelReaction(now); reaction_.reset(); reactionSubmitted_=false; reactionDeadline_.reset();
    if (!runtime_.state().paused) { applyBasePose(); wakeMotion(now); }
  }
  if (command == Control::EndDrag && hasGroundMotion()) {
    const auto v=runtime_.state().paused ? QPointF{} : releaseVelocity(now);
    if (spatialCharacter_) spatialCharacter_->release(v);
    else groundedController_->release({v.x(),v.y()});
    dragSamples_.clear(); wakeMotion(now);
  }
  if (command == Control::Hide || command == Control::Suspend || command == Control::Wake ||
      command == Control::Show || command == Control::Quit) {
    if (dropBusy()) {
      groundDeadline_.reset(); lastGroundTime_=now;
      if (command == Control::Wake || command == Control::Show) wakeMotion(now);
    } else rebaseGround();
  }
  if (command == Control::Hide || command == Control::Suspend || command == Control::Wake ||
      command == Control::Show || command == Control::Quit)
    clearSpringMotion();
  if (command == Control::Resume) wakeMotion(now);
  if (command == Control::Quit) {
    shutdown();
    QCoreApplication::quit();
    return;
  }
  if (command == Control::Show || command == Control::Hide)
    host_->setPetVisible(runtime_.state().visible);
  if (command == Control::Suspend || command == Control::Wake)
    sessions_.reset(now);
  synchronizeSessions(now);
  if ((command == Control::Resume || command == Control::EndDrag ||
       command == Control::Wake || command == Control::Show) &&
      !runtime_.state().paused && !runtime_.state().suspended &&
      !runtime_.state().dragging)
    applyBasePose();
  if (command == Control::Pause || command == Control::Resume ||
      command == Control::EndDrag)
    persist();
  updateMenu();
  schedule();
}
void ResidentApplication::advance() {
  if (stopped_)
    return;
  const double now = clock_();
  if (const auto deadline = sessions_.nextDeadline();
      deadline && *deadline <= now)
    synchronizeSessions(now);
  configureSupportFootprint();
  advanceSurfaces(now);
  advanceRaven(now);
  advanceCuriosity(now);
  submitReaction(now);
  const auto tick = runtime_.advance(now);
  if (motion_)
    for (unsigned step = 0; step < tick.steps; ++step)
      motion_->step(MotionPhase::Idle);
  if (characterController_ && tick.steps)
    characterTime_ += double(tick.steps) / 60.0;
  bool groundChanged = false;
  if (hasGroundMotion() && groundDeadline_ && *groundDeadline_ <= now) {
    const auto &state = runtime_.state();
    if (!state.visible || state.paused || state.suspended) {
      groundDeadline_.reset(); lastGroundTime_=now;
    }
    else {
      const bool wasDropping=dropBusy();
      const auto dt = std::clamp(now-lastGroundTime_, 0.0, 1.0/30);
      const auto proposal = spatialCharacter_ ? spatialCharacter_->propose(dt) : groundedController_->propose(dt);
      lastGroundTime_ = now;
      realizingGround_ = true;
      const auto predicted=host_->predictedWorldPosition(proposal.displacement,proposal.displacementY);
      if(!state.dragging && proposal.displacementY>0 && dropBusy() &&
          host_->acquireDescendingSupport(host_->groundWorldY(),predicted.y(),now,predicted.x())) {
        if(spatialCharacter_) spatialCharacter_->setSupportFrame(*host_);
        else {
          groundedController_->setSupportFrame(host_->groundSpan(),host_->floorWorldY());
          groundedController_->setSupportBounds(soleBounds(*host_));
        }
        supportFrameId_=host_->selectedSurfaceId();
      }
      const auto actual = state.dragging ? DesktopHost::WorldRealization{host_->groundRootX(),host_->elevation(),false}
          : host_->moveInWorld(proposal.displacement,proposal.displacementY);
      realizingGround_ = false;
      if (spatialCharacter_) spatialCharacter_->realize(actual.rootX, actual.blocked, actual.elevation);
      else groundedController_->realize(actual.rootX, actual.blocked, actual.elevation);
      blockedNavigation_ = actual.blocked;
      if (wasDropping && !dropBusy()) {
        resetCuriosity();
        synchronizeSessions(now);
      }
      groundChanged = true;
      groundDeadline_ = groundSettled() ? std::optional<double>{} : std::optional<double>{now+1.0/60};
      if (!groundDeadline_ && !state.dragging) persist();
    }
  }
  bool springChanged = false;
  if (springDeadline_ && *springDeadline_ <= now) {
    const auto &s = runtime_.state();
    if (!s.visible || s.paused || s.suspended)
      clearSpringMotion();
    else {
      characterController_->step(
          std::clamp(now - lastSpringTime_, 0.0, 4.0 / 60.0));
      lastSpringTime_ = now;
      springChanged = true;
      if (characterController_->settled())
        springDeadline_.reset();
      else
        springDeadline_ = now + 1.0 / 60.0;
    }
  }
  if (spatialCharacter_ && (groundChanged || blinkDirty_ || (tick.steps && reaction_))) applyReactionPose(now);
  else if (characterController_ && (tick.steps || springChanged || groundChanged || blinkDirty_))
    applyCharacterPose(now, tick.steps > 0);
  else if (!spatialCharacter_ && tick.steps)
    applyReactionPose(now);
  blinkDirty_=false;
  schedule();
}
void ResidentApplication::schedule() {
  timer_.stop();
  if (stopped_)
    return;
  if (spatialCharacter_) {
    const auto &state = runtime_.state();
    spatialCharacter_->setRenderingEnabled(state.visible && !state.paused && !state.suspended
        && groundDeadline_.has_value());
  }
  auto deadline = runtime_.nextDeadline();
  if (const auto activity = curiosity_.nextDeadline();
      (characterController_ || spatialCharacter_) && activity && (!deadline || *activity < *deadline)) deadline = activity;
  if (blinkSupported_) {
    const auto blinkDeadline=blink_.nextDeadline();
    if (blinkDeadline && (!deadline || *blinkDeadline < *deadline)) deadline=blinkDeadline;
  }
  if (cursorDeadline_ && (!deadline || *cursorDeadline_ < *deadline)) deadline = cursorDeadline_;
  if (reactionDeadline_ && (!deadline || *reactionDeadline_ < *deadline)) deadline = reactionDeadline_;
  if (groundDeadline_ && (!deadline || *groundDeadline_ < *deadline)) deadline = groundDeadline_;
  if (surfaceDeadline_ && (!deadline || *surfaceDeadline_ < *deadline)) deadline = surfaceDeadline_;
  if (ravenDeadline_ && (!deadline || *ravenDeadline_ < *deadline)) deadline = ravenDeadline_;
  if (flightLandAt_ && (!deadline || *flightLandAt_ < *deadline)) deadline = flightLandAt_;
  if (springDeadline_ && (!deadline || *springDeadline_ < *deadline))
    deadline = springDeadline_;
  if (const auto expiry = sessions_.nextDeadline();
      expiry && (!deadline || *expiry < *deadline))
    deadline = expiry;
  if (!deadline)
    return;
  // Round upward: an early millisecond timer must not spin before a fractional
  // frame deadline. The runtime independently bounds and discards late steps.
  const double milliseconds =
      std::ceil(std::max(0.0, *deadline - clock_()) * 1000.0);
  timer_.start(static_cast<int>(
      std::clamp(milliseconds, 1.0, double(std::numeric_limits<int>::max()))));
}
namespace {
QJsonObject sessionJson(const SessionRecord &record, double now) {
  QJsonObject result{{"source_id", QString::fromStdString(record.key.sourceId)},
          {"session_id", QString::fromStdString(record.key.sessionId)},
          {"label", QString::fromStdString(record.label)},
          {"seq", double(record.sequence)},
          {"status", sessionStatusName(record.status)},
          {"ttl_remaining", std::max(0.0, record.expiresAt - now)},
          {"acknowledged", record.acknowledged}};
  if (!record.sourceUrl.empty()) result.insert("source_url", QString::fromStdString(record.sourceUrl));
  return result;
}
bool integer(const QJsonValue &value, double minimum, double maximum) {
  const double number = value.toDouble(-1);
  return value.isDouble() && std::isfinite(number) &&
         std::floor(number) == number && number >= minimum && number <= maximum;
}
QString sessionText(const SessionRecord &record) {
  return QString::fromStdString(record.key.sourceId) + " · " +
         QString::fromStdString(record.label) + " — " +
         sessionStatusName(record.status);
}
} // namespace
RpcReply ResidentApplication::handleRequest(const QString &method,
                                            const QJsonObject &params) {
  if (stopped_)
    return {{}, -32603, "Application is shutting down"};
  const double now = clock_();
  // Keep expiry authoritative even for malformed or stale incoming updates.
  synchronizeSessions(now);
  schedule();
  if (method == "state.get") {
    if (!params.isEmpty())
      return {{}, -32602, "state.get takes no parameters"};
    const auto snapshot = sessions_.snapshot(now);
    QJsonArray active;
    for (const auto &record : snapshot.active)
      active.append(sessionJson(record, now));
    QJsonValue foreground =
        snapshot.foreground ? QJsonValue(sessionJson(*snapshot.foreground, now))
                            : QJsonValue(QJsonValue::Null);
    return {{{"visible", runtime_.state().visible},
             {"paused", runtime_.state().paused},
             {"character", preferences_.character},
             {"foreground", foreground},
             {"sessions", active}}};
  }
  if (method != "session.update" && method != "session.clear")
    return {{}, -32601, "Unknown method"};
  if (!params["source_id"].isString() || !params["session_id"].isString() ||
      !integer(params["seq"], 0, double(Sessions::MaximumSequence)))
    return {
        {},
        -32602,
        "Expected source_id, session_id and a nonnegative safe integer seq"};
  SessionKey key{params["source_id"].toString().toStdString(),
                 params["session_id"].toString().toStdString()};
  const auto sequence = static_cast<std::uint64_t>(params["seq"].toDouble());
  SessionResult result;
  bool cleared = false;
  if (method == "session.clear") {
    const auto before = sessions_.snapshot(now);
    cleared = std::any_of(before.active.begin(), before.active.end(),
                          [&](const SessionRecord &r) { return r.key == key; });
    result = sessions_.clear(key, sequence, now);
  } else {
    if (!params["label"].isString() || !params["status"].isString() ||
        (params.contains("ttl") && !integer(params["ttl"], 1, 3600)))
      return {{},
              -32602,
              "Expected plain label, supported status and optional integer ttl "
              "in 1..3600"};
    const auto validSourceUrl = [](const QString &text) {
      if (text.isEmpty()) return true;
      if (text.toUtf8().size() > 2048) return false;
      const auto decoded = QUrl::fromPercentEncoding(text.toUtf8());
      for (QChar ch : decoded)
        if (ch.category() == QChar::Other_Control) return false;
      const QUrl url(text, QUrl::StrictMode);
      if (!url.isValid() || url.isRelative() || url.authority(QUrl::FullyEncoded).contains('@')) return false;
      const auto scheme = url.scheme().toLower();
      if (scheme == "https" || scheme == "http") return !url.host().isEmpty();
      return scheme == "file" && url.authority().isEmpty() && url.isLocalFile()
          && QDir::isAbsolutePath(url.toLocalFile());
    };
    if (params.contains("source_url") && (!params["source_url"].isString()
        || !validSourceUrl(params["source_url"].toString())))
      return {{}, -32602, "Invalid source_url"};
    const auto statusName = params["status"].toString();
    std::optional<SessionStatus> status;
    for (auto candidate : {SessionStatus::Working, SessionStatus::NeedsInput,
                           SessionStatus::Completed, SessionStatus::Failed})
      if (statusName == sessionStatusName(candidate))
        status = candidate;
    if (!status)
      return {{}, -32602, "Unsupported session status"};
    result = sessions_.update(
        {key, params["label"].toString().toStdString(), sequence, *status,
         unsigned(params.contains("ttl") ? params["ttl"].toInt() : 120),
         params["source_url"].toString().toStdString()},
        now);
  }
  synchronizeSessions(now);
  schedule();
  switch (result.code) {
  case SessionResultCode::InvalidParameters:
    return {{}, -32602, "Invalid session parameters"};
  case SessionResultCode::StaleSequence:
    return {{}, -32003, "Sequence must increase for this session"};
  case SessionResultCode::Capacity:
    return {{}, -32004, "Session capacity reached"};
  case SessionResultCode::Accepted:
    break;
  }
  if (method == "session.clear")
    return {{{"seq", double(result.sequence)}, {"cleared", cleared}}};
  return {{{"seq", double(result.sequence)}, {"ttl", int(result.ttlSeconds)}}};
}
void ResidentApplication::synchronizeSessions(double now) {
  const auto &state = runtime_.state();
  sessions_.setSuppressed(!state.visible || state.paused || state.dragging ||
                              state.suspended || state.quitting || dropBusy(),
                          now);
  const auto snapshot = sessions_.snapshot(now);
  const bool sameReaction =
      reaction_ && snapshot.reaction &&
      reaction_->key == snapshot.reaction->key &&
      reaction_->status == snapshot.reaction->status &&
      reaction_->startedAt == snapshot.reaction->startedAt;
  if (reaction_ && !sameReaction) {
    runtime_.cancelReaction(now);
    if (!state.paused && !state.dragging && !state.suspended)
      applyBasePose();
  }
  if (snapshot.reaction && !sameReaction && !characterController_ && !spatialCharacter_)
    runtime_.requestReaction(now, snapshot.reaction->endsAt - now);
  reaction_ = snapshot.reaction;
  advanceCuriosity(now);
  submitReaction(now);
  const auto& displayed = statusLabel_->record();
  const bool displayedExpired = displayed && std::none_of(snapshot.active.begin(), snapshot.active.end(),
      [&](const auto& record) { return record.key == displayed->key; });
  if (!state.paused && state.visible && !state.suspended)
    statusLabel_->setRecord(snapshot.foreground);
  else if (!snapshot.foreground || displayedExpired)
    statusLabel_->setRecord({});
  toolbox_->setRecords(snapshot.active);
  updateLabel();
  updateSessionMenu(snapshot);
}
QPoint ResidentApplication::cardAnchor() const {
  if (spatialCharacter_) return spatialCharacter_->cardAnchor();
  if (!characterController_) return QPoint(128, 80);
  const auto anchors = character::utilityAnchors(characterController_->definition(), characterPose_);
  const int margin = int(std::ceil(characterController_->definition().motionEnvelope));
  return QPoint(qRound(anchors.card.x) + margin, qRound(anchors.card.y) + margin);
}
QString ResidentApplication::dispatchUtilityAction(SessionKey key, std::uint64_t sequence, UtilityAction action) {
  if (stopped_ || runtime_.state().suspended || !runtime_.state().visible) return {};
  const double now = clock_();
  const auto snapshot = sessions_.snapshot(now);
  const auto found = std::find_if(snapshot.active.begin(), snapshot.active.end(), [&](const auto& record) { return record.key == key; });
  if (found == snapshot.active.end() || found->sequence != sequence) { synchronizeSessions(now); schedule(); return "This notification has changed. Use the current card in the toolbox."; }
  QString feedback;
  switch (action) {
  case UtilityAction::OpenSource:
    if (!found->sourceUrl.empty() && !QDesktopServices::openUrl(QUrl(QString::fromStdString(found->sourceUrl), QUrl::StrictMode)))
      feedback = "Source could not be opened. Try again or open it directly.";
    break;
  case UtilityAction::Acknowledge:
    if (found->status != SessionStatus::Working) sessions_.acknowledge(key, now);
    break;
  }
  synchronizeSessions(now); schedule();
  return feedback;
}
void ResidentApplication::updateLabel() {
  const auto &state = runtime_.state();
  if (!state.visible || state.suspended || statusLabel_->text().isEmpty())
    statusLabel_->hide();
  else {
    statusLabel_->placeBeside(host_->position(), cardAnchor());
    statusLabel_->show();
  }
}
void ResidentApplication::updateSessionMenu(const SessionSnapshot &snapshot) {
  QStringList signature;
  for (const auto& record : snapshot.active)
    signature << QString::fromStdString(record.key.sourceId) << QString::fromStdString(record.key.sessionId)
              << QString::fromStdString(record.label) << sessionStatusName(record.status)
              << (record.acknowledged ? "read" : "unread") << QString::number(record.sequence);
  if (signature == sessionMenuSignature_ && !sessionActions_.empty()) return;
  sessionMenuSignature_ = signature;
  qDeleteAll(sessionActions_);
  sessionActions_.clear();
  auto add = [&](QString text) {
    auto *action = new QAction(text.replace("&", "&&"), this);
    sessionsMenu_->addAction(action);
    sessionActions_.append(action);
    return action;
  };
  if (snapshot.active.empty())
    add("No active sessions")->setEnabled(false);
  for (const auto &record : snapshot.active) {
    const bool attention = record.status != SessionStatus::Working;
    auto *action =
        add(sessionText(record) + (record.acknowledged ? " (acknowledged)"
                                   : attention         ? " · Acknowledge"
                                                       : ""));
    action->setEnabled(attention && !record.acknowledged);
    if (attention && !record.acknowledged) {
      action->setObjectName("acknowledgeSession");
      connect(action, &QAction::triggered, this, [this, key = record.key, seq = record.sequence] {
        dispatchUtilityAction(key, seq, UtilityAction::Acknowledge);
      });
    }
  }
}
void ResidentApplication::applyReactionPose(double now) {
  if (spatialCharacter_) {
    spatialCharacter_->apply(*host_);
    updateLabel();
    return;
  }
  if (characterController_) {
    applyCharacterPose(now);
    return;
  }
  if (!reaction_) {
    host_->applyPose(motion_->pose());
    return;
  }
  BehaviorIntent intent = BehaviorIntent::Acknowledge;
  switch (reaction_->status) {
  case SessionStatus::Working:
    break;
  case SessionStatus::NeedsInput:
    intent = BehaviorIntent::Attend;
    break;
  case SessionStatus::Completed:
    intent = BehaviorIntent::Celebrate;
    break;
  case SessionStatus::Failed:
    intent = BehaviorIntent::ExpressFailure;
    break;
  }
  host_->applyPose(
      reactionPose(motion_->pose(), intent,
                   (now - reaction_->startedAt) /
                       (reaction_->endsAt - reaction_->startedAt)));
}
character::ExpressionMotion *ResidentApplication::expressions() {
  if (spatialCharacter_) return &spatialCharacter_->expressions();
  if (groundedController_) return &groundedController_->expressions();
  return characterController_ ? &characterController_->expressions() : nullptr;
}
void ResidentApplication::wakeMotion(double now) {
  const auto &s = runtime_.state();
  if (!s.visible || s.paused || s.suspended || s.quitting) return;
  if (hasGroundMotion()) {
    if (!groundSettled() && !groundDeadline_) { lastGroundTime_ = now; groundDeadline_ = now + 1.0/60; }
  } else if (characterController_ && !characterController_->settled() && !springDeadline_) {
    lastSpringTime_ = now; springDeadline_ = now + 1.0/60;
  }
}
void ResidentApplication::resetCuriosity() {
  const double now = clock_();
  manualNavigation_ = false; blockedNavigation_ = false; reactionSubmitted_ = false;
  behavior::Capabilities capabilities;
  if (auto *motion = expressions()) {
    motion->reset();
    for (unsigned index = 0; index < unsigned(character::Role::Count); ++index) {
      auto role = static_cast<character::Role>(index);
      if (motion->supports(role)) capabilities.roles |= character::roleBit(role);
    }
  }
  if (groundedController_) groundedController_->setAction(character::Action::Rest, 0);
  else if (characterController_) characterController_->setAction(character::Action::Rest, 0);
  capabilities.navigation = hasGroundMotion();
  capabilities.reach = spatialCharacter_ && expressions() && expressions()->supports(character::Role::WristLeft);
  auto profile = behavior::Profile::Fallback;
  if (preferences_.character == "soft-creature") profile = behavior::Profile::Mallow;
  else if (preferences_.character == "werewolf-3d") profile = behavior::Profile::Werewolf;
  else if (preferences_.character == "werewolf") profile = behavior::Profile::Bramble;
  curiosity_.reset(now, profile, capabilities);
  blink_.reset(now);
  blinkSupported_ = spatialCharacter_ ? spatialCharacter_->blinkSupported()
      : characterController_ && characterController_->definition().bindings.count("eyes");
  blinkDirty_ = false;
  cursorPosition_.reset(); cursorDirection_.reset(); cursorDeadline_.reset();
  cursorTurnAfter_=now;
  perchTargetId_.clear(); flightLandAt_.reset(); perchDescending_=false; cruising_=false;
  // Landing resets expressive behavior, but must not restart the pending
  // perch cooldown each time. A new raven gets its first quiet opportunity.
  if(!flightCapable()) ravenDeadline_.reset();
  else if(!ravenDeadline_ || *ravenDeadline_<=now) ravenDeadline_=now+8;
  runtime_.setAmbientEnabled(!characterController_ && !spatialCharacter_, now);
  advanceCuriosity(now);
  submitReaction(now);
}
void ResidentApplication::advanceBlink(double now) {
  if (!blinkSupported_) return;
  const auto &state = runtime_.state();
  const bool suppressed = !state.visible || state.paused || state.suspended || state.quitting || state.dragging;
  blinkDirty_ = blink_.advance(now,suppressed) || blinkDirty_;
  if (spatialCharacter_) spatialCharacter_->setEyeOpenness(blink_.openness());
}
void ResidentApplication::advanceCuriosity(double now) {
  advanceBlink(now);
  if (!characterController_ && !spatialCharacter_) return;
  const auto &s = runtime_.state();
  behavior::Observation observation;
  observation.suppressed = !s.visible || s.paused || s.suspended || s.quitting || s.dragging || dropBusy();
  observation.reactionBusy = reaction_.has_value();
  observation.width = characterController_ ? characterController_->definition().width
                                           : std::max(1, host_->presentationSize().width());
  if (const auto state = groundedState()) {
    observation.rootX = state->rootX;
    const auto span = host_->groundSpan(); observation.spanMin = span.minimum; observation.spanMax = span.maximum;
    observation.arrived = std::abs(state->rootX-state->targetX) < .05 && std::abs(state->velocity) < .02
        && !state->feet[0].swing && !state->feet[1].swing;
    if (manualNavigation_ && observation.arrived && now >= manualUntil_) manualNavigation_ = false;
  }
  observation.manualBusy = manualNavigation_;
  observation.blocked = blockedNavigation_; blockedNavigation_ = false;
  const bool cursorAllowed = !observation.suppressed && !observation.manualBusy && !observation.reactionBusy
      && expressions()->supports(character::Role::Head);
  if (!cursorAllowed) {
    cursorDeadline_.reset(); cursorPosition_.reset();
  } else {
    if (cursorDeadline_ && *cursorDeadline_ > now+.5) cursorDeadline_.reset();
    if (!cursorDeadline_ || now >= *cursorDeadline_) cursorPosition_ = cursorSource_();
    if (cursorPosition_) {
      QPointF head;
      if (spatialCharacter_) head = spatialCharacter_->headAnchor();
      else {
        const auto &definition = characterController_->definition();
        const auto anchors = character::utilityAnchors(definition, characterPose_);
        const double margin = std::ceil(definition.motionEnvelope);
        head = QPointF(anchors.head.x+margin, anchors.head.y+margin);
      }
      const QPointF displacement = *cursorPosition_-QPointF(host_->position())-head;
      // The 3D canvas includes large transparent margins; proximity follows
      // the projected pet rather than the entire presentation window.
      const double cursorWidth = characterController_ ? observation.width
          : std::max(1,host_->interactionRegion().boundingRect().width());
      observation.cursorOffset = motion::Vec3{displacement.x()/cursorWidth,
                                             -displacement.y()/cursorWidth,0};
    }
  }
  const auto commands = curiosity_.advance(now, observation);
  if (cursorAllowed && (!cursorDeadline_ || now >= *cursorDeadline_))
    cursorDeadline_ = now+(curiosity_.activity()==behavior::Activity::Attention ? .1 : .5);
  for (const auto &command : commands) {
    auto *motion = expressions();
    using Kind = behavior::Command::Kind;
    switch (command.kind) {
    case Kind::Look:
      if (curiosity_.activity()==behavior::Activity::Attention) cursorDirection_=command.value;
      else motion->look(command.value);
      break;
    case Kind::RoleGoal: motion->goal(command.role, command.value); break;
    case Kind::RoleImpulse: motion->impulse(command.role, command.value); break;
    case Kind::Clear: motion->clear(command.role); break;
    case Kind::Reach: if (spatialCharacter_) spatialCharacter_->reach(command.side, command.value); break;
    case Kind::ReleaseReach: if (spatialCharacter_) spatialCharacter_->releaseReach(command.side); break;
    case Kind::Navigate:
    case Kind::StopNavigation: {
      const double target = command.kind == Kind::Navigate ? command.targetX : observation.rootX;
      if (spatialCharacter_) spatialCharacter_->target(target);
      else if (groundedController_) groundedController_->setTargetX(target);
      break;
    }
    }
  }
  if (curiosity_.activity()==behavior::Activity::Attention && cursorDirection_) {
    if (cursorTurnAfter_>now+.8) cursorTurnAfter_=now;
    // A clear lateral side change invites an in-place turn. Center crossings
    // retain the heading and a short cooldown avoids chasing pointer jitter.
    if (cursorPosition_ && observation.cursorOffset
        && std::abs(observation.cursorOffset->x)>.35 && now>=cursorTurnAfter_) {
      const bool turned=spatialCharacter_ ? spatialCharacter_->face(cursorPosition_->x())
          : groundedController_ && groundedController_->face(cursorPosition_->x());
      if (turned) {
        cursorTurnAfter_=now+.8;
        wakeMotion(now);
      }
    }
    auto direction=*cursorDirection_;
    if (spatialCharacter_) direction=spatialCharacter_->cursorLookDirection(direction);
    else {
      // A visual ancestor may mirror the head while the grounded body turns.
      // Convert lateral gaze into that frame; vertical pitch retains its meaning.
      const auto &definition=characterController_->definition();
      const auto head=definition.bindings.find("head");
      if (head!=definition.bindings.end() &&
          character::worldTransforms(definition,characterPose_)[head->second].m00<0)
        direction.x=-direction.x;
    }
    // Refresh a held target while its body frame moves; existing motion ticks
    // carry the change, and a settled frame gets no new animation deadline.
    expressions()->look(direction);
  } else cursorDirection_.reset();
  if (!commands.empty()) wakeMotion(now);
}
void ResidentApplication::submitReaction(double now) {
  reactionDeadline_.reset();
  if (!characterController_ && !spatialCharacter_) return;
  if (!reaction_ && !reactionSubmitted_) return;
  const auto &s = runtime_.state();
  if (s.paused || s.suspended || s.dragging || !s.visible || dropBusy()) return;
  character::Action action = character::Action::Rest;
  QString semantic;
  double progress = 1;
  if (reaction_) {
    progress = std::clamp((now-reaction_->startedAt)/(reaction_->endsAt-reaction_->startedAt), 0.0, 1.0);
    switch (reaction_->status) {
    case SessionStatus::Working: action = character::Action::Acknowledge; semantic = "nod"; break;
    case SessionStatus::NeedsInput: action = character::Action::Attend; semantic = "look"; break;
    case SessionStatus::Completed: action = character::Action::Celebrate; semantic = "success"; break;
    case SessionStatus::Failed: action = character::Action::Failure; semantic = "failure"; break;
    }
  }
  if (reaction_) {
    const double duration = reaction_->endsAt-reaction_->startedAt;
    const double anticipateEnd = reaction_->startedAt + duration*.12;
    const double releaseAt = reaction_->startedAt + duration*.7;
    if (now < anticipateEnd) reactionDeadline_ = anticipateEnd;
    else if (now < releaseAt) reactionDeadline_ = releaseAt;
    if (now >= releaseAt) progress = std::max(progress, .7);
    else if (now >= anticipateEnd) progress = std::max(progress, .12);
    if (!spatialCharacter_) {
      if (now < anticipateEnd) action = character::Action::Attend;
      else if (now >= releaseAt) action = character::Action::Rest;
    }
  }
  if (spatialCharacter_) spatialCharacter_->reaction(semantic, progress);
  else if (groundedController_) groundedController_->setAction(action, progress);
  else characterController_->setAction(action, progress);
  reactionSubmitted_ = reaction_.has_value();
  wakeMotion(now);
}
void ResidentApplication::applyBasePose() {
  if (spatialCharacter_) {
    spatialCharacter_->apply(*host_);
    return;
  }
  if (characterController_) {
    characterPose_ = groundedController_ ? groundedController_->pose(character::Action::Rest, 0) :
        characterController_->pose(character::Action::Rest, characterTime_);
    characterPose_.eyeOpenness=blinkSupported_ ? blink_.openness() : 1;
    host_->applyPose(characterPose_);
  } else if (motion_)
    host_->applyPose(motion_->pose());
}
void ResidentApplication::applyCharacterPose(double, bool) {
  characterPose_ = groundedController_ ? groundedController_->pose(character::Action::Rest, 0)
                                      : characterController_->pose(character::Action::Rest, 0);
  characterPose_.eyeOpenness=blinkSupported_ ? blink_.openness() : 1;
  host_->applyPose(characterPose_);
}
void ResidentApplication::clearSpringMotion() {
  springDeadline_.reset();
  if (characterController_)
    characterController_->resetDynamics();
  lastSpringTime_ = clock_();
}
void ResidentApplication::dragMotion(QPointF displacement) {
  const auto &s = runtime_.state();
  if (hasGroundMotion()) {
    if (s.paused || s.suspended || !s.visible) dragSamples_.clear();
    else sampleDrag(clock_());
    reconcileGround();
    if (spatialCharacter_) spatialCharacter_->hold(host_->groundRootX(),host_->elevation());
    else groundedController_->hold(host_->groundRootX(),host_->elevation());
    if (!s.visible || s.paused || s.suspended || s.quitting) return;
    if (groundedController_) groundedController_->impulse({-displacement.x(),0},
        std::min(2.,std::abs(displacement.x())/24.));
    wakeMotion(clock_()); schedule(); return;
  }
  if (!characterController_) return;
  if (!s.visible || s.paused || s.suspended || s.quitting)
    return;
  const double now = clock_();
  // Host displacement, capped before it reaches the spring layer, expresses
  // inertia opposite the direction of dragging. No window force is applied.
  characterController_->impulse(
      {-std::clamp(displacement.x(), -40.0, 40.0),
       -std::clamp(displacement.y(), -40.0, 40.0)},
      std::clamp(std::hypot(displacement.x(), displacement.y()) / 12.0, 0.0,
                 4.0));
  if (!springDeadline_) {
    lastSpringTime_ = now;
    springDeadline_ = now + 1.0 / 60.0;
  }
  schedule();
}
bool ResidentApplication::selectCharacter(const QString &directory,
                                          QString *error) {
  if (stopped_) {
    if (error)
      *error = "Application is shutting down";
    return false;
  }
  try {
    if (isSpatialPackage(directory)) {
      QFile manifest(directory+"/manifest.json");
      if(!manifest.open(QIODevice::ReadOnly)) throw std::runtime_error("cannot read spatial manifest");
      const QString spatialId=QJsonDocument::fromJson(manifest.readAll()).object().value("id").toString();
      const auto resolved = characterDirectoryForId(characterRoot_, spatialId);
      if (QFileInfo(resolved).canonicalFilePath() != QFileInfo(directory).canonicalFilePath())
        throw std::runtime_error("3D character is outside the configured catalog");
      auto replacement = std::make_unique<SpatialCharacter>(directory);
      replacement->setSpeed(preferences_.walkingSpeed);
      realizingGround_ = true;
      try { replacement->install(*host_); } catch (...) { realizingGround_ = false; throw; }
      realizingGround_ = false;
      spatialCharacter_ = std::move(replacement);
      characterController_.reset(); groundedController_.reset(); motion_.reset();
      springDeadline_.reset();
      preferences_.character = spatialId;
      rebaseGround(); applyBasePose();
      toolbox_->setWalkingSpeed(preferences_.walkingSpeed, true);
      updateMenu(); updateLabel();
      if (characterErrorAction_) characterErrorAction_->setVisible(false);
      resetCuriosity();
      persist(); schedule();
      if (error) error->clear();
      return true;
    }
    const auto loaded = character::loadCharacter(directory);
    if (!characterRoot_.isEmpty()) {
      const auto resolved = characterDirectoryForId(
          characterRoot_, QString::fromStdString(loaded.definition.id));
      if (QFileInfo(resolved).canonicalFilePath() !=
          QFileInfo(directory).canonicalFilePath())
        throw std::runtime_error("character is outside the configured catalog; "
                                 "launch with --character for another catalog");
    }
    auto controller =
        std::make_unique<character::Controller>(loaded.definition);
    auto grounded = loaded.definition.locomotion ? std::make_unique<character::GroundedController>(loaded.definition) : nullptr;
    if (grounded) grounded->setSpeedMultiplier(preferences_.walkingSpeed);
    const auto pose = controller->pose(character::Action::Rest, 0);
    realizingGround_ = true;
    try { host_->setCharacter(loaded); } catch (...) { realizingGround_ = false; throw; }
    realizingGround_ = false;
    spatialCharacter_.reset();
    characterController_ = std::move(controller);
    groundedController_ = std::move(grounded);
    toolbox_->setWalkingSpeed(preferences_.walkingSpeed, hasGroundMotion());
    rebaseGround();
    motion_.reset();
    characterTime_ = 0;
    characterPose_ = groundedController_ ? groundedController_->pose(character::Action::Rest, 0) : pose;
    springDeadline_.reset();
    lastSpringTime_ = clock_();
    preferences_.character = QString::fromStdString(loaded.definition.id);
    characterPose_.eyeOpenness=1;
    host_->applyPose(characterPose_);
    resetCuriosity();
    updateMenu(); updateLabel();
    if (characterErrorAction_)
      characterErrorAction_->setVisible(false);
    persist();
    schedule();
    if (error)
      error->clear();
    return true;
  } catch (const std::exception &e) {
    if (error)
      *error = QString::fromUtf8(e.what());
    return false;
  }
}
std::optional<character::GroundedState> ResidentApplication::groundedState() const {
  if (spatialCharacter_) return spatialCharacter_->state();
  return groundedController_ ? std::optional<character::GroundedState>{groundedController_->state()} : std::nullopt;
}
void ResidentApplication::rebaseGround(bool preserveSecondary) {
  groundDeadline_.reset(); lastGroundTime_ = clock_();
  if (!hasGroundMotion() || !host_) return;
  if (spatialCharacter_) { spatialCharacter_->reset(*host_); spatialCharacter_->reconcile(*host_); }
  else {
    groundedController_->reset(host_->groundRootX(), host_->groundSpan(), host_->floorWorldY(), preserveSecondary);
    groundedController_->reconcile(host_->groundRootX(),host_->groundSpan(),host_->floorWorldY(),host_->elevation());
  }
  configureSupportFootprint();
  supportFrameId_=host_->selectedSurfaceId();
  if(spatialCharacter_) spatialCharacter_->setSupportFrame(*host_);
  const auto &state = runtime_.state();
  if ((preserveSecondary || dropBusy()) && !state.paused && state.visible && !state.suspended && !groundSettled())
    groundDeadline_ = lastGroundTime_ + 1.0/60;
}
void ResidentApplication::configureSupportFootprint() {
  if(!hasGroundMotion()) return;
  if(spatialCharacter_) {
    const auto footprint=spatialCharacter_->supportFootprint();
    host_->setSupportFootprint(footprint.first,footprint.second);
  } else {
    const auto s=groundedController_->state();
    host_->setSupportFootprint(std::min(s.feet[0].position.x,s.feet[1].position.x)-s.rootX-2,
        std::max(s.feet[0].position.x,s.feet[1].position.x)-s.rootX+2);
    groundedController_->setSupportBounds(soleBounds(*host_));
  }
}
void ResidentApplication::reconcileGround() {
  if (!hasGroundMotion()) return;
  const auto previous=groundedState();
  const auto id=host_->selectedSurfaceId();
  const bool translated=previous && previous->phase!=character::DropPhase::Held &&
      previous->phase!=character::DropPhase::Airborne &&
      !runtime_.state().dragging && !id.empty() && id==supportFrameId_;
  if (spatialCharacter_) spatialCharacter_->reconcile(*host_);
  else groundedController_->reconcile(host_->groundRootX(),host_->groundSpan(),host_->floorWorldY(),host_->elevation());
  if(translated && host_->groundRootX()!=previous->rootX) {
    const double target=previous->targetX+host_->groundRootX()-previous->rootX;
    if(spatialCharacter_) spatialCharacter_->target(target);
    else groundedController_->setTargetX(target);
  }
  supportFrameId_=id;
  if(groundedController_) groundedController_->setSupportBounds(soleBounds(*host_));
  else if(spatialCharacter_) spatialCharacter_->setSupportFrame(*host_);
  wakeMotion(clock_());
}
bool ResidentApplication::dropBusy() const {
  const auto state=groundedState();
  return state && state->phase!=character::DropPhase::Grounded;
}
void ResidentApplication::sampleDrag(double now) {
  if (!std::isfinite(now)) return;
  const QPointF position(host_->groundRootX(),host_->groundWorldY());
  if (!dragSamples_.empty() && now<=dragSamples_.back().time) {
    // Equal timestamps replace the sample, never form a velocity interval.
    dragSamples_.back().position=position; return;
  }
  while (!dragSamples_.empty() && now-dragSamples_.front().time>.06) dragSamples_.pop_front();
  dragSamples_.push_back({now,position});
  while (dragSamples_.size()>12) dragSamples_.pop_front();
}
QPointF ResidentApplication::releaseVelocity(double now) const {
  if (dragSamples_.size()<2 || now-dragSamples_.back().time>.06) return {};
  const auto &first=dragSamples_.front(); const auto &last=dragSamples_.back();
  const double dt=last.time-first.time;
  if (dt<.008 || !std::isfinite(dt)) return {};
  const auto v=(last.position-first.position)/dt;
  if (!std::isfinite(v.x()) || !std::isfinite(v.y())) return {};
  // Preserve measured release momentum; flight resistance acts continuously.
  return v;
}
void ResidentApplication::endDrag(bool released) {
  if (released) sampleDrag(clock_());
  if (released && hasGroundMotion() && host_->acquireSupportAt(clock_())) dragSamples_.clear();
  if (!released) dragSamples_.clear();
  control(Control::EndDrag);
}
bool ResidentApplication::setWalkingSpeed(double multiplier) {
  if (stopped_ || !std::isfinite(multiplier) || multiplier < 0.5 || multiplier > 3.0) return false;
  if (multiplier == preferences_.walkingSpeed) return true;
  preferences_.walkingSpeed = multiplier;
  if (groundedController_) groundedController_->setSpeedMultiplier(multiplier);
  if (spatialCharacter_) spatialCharacter_->setSpeed(multiplier);
  toolbox_->setWalkingSpeed(multiplier, hasGroundMotion());
  persist();
  return true;
}
void ResidentApplication::advanceSurfaces(double now) {
  const auto &s=runtime_.state();
  if(stopped_ || !hasGroundMotion() || !s.visible || s.paused || s.suspended) {
    surfaceDeadline_.reset(); return;
  }
  if(surfaceDeadline_ && now<*surfaceDeadline_) return;
  bool available=false;
  if(surfaceSource_) {
    auto snapshot=surfaceSource_(now);
    available=snapshot.available;
    host_->setSurfaceObservations(std::move(snapshot.surfaces),now);
  } else available=host_->refreshSurfaces(now);
  surfaceDeadline_=available ? std::optional<double>{now+.5} : std::optional<double>{};
}
bool ResidentApplication::flyToWindow() {
  const auto &state=runtime_.state();
  if(!flightCapable() || stopped_ || !state.visible || state.paused || state.suspended || state.dragging) return false;
  configureSupportFootprint();
  advanceSurfaces(clock_());
  const character::SupportSurface *chosen=nullptr;
  double score=std::numeric_limits<double>::max(), x=0;
  const auto footprint=host_->supportFootprint();
  const double left=footprint.first,right=footprint.second;
  for(const auto &surface:host_->supportSurfaces()) {
    if(surface.id==host_->selectedSurfaceId()) continue;
    for(const auto &segment:surface.segments) {
      const auto desktop=host_->desktopSpan();
      const double minimum=std::max(segment.minimum-left+4,desktop.minimum);
      const double maximum=std::min(segment.maximum-right-4,desktop.maximum);
      if(minimum>maximum) continue;
      const double candidate=std::clamp(host_->groundRootX(),minimum,maximum);
      if(!character::usableSurface(surface,candidate,left,right,clock_())) continue;
      // Keep a readable approach above the edge inside this monitor.
      const double approachY=std::min(surface.topY,host_->groundWorldY())-50;
      const double elevation=host_->desktopFloorWorldY()-approachY;
      if(approachY<host_->minimumGroundWorldY() || elevation<30 || elevation>1600) continue;
      const double cost=std::abs(candidate-host_->groundRootX())+std::abs(surface.topY-host_->groundWorldY())*.35;
      if(cost<score) {chosen=&surface;score=cost;x=candidate;}
    }
  }
  if(!chosen) return false;
  const QString id=QString::fromStdString(chosen->id);
  const QPointF approach(x,std::min(chosen->topY,host_->groundWorldY())-50);
  const double clearance=chosen->topY-approach.y();
  if(!flyTo(x,host_->floorWorldY()-approach.y())) return false;
  perchTargetId_=id; perchApproach_=approach; perchApproachClearance_=clearance; perchDescending_=false;
  flightLandAt_=clock_()+12;
  return true;
}
bool ResidentApplication::flyForward(double elevation) {
  const auto state=groundedState();
  if(!state)return false;
  const auto span=host_->desktopSpan();
  const double margin=std::min(24.,(span.maximum-span.minimum)*.2);
  int direction=state->facing;
  if(direction>0 && span.maximum-state->rootX<60)direction=-1;
  if(direction<0 && state->rootX-span.minimum<60)direction=1;
  const double destination=direction>0?span.maximum-margin:span.minimum+margin;
  const double targetY=host_->floorWorldY()-elevation;
  if(!flyTo(destination,elevation))return false;
  cruising_=true;cruiseDirection_=direction;cruiseTargetX_=destination;
  cruiseElevation_=host_->desktopFloorWorldY()-std::max(targetY,host_->minimumGroundWorldY());
  return true;
}
void ResidentApplication::advanceRaven(double now) {
  const auto &s=runtime_.state();
  if(stopped_ || !s.visible || s.paused || s.suspended || s.dragging) {
    ravenDeadline_.reset(); flightLandAt_.reset(); return;
  }
  if(cruising_) {
    const auto state=groundedState();
    if(!state || !state->poweredFlight)cruising_=false;
    else {
      const auto span=host_->desktopSpan();
      const double margin=std::min(24.,(span.maximum-span.minimum)*.2);
      const double destination=cruiseDirection_>0?span.maximum-margin:span.minimum+margin;
      const double braking=std::max(35.,std::abs(state->velocity)*.6);
      const bool reverse=(destination-state->rootX)*cruiseDirection_<braking;
      if(reverse || std::abs(destination-cruiseTargetX_)>1) {
        const double height=cruiseElevation_;const int direction=reverse?-cruiseDirection_:cruiseDirection_;
        const double target=direction>0?span.maximum-margin:span.minimum+margin;
        if(flyTo(target,height)) {
          cruising_=true;cruiseDirection_=direction;cruiseElevation_=height;cruiseTargetX_=target;
        }
      }
    }
  }
  if(!perchTargetId_.isEmpty()) {
    const auto &surfaces=host_->supportSurfaces();
    const auto found=std::find_if(surfaces.begin(),surfaces.end(),[&](const auto &edge) {
      return edge.id==perchTargetId_.toStdString() && now>=edge.observedAt && now-edge.observedAt<=edge.maxAge;
    });
    if(found==surfaces.end() || (flightLandAt_ && now>=*flightLandAt_)) { land(); }
    else if(!perchDescending_) {
      const double desiredY=found->topY-perchApproachClearance_;
      const auto feet=host_->supportFootprint();
      const auto desktop=host_->desktopSpan();
      std::optional<double> desiredX;
      double distance=std::numeric_limits<double>::max();
      for(const auto &segment:found->segments) {
        const double minimum=std::max(segment.minimum-feet.first+4,desktop.minimum);
        const double maximum=std::min(segment.maximum-feet.second-4,desktop.maximum);
        if(minimum>maximum) continue;
        const double candidate=std::clamp(perchApproach_.x(),minimum,maximum);
        if(std::abs(candidate-perchApproach_.x())<distance) {
          desiredX=candidate; distance=std::abs(candidate-perchApproach_.x());
        }
      }
      if(!desiredX || desiredY<host_->minimumGroundWorldY()) {land();return;}
      if(std::abs(perchApproach_.y()-desiredY)>2 || std::abs(perchApproach_.x()-*desiredX)>1) {
        const auto id=perchTargetId_; const auto deadline=flightLandAt_;
        flyTo(*desiredX,host_->floorWorldY()-desiredY);
        perchTargetId_=id; flightLandAt_=deadline; perchApproach_=QPointF(*desiredX,desiredY);
      }
      const auto state=groundedState();
      const auto footprint=host_->supportFootprint();
      const bool solesFit=character::usableSurface(*found,host_->groundRootX(),footprint.first,footprint.second,now).has_value();
      // Start the descent over usable support, rather than waiting to hover
      // exactly at the steering target. Reserve the next half-second of drift
      // inside the same visible edge before releasing lift.
      const double drift=state?state->velocity*.5:0.;
      const bool landingFits=state && character::usableSurface(*found,host_->groundRootX(),
          footprint.first+std::min(0.,drift)-4,
          footprint.second+std::max(0.,drift)+4,now).has_value();
      if(state && solesFit && landingFits &&
          std::abs(host_->groundWorldY()-perchApproach_.y())<30 &&
          std::abs(state->verticalVelocity)<100) {
        const auto id=perchTargetId_;
        land(); perchTargetId_=id; perchDescending_=true;
      }
    } else if(!dropBusy()) { perchTargetId_.clear(); flightLandAt_.reset(); }
  }
  if(flightLandAt_ && now>=*flightLandAt_) land();
  if(!flightCapable()) {ravenDeadline_.reset();return;}
  const auto idleState=groundedState();
  // Quantized or blocked navigation can settle short of its old requested
  // coordinate. Physical rest releases manual ownership just as arrival does.
  if(manualNavigation_ && now>=manualUntil_ && idleState &&
      idleState->phase==character::DropPhase::Grounded && groundSettled())
    manualNavigation_=false;
  if(!ravenDeadline_) ravenDeadline_=now+8;
  if(now<*ravenDeadline_) return;
  if(manualNavigation_ || reaction_ || dropBusy() || !groundSettled()) {
    ravenDeadline_=now+2; return;
  }
  // A window is already a useful idle resting place. Keep it instead of
  // repeatedly departing for another perch after each landing.
  if(!host_->selectedSurfaceId().empty()) {ravenDeadline_=now+45;return;}
  ravenDeadline_=now+30;
  // An unavailable perch is a quiet retry, not an unsolicited hop.
  flyToWindow();
}
bool ResidentApplication::flightCapable() const {
  return spatialCharacter_ ? spatialCharacter_->flightCapable()
                           : (groundedController_ && groundedController_->flightCapable());
}
bool ResidentApplication::hop(double height) {
  const auto &s=runtime_.state();
  if (!hasGroundMotion() || stopped_ || !s.visible || s.paused || s.suspended ||
      s.dragging || dropBusy() || !std::isfinite(height) || height<=0 || height>160) return false;
  const bool accepted=spatialCharacter_ ? spatialCharacter_->hop(height) : groundedController_->hop(height);
  if(!accepted) return false;
  host_->clearSupport(); reconcileGround();
  manualNavigation_=true; flightLandAt_.reset(); perchTargetId_.clear();
  manualUntil_=clock_()+.25;
  wakeMotion(clock_()); schedule(); return true;
}
bool ResidentApplication::flyTo(double worldX,double elevation) {
  const auto &s=runtime_.state();
  if(!flightCapable() || stopped_ || !s.visible || s.paused || s.suspended ||
      s.dragging || !std::isfinite(worldX) || !std::isfinite(elevation) ||
      elevation<0 || elevation>4096) return false;
  const auto phase=groundedState()->phase;
  if(phase==character::DropPhase::Landing || phase==character::DropPhase::Recovering) return false;
  cruising_=false;
  const double targetY=host_->floorWorldY()-elevation;
  // Push off before rebasing a perched floor; otherwise it becomes a passive
  // drop before the wings have built enough stroke speed to lift the body.
  if(!dropBusy() && !host_->selectedSurfaceId().empty()) {
    const bool launched=spatialCharacter_ ? spatialCharacter_->flyTo(host_->groundRootX(),elevation)
                                        : groundedController_->flyTo(host_->groundRootX(),elevation);
    if(!launched)return false;
  }
  host_->clearSupport();
  reconcileGround();
  const auto span=host_->groundSpan();
  worldX=std::clamp(worldX,span.minimum,span.maximum);
  elevation=std::max(0.,host_->floorWorldY()-targetY);
  const double maxHeight=std::max(0.,host_->floorWorldY()-host_->minimumGroundWorldY());
  elevation=std::min(elevation,maxHeight);
  const bool accepted=spatialCharacter_ ? spatialCharacter_->flyTo(worldX,elevation)
                                      : groundedController_->flyTo(worldX,elevation);
  if(!accepted) return false;
  manualNavigation_=true; manualUntil_=clock_()+.25;
  flightLandAt_.reset(); perchTargetId_.clear(); perchDescending_=false;
  wakeMotion(clock_()); schedule(); return true;
}
bool ResidentApplication::land() {
  const auto &s=runtime_.state();
  if(!hasGroundMotion() || stopped_ || !s.visible || s.paused || s.suspended || s.dragging) return false;
  cruising_=false;
  if(spatialCharacter_) spatialCharacter_->land(); else groundedController_->land();
  flightLandAt_.reset(); perchTargetId_.clear(); perchDescending_=false;
  wakeMotion(clock_()); schedule(); return true;
}
void ResidentApplication::setGroundTarget(double worldX) {
  const auto &s = runtime_.state();
  if (!hasGroundMotion() || stopped_ || !s.visible || s.paused || s.suspended || s.dragging || dropBusy() || !std::isfinite(worldX)) return;
  manualNavigation_ = true;
  manualUntil_ = clock_() + .25;
  advanceCuriosity(clock_());
  // Integer presentation is authoritative, including authored fractional origin.
  const auto target = std::round(worldX-host_->groundRootX()) + host_->groundRootX();
  if (spatialCharacter_) spatialCharacter_->target(target);
  else groundedController_->setTargetX(target);
  if (!groundDeadline_ && !groundSettled()) {
    lastGroundTime_=clock_(); groundDeadline_=lastGroundTime_+1.0/60;
  }
  schedule();
}
bool ResidentApplication::hasGroundMotion() const { return bool(groundedController_) || bool(spatialCharacter_); }
bool ResidentApplication::groundSettled() const {
  return spatialCharacter_ ? spatialCharacter_->settled() : (!groundedController_ || groundedController_->settled());
}
void ResidentApplication::persist() {
  if (!host_)
    return;
  preferences_.position = host_->position();
  preferences_.display = host_->displayName();
  preferences_.hasPosition = true;
  preferences_.paused = runtime_.state().paused;
  const bool failed = !savePreferences(settingsFile_, preferences_);
  if (failed && !settingsWriteFailed_)
    qWarning().noquote() << "error: could not save preferences:"
                         << settingsFile_;
  settingsWriteFailed_ = failed;
  if (settingsErrorAction_)
    settingsErrorAction_->setVisible(failed);
}
void ResidentApplication::updateMenu() {
  for (auto* action : findChildren<QAction*>())
    if (action->objectName() == "walkLeft" || action->objectName() == "walkRight" || action->objectName() == "stopWalking")
      action->setEnabled(hasGroundMotion());
    else if(action->objectName()=="hop" || action->objectName()=="land") action->setEnabled(hasGroundMotion());
    else if(action->objectName()=="fly" || action->objectName()=="perch") action->setEnabled(flightCapable());
  visibilityAction_->setText(runtime_.state().visible ? "Hide" : "Show");
  pauseAction_->setText(runtime_.state().paused ? "Resume" : "Pause");
}
void ResidentApplication::shutdown() {
  if (stopped_)
    return;
  stopped_ = true;
  timer_.stop();
  // Observer removal and callback invalidation precede destruction of the host
  // and menus. Context-bound Qt callbacks expire with this QObject.
  sleep_.reset();
  ipc_.reset();
  statusLabel_.reset();
  toolbox_.reset();
  persist();
  if (host_) {
    host_->draggingChanged = {};
    host_->placementChanged = {};
    host_->contextMenuRequested = {};
    host_->renderingError = {};
    host_->dragDisplacement = {};
    host_->groundChanged = {};
    host_.reset();
  }
  if (tray_) {
    tray_->hide();
    tray_->setContextMenu(nullptr);
    tray_.reset();
  }
  menu_.reset();
  if (instance_)
    instance_->release();
}
} // namespace pets
