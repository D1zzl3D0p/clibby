#include "motion.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QHash>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
void require(bool ok, const QString &message) {
    if (!ok) throw std::runtime_error(message.toStdString());
}
double number(const QJsonValue &value) {
    require(value.isDouble() && std::isfinite(value.toDouble()), "rig requires finite numbers");
    return value.toDouble();
}
QPointF point(const QJsonValue &value) {
    auto a = value.toArray(); require(a.size() == 2, "rig point must have two coordinates");
    return {number(a[0]), number(a[1])};
}
constexpr double tau = 6.283185307179586476925286766559;
}
QJsonObject readJson(const QString &path) {
    QFile file(path);
    require(file.open(QIODevice::ReadOnly), "cannot read " + path + ": " + file.errorString());
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(file.readAll(), &error);
    require(error.error == QJsonParseError::NoError && document.isObject(), "invalid JSON object: " + path);
    return document.object();
}
QStringList scenarios() {
    return {"static", "idle-motion", "walk-fixed", "walk-desktop", "hidden", "mixed-active", "mixed-rest"};
}
QString phaseAt(double seconds, const QString &scenario) {
    require(std::isfinite(seconds) && seconds >= 0, "seconds must be finite and nonnegative");
    require(scenarios().contains(scenario), "unknown scenario: " + scenario);
    const double offset = std::fmod(seconds, 60.0);
    if (scenario == "mixed-active") return offset < 45 ? "idle-motion" : "walk-desktop";
    if (scenario == "mixed-rest") return offset < 40 ? "static" : offset < 55 ? "walk-desktop" : "idle-motion";
    return scenario;
}
Rig loadRig(const QString &directory) {
    require(QDir::isAbsolutePath(directory) && QDir(directory).exists(), "assets must be an existing absolute directory");
    const auto json = readJson(directory + "/rig.json");
    require(json["version"].toInt() == 1, "unsupported rig version");
    require(point(json["surface_size"]) == QPointF(256, 256), "surface must be 256 by 256");
    Rig rig;
    const auto atlas = json["atlas"].toString();
    require(!atlas.isEmpty() && QFileInfo(atlas).fileName() == atlas, "atlas must be a local filename");
    rig.atlas = directory + "/" + atlas;
    require(QFileInfo::exists(rig.atlas), "missing atlas " + rig.atlas);
    rig.origin = point(json["origin"]);
    const auto atlasSize = point(json["atlas_size"]);
    for (const auto &p : json["hit_polygon"].toArray()) rig.hitPolygon.append(point(p));
    require(rig.hitPolygon.size() >= 3, "missing hit polygon");
    QHash<QString, int> indexes;
    const QStringList expected{"tail", "rear_left", "rear_right", "body", "front_left", "front_right", "head", "ear_left", "ear_right", "eye_left", "eye_right", "muzzle"};
    for (const auto &value : json["parts"].toArray()) {
        const auto object = value.toObject();
        Part part;
        part.id = object["id"].toString();
        require(expected.contains(part.id) && !indexes.contains(part.id), "unknown or duplicate part id");
        if (!object["parent"].isNull()) {
            const auto parent = object["parent"].toString();
            require(indexes.contains(parent), "parent must precede child");
            part.parent = indexes[parent];
        }
        auto rect = object["rect"].toArray(); require(rect.size() == 4, "invalid atlas rectangle");
        part.rect = QRectF(number(rect[0]), number(rect[1]), number(rect[2]), number(rect[3]));
        require(part.rect.width() > 0 && part.rect.height() > 0 && QRectF(0, 0, atlasSize.x(), atlasSize.y()).contains(part.rect), "atlas rectangle out of bounds");
        part.position = point(object["position"]); part.pivot = point(object["pivot"]); part.size = point(object["size"]);
        require(part.size.x() > 0 && part.size.y() > 0, "invalid part size");
        part.rotation = number(object["rotation"]);
        indexes[part.id] = rig.parts.size(); rig.parts.append(part);
    }
    require(rig.parts.size() == 12, "rig requires 12 parts");
    rig.motion = json["motion"].toObject();
    for (const auto *key : {"hz", "max_catchup_steps", "root_amplitude", "root_period_ticks", "stride_distance", "tail_stiffness", "tail_damping", "tail_angle_limit", "tail_velocity_limit"})
        require(number(rig.motion[key]) > 0, "motion constants must be positive");
    require(rig.motion["hz"].toInt() == 60 && rig.motion["max_catchup_steps"].toInt() == 4, "fixture requires 60 Hz / four catch-up steps");
    return rig;
}
QJsonObject Pose::json(const Rig &rig) const {
    QJsonArray list;
    for (int i = 0; i < parts.size(); ++i) {
        const auto &p = parts[i];
        list.append(QJsonObject{{"id", rig.parts[i].id}, {"x", p.x}, {"y", p.y}, {"rotation", p.rotation}, {"scale_x", p.scaleX}, {"scale_y", p.scaleY}});
    }
    return {{"root_offset", QJsonArray{root.x(), root.y()}}, {"parts", list}};
}
Motion::Motion(const Rig &rig) : rig_(rig),
    rootAmplitude_(rig.motion["root_amplitude"].toDouble()),
    rootPeriod_(rig.motion["root_period_ticks"].toDouble()),
    stride_(rig.motion["stride_distance"].toDouble()),
    stiffness_(rig.motion["tail_stiffness"].toDouble()),
    damping_(rig.motion["tail_damping"].toDouble()),
    angleLimit_(rig.motion["tail_angle_limit"].toDouble()),
    velocityLimit_(rig.motion["tail_velocity_limit"].toDouble()) {
    for (const auto &p : rig.parts) pose_.parts.append({p.position.x(), p.position.y(), p.rotation, 1, 1});
}
void Motion::step(const QString &phase) {
    require(scenarios().contains(phase) && !phase.startsWith("mixed"), "step requires resolved phase");
    step(phase == "static" || phase == "hidden" ? MotionPhase::Static
         : phase == "walk-fixed" || phase == "walk-desktop" ? MotionPhase::Walk : MotionPhase::Idle);
}
void Motion::step(MotionPhase phase) {
    if (phase == MotionPhase::Static) return;
    ++active_;
    const bool walking = phase == MotionPhase::Walk;
    if (walking) {
        const double old = pose_.root.x(); ++walking_;
        pose_.root.setX(rootAmplitude_*std::sin(tau*walking_/rootPeriod_));
        distance_ += std::abs(pose_.root.x()-old);
    }
    const double gait = walking ? std::sin(tau*distance_/stride_) : 0;
    const double tx = 24*std::sin(tau*active_/180), ty = 8*std::sin(tau*active_/240);
    const double head = 0.5*std::atan2(ty, 48+tx), breath = std::sin(tau*active_/180);
    const double target = 0.25*std::sin(tau*active_/120)+0.1*gait;
    const double vmax = velocityLimit_, qmax = angleLimit_;
    velocity_ = std::clamp(velocity_+(stiffness_*(target-tail_)-damping_*velocity_)/60, -vmax, vmax);
    tail_ = std::clamp(tail_+velocity_/60, -qmax, qmax);
    if (std::abs(tail_) == qmax) velocity_ = 0;
    for (int i = 0; i < rig_.parts.size(); ++i) {
        const auto &base = rig_.parts[i]; auto &p = pose_.parts[i];
        p = {base.position.x(), base.position.y(), base.rotation, 1, 1};
        if (base.id == "body") { p.y -= 1.5*breath; p.scaleY = 1+0.015*breath; }
        else if (base.id == "head") p.rotation += head;
        else if (base.id == "tail") p.rotation += tail_;
        else if (base.id == "rear_left" || base.id == "front_right") p.rotation += 0.35*gait;
        else if (base.id == "rear_right" || base.id == "front_left") p.rotation -= 0.35*gait;
        else if (base.id == "eye_left" || base.id == "eye_right") { p.x += tx/24*1.5; p.y += ty/8*0.75; }
    }
}
Pose evaluatePose(const Rig &rig, qint64 tick, const QString &scenario) {
    require(tick >= 0, "tick must be nonnegative"); phaseAt(0, scenario);
    Motion state(rig);
    for (qint64 step = 0; step < tick; ++step) state.step(phaseAt(static_cast<double>(step)/60, scenario));
    return state.pose();
}
