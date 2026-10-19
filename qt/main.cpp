#include "motion.h"
#include "petitem.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMouseEvent>
#include <QMutex>
#include <QMutexLocker>
#include <QPolygon>
#include <QQuickWindow>
#include <QSaveFile>
#include <QScreen>
#include <QSGRendererInterface>
#include <QSurfaceFormat>
#include <QTimer>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void require(bool ok, const QString &message) {
    if (!ok) throw std::runtime_error(message.toStdString());
}
void writeJson(const QString &path, const QJsonObject &object) {
    QSaveFile file(path);
    require(file.open(QIODevice::WriteOnly), "cannot write " + path + ": " + file.errorString());
    const auto data = QJsonDocument(object).toJson();
    require(file.write(data) == data.size() && file.commit(), "cannot commit " + path);
}
struct Options {
    QString scenario, assets, output, dump, screenshot, ready;
    double duration = 0, warmup = 0;
};
Options parse(int argc, char **argv) {
    QMap<QString, QString> values;
    const QStringList allowed{"scenario", "assets", "output", "duration", "warmup", "dump-poses", "screenshot", "ready-file"};
    for (int i = 1; i < argc; ++i) {
        const QString arg = QString::fromLocal8Bit(argv[i]);
        require(arg.startsWith("--") && allowed.contains(arg.mid(2)) && i + 1 < argc, "unknown or incomplete argument " + arg);
        require(!values.contains(arg.mid(2)), "duplicate argument " + arg);
        values[arg.mid(2)] = QString::fromLocal8Bit(argv[++i]);
    }
    Options o;
    o.scenario = values["scenario"]; o.assets = values["assets"]; o.output = values["output"];
    o.dump = values["dump-poses"]; o.screenshot = values["screenshot"];
    o.ready = values["ready-file"];
    bool valid = false;
    o.duration = values["duration"].toDouble(&valid);
    require(valid && std::isfinite(o.duration) && o.duration > 0 && o.duration <= 86400, "duration must be finite, positive, at most 86400 seconds");
    if (values.contains("warmup")) {
        o.warmup = values["warmup"].toDouble(&valid);
        require(valid && std::isfinite(o.warmup) && o.warmup >= 0 && o.warmup < o.duration, "warmup must be finite, nonnegative and shorter than duration");
    }
    phaseAt(0, o.scenario);
    require(!o.output.isEmpty(), "output path is required");
    require(o.dump.isEmpty() || o.screenshot.isEmpty(), "dump-poses and screenshot are separate untimed modes");
    require(o.screenshot.isEmpty() || o.scenario != "hidden", "cannot screenshot hidden scenario");
    // Detect unwritable output before starting a timed run without replacing existing data.
    QSaveFile probe(o.output);
    require(probe.open(QIODevice::WriteOnly), "cannot open output " + o.output + ": " + probe.errorString());
    probe.cancelWriting();
    return o;
}
QString apiName(QSGRendererInterface::GraphicsApi api) {
    switch (api) {
    case QSGRendererInterface::Metal: return "Metal";
    case QSGRendererInterface::OpenGL: return "OpenGL";
    case QSGRendererInterface::Vulkan: return "Vulkan";
    case QSGRendererInterface::Direct3D11: return "Direct3D11";
    case QSGRendererInterface::Software: return "Software";
    case QSGRendererInterface::Null: return "Null";
    default: return "Unknown";
    }
}
double monotonicSeconds() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
struct RenderStats {
    QMutex mutex;
    QString backend;
    qint64 afterRendering = 0, frameEnd = 0, swapped = 0;
    double lastSwap = 0;
    QVector<double> intervals;
    void reset() {
        QMutexLocker guard(&mutex);
        afterRendering = frameEnd = swapped = 0; lastSwap = 0; intervals.clear();
    }
};
class PetWindow final : public QQuickWindow {
public:
    std::function<void(QPoint)> dragged;
protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::RightButton) { close(); return; }
        if (event->button() == Qt::LeftButton) { offset_ = event->globalPosition().toPoint()-position(); dragging_ = true; event->accept(); }
    }
    void mouseMoveEvent(QMouseEvent *event) override {
        if (dragging_ && dragged) dragged(event->globalPosition().toPoint()-offset_);
    }
    void mouseReleaseEvent(QMouseEvent *) override { dragging_ = false; }
private:
    QPoint offset_;
    bool dragging_ = false;
};
// Number of active intervals in [start,end); no polling through suspended intervals.
qint64 activeIntervals(qint64 start, qint64 end, const QString &scenario) {
    if (scenario == "static" || scenario == "hidden") return 0;
    if (scenario != "mixed-rest") return std::max<qint64>(0, end-start);
    const auto total = [](qint64 tick) { return (tick/3600)*1200 + std::max<qint64>(0, tick%3600-2400); };
    return total(end)-total(start);
}
}

int main(int argc, char **argv) {
    try {
        if (argc == 2 && QString::fromLocal8Bit(argv[1]) == "--help") {
            std::cout << "pet_qt --scenario NAME --duration SECONDS --warmup SECONDS --output PATH --assets ABS_DIR [--ready-file PATH] [--dump-poses PATH | --screenshot PATH]\n"
                         "Duration includes warmup. Drag pet with left button; right click quits. Dump is headless and untimed.\n";
            return 0;
        }
        const auto options = parse(argc, argv);
        const auto rig = loadRig(options.assets);
        if (!options.dump.isEmpty()) {
            QCoreApplication app(argc, argv);
            QJsonArray poses;
            const auto entries = readJson(options.assets+"/golden.json")["poses"].toArray();
            require(!entries.isEmpty(), "empty golden fixture");
            for (const auto &entry : entries) {
                const auto object = entry.toObject();
                const auto scenario = object["scenario"].toString();
                const auto tick = object["tick"].toInteger(-1);
                auto pose = evaluatePose(rig, tick, scenario).json(rig);
                pose["scenario"] = scenario; pose["tick"] = tick; poses.append(pose);
            }
            writeJson(options.dump, {{"version", 1}, {"poses", poses}});
            return 0;
        }
        QQuickWindow::setDefaultAlphaBuffer(true);
#ifdef Q_OS_MACOS
        if (qEnvironmentVariable("QT_QUICK_BACKEND") != "software")
            QQuickWindow::setGraphicsApi(QSGRendererInterface::Metal);
#endif
        QSurfaceFormat format;
        format.setAlphaBufferSize(8); format.setSamples(0); format.setSwapInterval(1);
        QSurfaceFormat::setDefaultFormat(format);
        QGuiApplication app(argc, argv);
        app.setApplicationName("Clibby Qt");
        QImage atlas(rig.atlas);
        require(!atlas.isNull(), "atlas is not a readable image");
        atlas = atlas.convertToFormat(QImage::Format_RGBA8888_Premultiplied);
        // Outlive the window: render-thread callbacks can run through its teardown.
        RenderStats rendering;
        QString error;
        PetWindow window;
        window.setTitle("Clibby Qt");
        window.setFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
        window.setColor(Qt::transparent); window.resize(256, 256);
        window.setMinimumSize(QSize(256, 256)); window.setMaximumSize(QSize(256, 256));
        const bool desktop = options.scenario == "walk-desktop" || options.scenario.startsWith("mixed-");
        const bool inside = options.scenario == "walk-fixed";
        PetItem item(rig, atlas, inside, window.contentItem());
        auto motion = std::make_unique<Motion>(rig);
        QRect screen = window.screen()->availableGeometry();
        QPoint anchor(screen.center().x()-128, screen.bottom()-256-48);
        qint64 positionWrites = 0, clipped = 0, poseEvaluations = 0, schedulerCallbacks = 0, dropped = 0, overruns = 0;
        auto place = [&](QPoint target) {
            const QPoint bounded(std::clamp(target.x(), screen.left(), std::max(screen.left(), screen.right()-255)),
                                 std::clamp(target.y(), screen.top(), std::max(screen.top(), screen.bottom()-255)));
            if (bounded != target) ++clipped;
            if (window.position() != bounded) { window.setPosition(bounded); ++positionWrites; }
        };
        place(anchor);
        window.dragged = [&](QPoint position) { anchor = position-(desktop ? motion->pose().root.toPoint() : QPoint()); place(position); };
        auto applyPose = [&]() {
            item.setPose(motion->pose());
            if (desktop) place(anchor+motion->pose().root.toPoint());
            QPolygon polygon;
            for (const auto &point : rig.hitPolygon) polygon.append((point+(inside ? motion->pose().root : QPointF())).toPoint());
            const QRegion mask(polygon);
            if (window.mask() != mask) window.setMask(mask);
        };
        applyPose();
        QObject::connect(&window, &QQuickWindow::sceneGraphInitialized, &window, [&]() {
            QMutexLocker guard(&rendering.mutex); rendering.backend = apiName(window.rendererInterface()->graphicsApi());
        }, Qt::DirectConnection);
        QObject::connect(&window, &QQuickWindow::afterRendering, &window, [&]() {
            QMutexLocker guard(&rendering.mutex); ++rendering.afterRendering;
        }, Qt::DirectConnection);
        QObject::connect(&window, &QQuickWindow::afterFrameEnd, &window, [&]() {
            QMutexLocker guard(&rendering.mutex); ++rendering.frameEnd;
        }, Qt::DirectConnection);
        QObject::connect(&window, &QQuickWindow::frameSwapped, &window, [&]() {
            const double now = monotonicSeconds(); QMutexLocker guard(&rendering.mutex);
            if (rendering.lastSwap != 0) rendering.intervals.append(now-rendering.lastSwap);
            rendering.lastSwap = now; ++rendering.swapped;
        }, Qt::DirectConnection);
        QObject::connect(&window, &QQuickWindow::sceneGraphError, &app, [&](QQuickWindow::SceneGraphError, const QString &message) { error = message; app.exit(2); });
        QElapsedTimer lifetime, phaseClock;
        QTimer scheduler; scheduler.setSingleShot(true); scheduler.setTimerType(Qt::PreciseTimer);
        qint64 cursor = 0;
        bool measurement = options.warmup == 0;
        double measurementStarted = 0;
        const auto startPose = motion->pose().json(rig);
        std::function<void()> schedule;
        schedule = [&]() {
            const double seconds = phaseClock.nsecsElapsed()/1e9;
            const auto phase = phaseAt(seconds, options.scenario);
            if (phase == "hidden" || phase == "static") {
                scheduler.stop();
                if (options.scenario == "mixed-rest") {
                    const double deadline = (std::floor(seconds/60)*60+40);
                    scheduler.start(std::max(1, static_cast<int>(std::ceil((deadline-seconds)*1000))));
                }
                return;
            }
            const double deadline = (std::floor(seconds*60)+1)/60;
            scheduler.start(std::max(1, static_cast<int>(std::ceil((deadline-seconds)*1000))));
        };
        QObject::connect(&scheduler, &QTimer::timeout, &app, [&]() {
            ++schedulerCallbacks;
            const qint64 due = static_cast<qint64>(phaseClock.nsecsElapsed()*60/1000000000LL);
            const qint64 begin = std::max(cursor, due-4);
            const qint64 lost = activeIntervals(cursor, begin, options.scenario);
            if (lost > 0) { dropped += lost; ++overruns; }
            bool changed = false;
            for (qint64 tick = begin; tick < due; ++tick) {
                const auto phase = phaseAt(static_cast<double>(tick)/60, options.scenario);
                if (phase != "static" && phase != "hidden") { motion->step(phase); ++poseEvaluations; changed = true; }
            }
            cursor = due;
            if (changed) applyPose();
            schedule();
        });
        QTimer warmup;
        warmup.setSingleShot(true); warmup.setTimerType(Qt::PreciseTimer);
        if (options.warmup > 0) {
            QObject::connect(&warmup, &QTimer::timeout, &app, [&]() {
                scheduler.stop();
                const auto oldPose = motion->pose().json(rig);
                motion = std::make_unique<Motion>(rig); cursor = 0;
                if (oldPose != motion->pose().json(rig)) applyPose();
                poseEvaluations = schedulerCallbacks = dropped = overruns = positionWrites = clipped = 0;
                rendering.reset(); phaseClock.restart(); measurement = true;
                measurementStarted = lifetime.nsecsElapsed()/1e9; schedule();
            });
        }
        QSize screenshotSize;
        if (!options.screenshot.isEmpty()) QTimer::singleShot(std::max(1, static_cast<int>(options.duration*500)), &app, [&]() {
            const auto image = window.grabWindow(); screenshotSize = image.size();
            QSaveFile output(options.screenshot);
            if (image.isNull() || !output.open(QIODevice::WriteOnly) || !image.save(&output, "PNG") || !output.commit()) {
                error = "failed screenshot capture/write"; app.exit(2);
            }
        });
        QTimer finish; finish.setSingleShot(true); finish.setTimerType(Qt::PreciseTimer);
        QObject::connect(&finish, &QTimer::timeout, &app, &QCoreApplication::quit);
        if (options.scenario != "hidden") window.show();
        if (!options.ready.isEmpty()) writeJson(options.ready, {{"ready", true}, {"pid", QCoreApplication::applicationPid()}});
        lifetime.start(); phaseClock.start();
        if (options.warmup > 0) warmup.start(static_cast<int>(std::ceil(options.warmup*1000)));
        finish.start(static_cast<int>(std::ceil(options.duration*1000)));
        schedule();
        const int status = app.exec();
        scheduler.stop(); warmup.stop(); finish.stop();
        const double elapsed = lifetime.nsecsElapsed()/1e9;
        const double dpr = window.effectiveDevicePixelRatio();
        const QJsonArray logicalSize{window.width(), window.height()};
        QJsonObject output{{"schema_version", 1}, {"engine", "Qt"}, {"engine_version", qVersion()}, {"build_type", PET_BUILD_TYPE},
            {"scenario", options.scenario}, {"duration_seconds", options.duration}, {"warmup_seconds", options.warmup},
            {"elapsed_seconds", elapsed}, {"measurement_elapsed_seconds", measurement ? elapsed-measurementStarted : 0},
            {"measurement_clock_reset", measurement}, {"measurement_start_pose", startPose},
            {"pose_evaluations", poseEvaluations}, {"scheduler_callbacks", schedulerCallbacks}, {"simulation_overruns", overruns}, {"dropped_steps", dropped},
            {"window_position_writes", positionWrites}, {"window_position_clipped", clipped}, {"logical_window_size", logicalSize},
            {"device_pixel_ratio", dpr}, {"framebuffer_size_derived", QJsonArray{qRound(window.width()*dpr), qRound(window.height()*dpr)}},
            {"framebuffer_size", screenshotSize.isEmpty() ? QJsonValue() : QJsonValue(QJsonArray{screenshotSize.width(), screenshotSize.height()})},
            {"framebuffer_size_reason", screenshotSize.isEmpty() ? "No readback during timed runs; derived size is window size times effectiveDevicePixelRatio" : "Measured grabWindow image size in untimed screenshot run"},
            {"platform", QGuiApplication::platformName()}, {"display_refresh_rate_reported_hz", window.screen()->refreshRate()},
            {"displayed_frames", QJsonValue()}, {"displayed_frames_reason", "Qt callback timestamps do not establish physical display presentation"},
            {"screenshot_mode", !options.screenshot.isEmpty()}, {"error", error.isEmpty() ? QJsonValue() : QJsonValue(error)},
            {"final_pose", motion->pose().json(rig)}, {"window_visible", window.isVisible()}, {"window_exposed", window.isExposed()},
            {"functional_equivalence_verified", false}};
        {
            QMutexLocker guard(&rendering.mutex);
            QJsonArray intervals; for (double interval : rendering.intervals) intervals.append(interval);
            output["backend"] = rendering.backend.isEmpty() ? QJsonValue() : QJsonValue(rendering.backend);
            output["backend_reason"] = rendering.backend.isEmpty() ? "Scene graph never initialized (expected for hidden)" : "Observed rendererInterface graphicsApi at sceneGraphInitialized";
            output["after_rendering_events"] = rendering.afterRendering;
            output["after_frame_end_events"] = rendering.frameEnd;
            output["frame_swapped_events"] = rendering.swapped;
            output["frame_event_intervals_seconds"] = intervals;
            output["frame_event_semantics"] = "QQuickWindow::frameSwapped: frame queued for presentation, not proof of display; afterRendering and afterFrameEnd are separate Qt render callbacks";
        }
        writeJson(options.output, output);
        // Disconnect direct render callbacks before captured stack state is destroyed.
        QObject::disconnect(&window, nullptr, &window, nullptr);
        return error.isEmpty() ? status : 2;
    } catch (const std::exception &error) {
        std::cerr << "error: " << error.what() << '\n'; return 2;
    }
}
