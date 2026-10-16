#pragma once
#include <QJsonObject>
#include <QPointF>
#include <QRectF>
#include <QStringList>
#include <QVector>

struct Part {
    QString id;
    int parent = -1;
    QRectF rect;
    QPointF position, pivot, size;
    double rotation = 0;
};
struct Rig {
    QString atlas;
    QPointF origin;
    QVector<QPointF> hitPolygon;
    QVector<Part> parts;
    QJsonObject motion;
};
struct Transform { double x = 0, y = 0, rotation = 0, scaleX = 1, scaleY = 1; };
struct Pose {
    QPointF root;
    QVector<Transform> parts;
    QJsonObject json(const Rig &rig) const;
};
QStringList scenarios();
QString phaseAt(double seconds, const QString &scenario);
Rig loadRig(const QString &directory);
enum class MotionPhase { Static, Idle, Walk };
class Motion {
public:
    explicit Motion(const Rig &rig);
    void step(const QString &phase);
    void step(MotionPhase phase);
    const Pose &pose() const { return pose_; }
private:
    const Rig &rig_;
    Pose pose_;
    qint64 active_ = 0, walking_ = 0;
    double distance_ = 0, tail_ = 0, velocity_ = 0;
    double rootAmplitude_, rootPeriod_, stride_, stiffness_, damping_, angleLimit_, velocityLimit_;
};
Pose evaluatePose(const Rig &rig, qint64 tick, const QString &scenario);
QJsonObject readJson(const QString &path);
