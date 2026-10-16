#include "desktop_host.h"
#include "settings.h"
#include "characters/raster.h"
#include <QPainter>
#include <QApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMouseEvent>
#include <QQuickWindow>
#include <QQuickItem>
#include <QPointer>
#include <QScreen>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
void write(const QString& path, const QByteArray& bytes) {
    QFile file(path);
    check(file.open(QIODevice::WriteOnly), "test file opens");
    check(file.write(bytes) == bytes.size(), "test file writes");
}
const QVector<pets::DisplayGeometry> displays{
    {"primary", QRect(0, 20, 1920, 1060)},
    {"left", QRect(-1280, -200, 1280, 1024)}
};
void geometry() {
    const auto left = pets::placeOnDisplays({-5000, -5000}, {256, 256}, displays, "left");
    check(left.position == QPoint(-1280, -200) && left.display == "left", "negative display clamps to its own origin");
    const auto removed = pets::placeOnDisplays({-1100, 40}, {256, 256}, displays, "removed");
    check(removed.position == QPoint(0, 40) && removed.display == "primary", "missing saved display falls back to primary");
    const auto tiny = pets::placeOnDisplays({50, 80}, {256, 256}, {{"small", QRect(-80, 40, 100, 90)}});
    check(tiny.position == QPoint(-80, 40), "undersized screen anchors at available origin");
    const auto crossing = pets::placeOnDisplays({-200, 300}, {256, 256}, displays);
    check(crossing.display == "left" && crossing.position == QPoint(-200, 300), "reachable crossing preserves pointer target");
    const auto nearest = pets::placeOnDisplays({-4000, 300}, {256, 256}, displays);
    check(nearest.display == "left" && nearest.position == QPoint(-1280, 300), "outside target chooses nearest display");
    const auto rightEdge = pets::placeOnDisplays({1900, 2000}, {256, 256}, displays, "primary");
    check(rightEdge.position == QPoint(1664, 824), "inclusive rectangle edges retain entire window");
}
const QRegion interaction(QRect(80, 40, 100, 160));
void adjoiningDisplays() {
    const QVector<pets::DisplayGeometry> adjacent{
        {"primary", QRect(0, 0, 1920, 1080)}, {"right", QRect(1920, 0, 1920, 1080)}
    };
    // Hand-derived desired positions on either side of the center-selection
    // boundary. Switching selected displays must not alter the pointer offset.
    for (const int x : {1789, 1790, 1791, 1792, 1793, 1792, 1791, 1790, 1789}) {
        const auto p = pets::placeOnDisplays({x, 100}, {256, 256}, adjacent, {}, interaction);
        check(p.position == QPoint(x, 100), "adjacent display crossing follows each pointer pixel in both directions");
        check(p.display == (x < 1792 ? "primary" : "right"), "crossing records display under target center");
        const auto restored = pets::placeOnDisplays(p.position, {256, 256}, adjacent, p.display, interaction);
        check(restored.position == QPoint(x, 100), "reachable crossing position survives restoration and reconciliation");
    }
    const auto removed = pets::placeOnDisplays({1792, 100}, {256, 256}, {adjacent.front()}, "right", interaction);
    check(removed.position == QPoint(1664, 100) && removed.display == "primary", "removed saved display repairs onto primary despite partial reachability");
}
void interactionReachability() {
    const QVector<pets::DisplayGeometry> single{{"primary", QRect(0, 0, 1920, 1080)}};
    const auto partial = pets::placeOnDisplays({-90, 100}, {256, 256}, single, "primary", interaction);
    check(partial.position == QPoint(-90, 100), "outer edge preserves partially visible authored interaction region");
    const auto transparentOnly = pets::placeOnDisplays({-240, 100}, {256, 256}, single, {}, interaction);
    check(transparentOnly.position == QPoint(0, 100), "transparent window overlap cannot keep an unreachable pet offscreen");
    const auto rightEdge = pets::placeOnDisplays({1840, 100}, {256, 256}, single, {}, interaction);
    check(rightEdge.position == QPoint(1664, 100), "outer right edge repairs once interaction leaves displays");
    const auto tiny = pets::placeOnDisplays({1000, 1000}, {256, 256}, {{"tiny", QRect(0, 0, 10, 10)}}, {}, interaction);
    check(tiny.position == QPoint(-80, -40), "tiny screen repair places interactive pixels on display");
    // Disconnected authored regions: bounding-box overlap alone is insufficient.
    const QRegion separated = QRegion(QRect(0, 0, 10, 10)) | QRegion(QRect(100, 100, 10, 10));
    const auto hole = pets::placeOnDisplays({0, 0}, {256, 256}, {{"island", QRect(40, 40, 10, 10)}}, {}, separated);
    check(!separated.translated(hole.position).intersected(QRect(40, 40, 10, 10)).isEmpty(), "repair intersects actual mask rather than transparent holes");
}
void unequalAndGappedDisplays() {
    const QVector<pets::DisplayGeometry> uneven{
        {"primary", QRect(0, 0, 800, 600)}, {"right", QRect(850, 150, 400, 300)}
    };
    for (const int x : {740, 750, 760, 770, 760, 750, 740}) {
        const auto p = pets::placeOnDisplays({x, 200}, {256, 256}, uneven, {}, interaction);
        check(p.position == QPoint(x, 200), "unequal displays and small gap preserve reachable crossing");
    }
    const auto corner = pets::placeOnDisplays({760, 40}, {256, 256}, uneven, {}, interaction);
    check(corner.position == QPoint(760, 40), "vertical display offset preserves partly reachable interaction");
    const QVector<pets::DisplayGeometry> wideGap{
        {"primary", QRect(0, 0, 800, 600)}, {"right", QRect(1100, 150, 400, 300)}
    };
    const auto stranded = pets::placeOnDisplays({850, 200}, {256, 256}, wideGap, {}, interaction);
    check(stranded.position == QPoint(1100, 194) && stranded.display == "right", "unreachable gap repairs onto nearest available display");
}
void malformedSettings() {
    QTemporaryDir dir;
    const auto path = dir.filePath("preferences.json");
    write(path, R"({"version":1,"paused":true})");
    check(pets::loadPreferences(path).paused, "valid typed pause preference accepted before malformed cases");
    const QList<QByteArray> invalid{
        "not json", "[]", R"({"version":2,"paused":true})",
        R"({"version":1,"paused":"true"})",
        R"({"version":1,"walkingSpeed":"2"})", R"({"version":1,"walkingSpeed":0.4})",
        R"({"version":1,"walkingSpeed":3.1})", R"({"version":1,"position":{"x":1.5,"y":0}})",
        R"({"version":1,"position":{"x":1000001,"y":0},"paused":true})",
        R"({"version":1,"position":{"x":5},"paused":true})",
        R"({"version":1,"display":42,"paused":true})",
        R"({"version":1,"character":"../../foreign","paused":true})",
        QByteArray(16385, ' ')
    };
    for (const auto& bytes : invalid) {
        write(path, bytes);
        const auto p = pets::loadPreferences(path);
        check(!p.hasPosition && !p.paused && p.display.isEmpty() && p.character == "fixture-animal", "malformed settings reset all preferences safely");
    }
    const auto missing = pets::loadPreferences(dir.filePath("missing.json"));
    check(!missing.hasPosition && !missing.paused, "missing settings use defaults");
}
void settingsRoundTrip() {
    QTemporaryDir dir;
    const auto path = dir.filePath("preferences.json");
    pets::Preferences p;
    p.walkingSpeed = 2.75;
    p.position = {-1280, -200}; p.display = "External Display"; p.hasPosition = true; p.paused = true;
    check(pets::savePreferences(path, p), "preferences saved atomically");
    const auto loaded = pets::loadPreferences(path);
    check(loaded.hasPosition && loaded.position == QPoint(-1280, -200) && loaded.display == "External Display" && loaded.paused && loaded.walkingSpeed == 2.75, "typed placement and pause survive restart");
    QFile file(path); check(file.open(QIODevice::ReadOnly), "saved preferences readable");
    const auto before = file.readAll(); file.close();
    const auto object = QJsonDocument::fromJson(before).object();
    check(object.keys() == QStringList({"character", "display", "paused", "position", "version", "walkingSpeed"}), "only allowed preferences persisted");
    p.position.setX(1000001);
    check(!pets::savePreferences(path, p), "invalid preference write rejected");
    check(file.open(QIODevice::ReadOnly) && file.readAll() == before, "rejected write preserves previous complete file");
    file.close();
    p.position.setX(-1280);
    for (double speed : {0.0, 0.49, 3.01, 100.0}) {
        p.walkingSpeed = speed;
        check(!pets::savePreferences(path, p), "unsupported walking speed rejected before saving");
    }
    p = {};
    check(pets::savePreferences(path, p), "replace previous complete preferences");
    const auto defaults = pets::loadPreferences(path);
    check(!defaults.hasPosition && !defaults.paused, "absence of saved placement survives round trip");
    check(!pets::savePreferences(dir.filePath("absent/prefs.json"), p), "unwritable destination reports failure");
}
void characterPreferences() {
    QTemporaryDir dir;
    const auto path = dir.filePath("preferences.json");
    pets::Preferences p;
    p.character = "soft-creature"; p.paused = true;
    check(pets::savePreferences(path, p), "production character identity can be saved");
    check(pets::loadPreferences(path).character == "soft-creature", "production character selection survives restart");
    for (const auto& id : {QString("../escape"), QString("/absolute"), QString("with space"), QString(65, 'a')}) {
        p.character = id;
        check(!pets::savePreferences(path, p), "character identity cannot be an asset path");
    }
    check(pets::loadPreferences(path).character == "soft-creature", "invalid selection preserves previous settings");
}
QQuickWindow* petWindow() {
    for (auto* window : QGuiApplication::topLevelWindows())
        if (window->title() == "Better Pets") return dynamic_cast<QQuickWindow*>(window);
    throw std::runtime_error("host owns a discoverable pet window");
}
void mouse(QQuickWindow* window, QEvent::Type type, QPoint local, QPoint global,
           Qt::MouseButton button, Qt::MouseButtons buttons) {
    QMouseEvent event(type, QPointF(local), QPointF(global), button, buttons, Qt::NoModifier);
    QApplication::sendEvent(window, &event);
}
void reachableRelease(const QString& assets) {
    const auto rig = loadRig(assets);
    pets::DesktopHost host(rig, QImage(rig.atlas));
    const auto available = QGuiApplication::primaryScreen()->availableGeometry();
    host.restorePlacement(available.topLeft() + QPoint(20, 100), QGuiApplication::primaryScreen()->name());
    host.setPetVisible(true);
    auto* window = petWindow();
    const QPoint hit(120, 120);
    const auto start = host.position();
    const auto target = available.topLeft() + QPoint(-60, 100);
    mouse(window, QEvent::MouseButtonPress, hit, start + hit, Qt::LeftButton, Qt::LeftButton);
    mouse(window, QEvent::MouseMove, hit, target + hit, Qt::NoButton, Qt::LeftButton);
    check(host.position() == target, "real drag preserves reachable partially offscreen target");
    mouse(window, QEvent::MouseButtonRelease, hit, target + hit, Qt::LeftButton, Qt::NoButton);
    check(host.position() == target, "release retains realized pointer placement");
    const auto display = host.displayName();
    host.reconcileScreens();
    check(host.position() == target, "reconcile retains reachable released placement");
    host.setPetVisible(false); host.setPetVisible(true);
    check(host.position() == target, "show retains reachable released placement");
    host.restorePlacement(target, display);
    check(host.position() == target, "restart restoration retains reachable released placement");
}
void productionDropPlacement(const QString& assets) {
    pets::DesktopHost host(pets::character::loadCharacter(assets+"/../characters/soft-creature"));
    host.setPetVisible(true);
    auto *window=petWindow();
    bool release=false; int ended=0;
    host.dragEnded=[&](bool actualRelease){release=actualRelease;++ended;};
    const auto hit=host.interactionRegion().boundingRect().center();
    const auto start=host.position()+hit;
    mouse(window,QEvent::MouseButtonPress,hit,start,Qt::LeftButton,Qt::LeftButton);
    mouse(window,QEvent::MouseMove,hit,start-QPoint(0,120),Qt::NoButton,Qt::LeftButton);
    const auto elevated=host.position();
    mouse(window,QEvent::MouseButtonRelease,hit,elevated+hit,Qt::LeftButton,Qt::NoButton);
    check(ended==1&&release,"real release is distinguished from interruption");
    check(host.position()==elevated&&host.elevation()>100,"production release preserves height");
    host.reconcileScreens();
    check(host.position()==elevated,"screen reconciliation preserves elevation");
    const auto before=host.groundWorldY();
    for(int i=0;i<10;++i) host.moveInWorld(0,.1);
    check(std::abs(host.groundWorldY()-before-1)<.001,"vertical subpixel displacement accumulates");
    host.moveInWorld(0,1000);
    check(host.elevation()<.5,"floor stops large displacement without tunneling");
    mouse(window,QEvent::MouseButtonPress,hit,host.position()+hit,Qt::LeftButton,Qt::LeftButton);
    QEvent interrupted(QEvent::UngrabMouse); QApplication::sendEvent(window,&interrupted);
    check(ended==2&&!release,"lost mouse grab cannot cause throw momentum");
    const auto *screen=QGuiApplication::primaryScreen();
    const auto area=screen->availableGeometry();
    host.restorePlacement(area.topLeft()+QPoint(-20,100),screen->name());
    const auto partial=host.position();
    const auto edgeStep=host.moveInWorld(0,.1);
    check(host.position().x()==partial.x()&&!edgeStep.blocked,
        "zero horizontal flight preserves reachable partially offscreen placement");
}
void visualSilhouetteIsNotTheGrabPolygon() {
    pets::character::LoadedCharacter loaded;
    auto &d = loaded.definition;
    d.width = d.height = 64; d.motionEnvelope = 8;
    d.bones = {{"body", -1, {20, 20}}};
    d.attachments = {{0, {0, 0, 8, 8}, {}, {}, 0}};
    d.interaction = {{22, 22}, {26, 22}, {26, 26}, {22, 26}};
    loaded.atlas = QImage(8, 8, QImage::Format_ARGB32);
    loaded.atlas.fill(Qt::red);
    pets::DesktopHost host(loaded);
    host.setPetVisible(true);
    auto *window = petWindow();
    check(window->mask().contains(QPoint(29, 29)), "visible texture outside authored grab polygon is not cut off");
    check(!window->mask().contains(QPoint(0, 0)), "transparent corner is outside native mask");
    int starts = 0; host.draggingChanged = [&](bool active) { if(active) ++starts; };
    const auto position = host.position();
    mouse(window, QEvent::MouseButtonPress, {29, 29}, position + QPoint(29, 29), Qt::LeftButton, Qt::LeftButton);
    check(starts == 0, "visual fringe does not become grab geometry");
    pets::character::Pose pose; pose.bones = {{40, 20}};
    host.applyPose(pose);
    check(window->mask().contains(QPoint(49, 29)), "posed texture remains visible beyond rest silhouette");
    check(!window->mask().contains(QPoint(29, 29)), "old visual fringe is released after movement");
    mouse(window, QEvent::MouseButtonPress, {31, 31}, position + QPoint(31, 31), Qt::LeftButton, Qt::LeftButton);
    check(starts == 1, "authored grab region stays reachable after art moves");
    host.cancelDrag();
    host.draggingChanged = {};
}
void productionSilhouettes(const QString& assets) {
    using namespace pets::character;
    for (const auto *id : {"soft-creature", "werewolf"}) {
      const auto loaded = loadCharacter(assets + "/../characters/" + id);
      pets::DesktopHost host(loaded);
      auto *window = petWindow();
      const auto authored = host.interactionRegion();
      Controller controller(loaded.definition);
      const auto verify = [&](const pets::character::Pose &pose) {
        host.applyPose(pose);
        check(host.interactionRegion() == authored, "actions preserve authored grab geometry");
        RasterOptions options; options.resolution = 96;
        const auto raster = renderCharacter(loaded.definition, loaded.atlas, pose, options);
        const double side = std::max(window->width(), window->height());
        QImage logical(window->size(), QImage::Format_ARGB32_Premultiplied);
        logical.fill(Qt::transparent);
        QPainter painter(&logical);
        if (loaded.definition.locomotion) {
          painter.drawImage(QRectF((window->width()-side)/2, (window->height()-side)/2, side, side), raster);
        } else {
          const auto world = worldTransforms(loaded.definition, pose);
          for (const auto &a : loaded.definition.attachments) {
            const auto &t = world.at(a.bone);
            painter.setTransform(QTransform(t.m00, t.m10, t.m01, t.m11,
                t.tx + loaded.definition.motionEnvelope, t.ty + loaded.definition.motionEnvelope));
            painter.drawImage(QRectF(a.offset.x-a.pivot.x, a.offset.y-a.pivot.y, a.source.width, a.source.height),
                loaded.atlas, QRectF(a.source.x,a.source.y,a.source.width,a.source.height));
          }
        }
        painter.end();
        // Draw outside the declared canvas as well, so a crop inside the
        // compositor cannot hide a genuinely undersized motion envelope.
        QImage uncropped(window->size() + QSize(64,64), QImage::Format_ARGB32_Premultiplied);
        uncropped.fill(Qt::transparent);
        QPainter wide(&uncropped);
        const auto world = worldTransforms(loaded.definition, pose);
        for (const auto &a : loaded.definition.attachments) {
          const auto &t = world.at(a.bone);
          wide.setTransform(QTransform(t.m00,t.m10,t.m01,t.m11,
              t.tx+loaded.definition.motionEnvelope+32,t.ty+loaded.definition.motionEnvelope+32));
          wide.drawImage(QRectF(a.offset.x-a.pivot.x,a.offset.y-a.pivot.y,a.source.width,a.source.height),
              loaded.atlas,QRectF(a.source.x,a.source.y,a.source.width,a.source.height));
        }
        wide.end();
        const QRect canvas(QPoint(32,32),window->size());
        for(int y=0;y<uncropped.height();++y)
          for(int x=0;x<uncropped.width();++x)
            if(!canvas.contains(x,y))
              check(!uncropped.pixelColor(x,y).alpha(), "motion envelope contains all posed texture pixels before composition");
        for (int y=0; y<logical.height(); ++y)
          for (int x=0; x<logical.width(); ++x)
            if(logical.pixelColor(x,y).alpha())
              check(window->mask().contains(QPoint(x,y)), "rest, action and drag texture pixels survive native mask");
      };
      for (auto action : {Action::Rest, Action::Idle, Action::Walk, Action::Attend, Action::Celebrate, Action::Failure, Action::Acknowledge})
        for (int frame=0; frame<12; ++frame) verify(controller.pose(action, frame/12.0));
      controller.impulse({1,-1}, 4);
      for(int frame=0;frame<20;++frame) { controller.step(1.0/60); verify(controller.pose(Action::Idle, frame/60.0)); }
      if (loaded.definition.locomotion) {
        GroundedController gait(loaded.definition);
        double root = loaded.definition.origin.x;
        gait.reset(root, {-200,200}, loaded.definition.locomotion->groundY);
        for (const double target : {180.0,-100.0}) {
          gait.setTargetX(target);
          for(int frame=0;frame<600;++frame) {
            root += gait.propose(1.0/60).displacement;
            gait.realize(root);
            if(frame%6==0) verify(gait.pose());
          }
        }
      }
    }
}
void preparedPresentation(const QString& assets) {
    const auto rig = loadRig(assets);
    pets::DesktopHost host(rig, QImage(rig.atlas));
    auto *window = petWindow();
    auto *prepared = new QQuickItem;
    const QRegion grab(QRect(70, 90, 40, 70));
    const QRegion visual(QRect(40, 20, 100, 170));
    host.setPresentation(prepared, {180, 200}, {90, 180}, grab, visual);
    check(prepared->parentItem() == window->contentItem() && prepared->parent() == window->contentItem(),
          "prepared scene transfers visual and QObject ownership");
    check(window->contentItem()->childItems().size() == 1, "prepared scene replaces fixture renderer");
    check(host.presentationSize() == QSize(180, 200) && host.localGroundAnchor() == QPointF(90, 180),
          "presentation metadata defines local ground coordinates");
    check(window->mask() == (grab | visual) && host.interactionRegion() == grab,
          "visual shape preserves projected limbs separately from grab shape");
    const auto available = QGuiApplication::primaryScreen()->availableGeometry();
    check(host.groundWorldY() == available.y() + available.height(), "prepared scene realizes desktop floor");
    const auto before = host.groundRootX();
    const auto moved = host.moveOnGround(12);
    check(moved.rootX == before + 12 && !moved.blocked, "prepared scene follows realized ground movement");
    const QRegion posed(QRect(20, 25, 90, 150));
    host.updatePresentationMask(posed);
    check(window->mask() == (grab | posed), "projected mask updates without growing stale silhouettes");
    host.applyPose(evaluatePose(rig, 44, "idle-motion"));
    check(window->mask() == (grab | posed), "fixture pose cannot replace prepared scene mask");
    auto invalid = std::make_unique<QQuickItem>();
    bool rejected = false;
    try { host.setPresentation(invalid.get(), {180, 200}, {90, 180}, {}, visual); }
    catch (const std::runtime_error &) { rejected = true; }
    check(rejected && prepared->parentItem() == window->contentItem() &&
          invalid->parentItem() == nullptr && window->mask() == (grab | posed),
          "invalid scene metadata preserves active presentation and caller ownership");
    const QRegion movedGrab(QRect(50, 80, 30, 50));
    host.updatePresentationMask(posed, movedGrab);
    check(host.interactionRegion() == movedGrab && window->mask() == (movedGrab | posed),
          "projected grab can follow posed body independently of visual fringe");
    QPointer<QQuickItem> owned(prepared);
    const auto loaded = pets::character::loadCharacter(assets + "/../characters/soft-creature");
    host.setCharacter(loaded);
    check(owned.isNull() && window->contentItem()->childItems().size() == 1,
          "switching back to 2D destroys owned prepared scene");
}
void restoreGroundedFloor(const QString& assets) {
    const auto rig = loadRig(assets);
    pets::DesktopHost host(rig, QImage(rig.atlas));
    host.setPresentation(new QQuickItem, {180, 200}, {90, 180},
                         QRegion(QRect(70, 20, 40, 140)),
                         QRegion(QRect(40, 10, 100, 180)));
    const auto *screen = QGuiApplication::primaryScreen();
    const auto available = screen->availableGeometry();
    const int floor = available.y() + available.height();
    const QPoint offscreenFloor(available.left() + 20, floor - 100);
    check(host.interactionRegion().translated(offscreenFloor).intersects(available),
          "saved torso is reachable despite offscreen planted feet");
    host.restorePlacement(offscreenFloor, screen->name());
    check(host.groundWorldY() == floor &&
          host.position().x() == offscreenFloor.x(),
          "restoration repairs offscreen floor while preserving horizontal placement");
    const QPoint valid(available.left() + 20, floor - 220);
    host.restorePlacement(valid, screen->name());
    check(host.position() == valid,
          "valid saved grounded placement remains unchanged");
}
void hostLifecycle(const QString& assets) {
    const auto rig = loadRig(assets);
    pets::DesktopHost host(rig, QImage(rig.atlas));
    check(!host.isPetVisible(), "constructor leaves visibility to shell");
    host.restorePlacement({-999999, -999999}, "removed");
    auto* window = petWindow();
    const auto available = QGuiApplication::primaryScreen()->availableGeometry();
    check(host.position() == available.topLeft(), "real host restores removed display on primary");
    check(window->flags().testFlag(Qt::WindowDoesNotAcceptFocus) && window->flags().testFlag(Qt::FramelessWindowHint), "pet requests focus-safe frameless window");
    check(window->color() == QColor(Qt::transparent) && !window->mask().isEmpty(), "host uses transparency and interaction region");
    host.setPetVisible(true);
    check(host.isPetVisible(), "show restores pet window");
    // Qt::Tool maps to a macOS panel that hides when the application deactivates.
    check((window->flags() & Qt::WindowType_Mask) != Qt::Tool, "resident must not request an auto-hiding tool panel");
    QEvent inactive(QEvent::WindowDeactivate); QApplication::sendEvent(window, &inactive);
    check(host.isPetVisible(), "losing focus preserves pet visibility");
    const auto position = host.position();
    const auto mask = window->mask();
    host.applyPose(evaluatePose(rig, 44, "idle-motion"));
    check(host.position() == position && window->mask() == mask && host.isPetVisible(), "pose does not move window or interaction mask");
    host.setPetVisible(false);
    host.applyPose(evaluatePose(rig, 60, "idle-motion"));
    check(!host.isPetVisible(), "hidden pose application does not show window");
    host.setPetVisible(true);
    check(host.isPetVisible(), "explicit show after hidden pose works");
    int starts = 0, ends = 0, placements = 0, menus = 0;
    host.draggingChanged = [&](bool dragging) { dragging ? ++starts : ++ends; };
    host.placementChanged = [&] { ++placements; };
    host.contextMenuRequested = [&](QPoint point) { check(point == position + QPoint(120, 120), "menu gets global pointer position"); ++menus; };
    const QPoint hit(120, 120);
    mouse(window, QEvent::MouseButtonPress, hit, position + hit, Qt::LeftButton, Qt::LeftButton);
    mouse(window, QEvent::MouseMove, hit + QPoint(40, 40), position + hit + QPoint(40, 40), Qt::NoButton, Qt::LeftButton);
    check(starts == 1 && host.position() == position + QPoint(40, 40) && placements > 0, "drag reports state and realized placement");
    mouse(window, QEvent::MouseButtonRelease, hit, host.position() + hit, Qt::LeftButton, Qt::NoButton);
    check(ends == 1, "release ends drag once");
    host.restorePlacement(position, host.displayName());
    mouse(window, QEvent::MouseButtonPress, hit, position + hit, Qt::RightButton, Qt::RightButton);
    check(menus == 1 && host.isPetVisible(), "right click requests context menu without closing pet");
    mouse(window, QEvent::MouseButtonPress, hit, position + hit, Qt::LeftButton, Qt::LeftButton);
    host.setPetVisible(false);
    check(ends == 2, "hide cancels active drag");
    host.setPetVisible(true);
    mouse(window, QEvent::MouseButtonPress, hit, position + hit, Qt::LeftButton, Qt::LeftButton);
    QEvent ungrab(QEvent::UngrabMouse); QApplication::sendEvent(window, &ungrab);
    check(ends == 3, "lost grab cancels active drag");
    mouse(window, QEvent::MouseButtonPress, hit, position + hit, Qt::LeftButton, Qt::LeftButton);
    host.cancelDrag(); host.cancelDrag();
    check(ends == 4, "suspend boundary cancels drag idempotently");
    const auto stopped = host.position();
    mouse(window, QEvent::MouseMove, hit, stopped + hit + QPoint(50, 50), Qt::NoButton, Qt::LeftButton);
    check(host.position() == stopped, "canceled drag ignores stale move");
    host.reconcileScreens();
    check(host.position() == stopped && host.displayName() == QGuiApplication::primaryScreen()->name(), "reconcile preserves reachable placement and display identity");
    host.draggingChanged = {}; host.contextMenuRequested = {}; host.placementChanged = {};
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    int failures = 0;
    const auto run = [&](const char* name, const std::function<void()>& test) {
        try { test(); std::cout << "PASS " << name << '\n'; }
        catch (const std::exception& error) { ++failures; std::cerr << "FAIL " << name << ": " << error.what() << '\n'; }
    };
    run("grounded restoration floor repair", [&] { restoreGroundedFloor(QString::fromLocal8Bit(argv[1])); });
    run("prepared presentation ownership and ground", [&] { preparedPresentation(QString::fromLocal8Bit(argv[1])); });
    run("production silhouette preservation", [&] { check(argc == 2, "asset argument required"); productionSilhouettes(QString::fromLocal8Bit(argv[1])); });
    run("visual silhouette and stable grab region", visualSilhouetteIsNotTheGrabPolygon);
    run("display placement", geometry);
    run("adjoining display continuity", adjoiningDisplays);
    run("authored interaction reachability", interactionReachability);
    run("unequal and gapped displays", unequalAndGappedDisplays);
    run("invalid settings", malformedSettings);
    run("atomic preferences", settingsRoundTrip);
    run("production character preferences", characterPreferences);
    run("host lifecycle and pointer delivery", [&] { check(argc == 2, "asset argument required"); hostLifecycle(QString::fromLocal8Bit(argv[1])); });
    run("reachable release and restoration", [&] { check(argc == 2, "asset argument required"); reachableRelease(QString::fromLocal8Bit(argv[1])); });
    run("production drop placement and cancellation", [&] { check(argc == 2, "asset argument required"); productionDropPlacement(QString::fromLocal8Bit(argv[1])); });
    return failures == 0 ? 0 : 1;
}
