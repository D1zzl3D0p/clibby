#pragma once
#include "characters3d/pose.h"
#include "characters3d/signature.h"
#include "behavior/curiosity.h"
#include <QObject>
#include <QQuaternion>
#include <QRegion>
#include <QQuickItem>
#include <QQmlEngine>
#include <QVector3D>
#include <map>
#include <memory>
#include <vector>
#include <functional>
class WolfMotion : public QObject {
 Q_OBJECT
 Q_PROPERTY(double eyeOpenness READ eyeOpenness WRITE setEyeOpenness NOTIFY changed)
 Q_PROPERTY(bool autonomous READ autonomous WRITE setAutonomous NOTIFY changed)
 Q_PROPERTY(quint32 seed READ seed WRITE setSeed NOTIFY changed)
 Q_PROPERTY(double walkingSpeed READ walkingSpeed WRITE setWalkingSpeed NOTIFY changed)
 Q_PROPERTY(double elevation READ elevation NOTIFY changed)
 Q_PROPERTY(double rootOffset READ rootOffset NOTIFY changed)
 Q_PROPERTY(double targetOffset READ targetOffset NOTIFY changed)
 Q_PROPERTY(QVector3D leftContact READ leftContact NOTIFY changed)
 Q_PROPERTY(QVector3D rightContact READ rightContact NOTIFY changed)
 Q_PROPERTY(bool leftSwing READ leftSwing NOTIFY changed)
 Q_PROPERTY(bool rightSwing READ rightSwing NOTIFY changed)
 Q_PROPERTY(double yaw READ yaw NOTIFY changed)
 Q_PROPERTY(QVector3D headAnchor READ headAnchor NOTIFY anchorsChanged)
 Q_PROPERTY(QVector3D cardAnchor READ cardAnchor NOTIFY anchorsChanged)
 Q_PROPERTY(QVector3D effectAnchor READ effectAnchor NOTIFY anchorsChanged)
 Q_PROPERTY(QVector3D leftGrip READ leftGrip NOTIFY anchorsChanged)
 Q_PROPERTY(QVector3D rightGrip READ rightGrip NOTIFY anchorsChanged)
 Q_PROPERTY(QString status READ status NOTIFY changed)
 Q_PROPERTY(bool reachSupported READ reachSupported CONSTANT)
 Q_PROPERTY(bool signatureSupported READ signatureSupported CONSTANT)
 Q_PROPERTY(QString signatureName READ signatureName CONSTANT)
public:
 explicit WolfMotion(const QString& rig,QObject* parent=nullptr,std::function<double()> clock={});
 double behaviorTime() const { return behaviorClock_(); }
 double walkingSpeed() const { return motion_->speedMultiplier(); }
 void setWalkingSpeed(double);
 void bind(QObject* scene);
 void advance(double dt);
 void advancePresentation(double dt, bool render=true);
 bool autonomous() const { return autonomous_; }
 void setAutonomous(bool);
 quint32 seed() const { return seed_; }
 void setSeed(quint32);
 std::optional<double> nextBehaviorDeadline() const;
 void advanceBehavior(double now);
 Q_INVOKABLE void look(QVector3D direction);
 Q_INVOKABLE void clearExpression();
 Q_INVOKABLE void roleGoal(const QString& role, QVector3D radians);
 pets::character3d::Motion& motion() { return *motion_; }
 const pets::character3d::Motion& motion() const { return *motion_; }
 void setReaction(const QString& semantic, double normalizedProgress);
 QPointF projectAnchor(QQuickItem* stage, QVector3D point) const;
 QRegion projectedVisualRegion(QQuickItem* stage) const;
 QRegion projectedGrabRegion(QQuickItem* stage) const;
 double elevation() const { return motion_->state().elevation/100.; }
 double rootOffset() const { return motion_->state().rootX/100.; }
 double targetOffset() const { return motion_->state().targetX/100.; }
 QVector3D contact(int i) const { auto p=motion_->pose().contacts[i]; return QVector3D(p.x,p.y,p.z); }
 QVector3D leftContact() const { return contact(0); }
 QVector3D rightContact() const { return contact(1); }
 bool leftSwing() const { return motion_->pose().swing[0]; }
 bool rightSwing() const { return motion_->pose().swing[1]; }
 double yaw() const { return motion_->pose().yaw*180/3.141592653589793; }
 QVector3D headAnchor() const;
 QVector3D cardAnchor() const;
 QVector3D effectAnchor() const;
 QVector3D grip(int side) const;
 QVector3D leftGrip() const { return grip(0); }
 QVector3D rightGrip() const { return grip(1); }
 QString status() const;
 Q_INVOKABLE void reach(int side, QVector3D modelGripTarget);
 Q_INVOKABLE void release(int side);
 Q_INVOKABLE void gesture(const QString &semantic);
 bool signatureSupported() const { return signatureBound_; }
 QString signatureName() const { return signatureBound_ ? QString::fromUtf8(signature_.name().data(),int(signature_.name().size())) : QString(); }
 Q_INVOKABLE bool signature();
 void cancelSignature(bool immediate=false);
 bool signatureActive() const { return signature_.active(); }

 Q_INVOKABLE void target(double x);
 Q_INVOKABLE void stop();
 Q_INVOKABLE bool hop(double height);
 Q_INVOKABLE bool flyTo(double x,double elevation);
 Q_INVOKABLE void land();
 Q_INVOKABLE void impulse();
 Q_INVOKABLE void pause(bool);
 bool paused() const { return paused_; }
 bool settled() const { return motion_->settled() && gestureTime_ >= 1 && !signature_.active(); }
 int jointCount() const { return joints_.size(); }
 bool verifyDeformation();
 void apply();
 bool eyeBlinkSupported() const { return joints_.count("eye_left") && joints_.count("eye_right"); }
 bool reachSupported() const { return hasArms_; }
 double eyeOpenness() const { return eyeOpenness_; }
 void setEyeOpenness(double);
signals: void changed(); void anchorsChanged();
private:
 struct Joint { QObject* node=nullptr; QString parent; QVector3D restPosition,restWorldPosition,restScale{1,1,1};QQuaternion restRotation,restWorldRotation; };
 std::map<QString,Joint> joints_;
 struct SkinBound { QString joint; QVector3D minimum,maximum; };
 std::vector<std::vector<SkinBound>> skinBounds_;
 pets::character::LimbArray<pets::character3d::Leg> legs_;
 std::vector<QString> legRoles_;
 pets::character::Gait gait_=pets::character::Gait::Biped;
 std::array<pets::character3d::Arm,2> arms_;
 bool wings_=false,hasArms_=false,catalogueStyle_=false;
 std::uint32_t expressionRoles_=0;
 std::optional<pets::behavior::Personality> personality_;
 double eyeOpenness_=1;
 QString skinName_="WerewolfSkin";
 QString gesture_;
 pets::character3d::SignatureMotion signature_{""};
 bool signatureBound_=false;
 double gestureTime_=1;

 std::unique_ptr<pets::character3d::Motion> motion_;
 bool externalReaction_=false;
 bool autonomous_=false;
 quint32 seed_=1;
 double behaviorNow_=0;
 std::function<double()> behaviorClock_;
 pets::behavior::Curiosity curiosity_{1};
 int reactionPhase_=-1;
 void submitReaction();
 void manualInput();
 QObject* avatar_=nullptr;
 bool paused_=false;
 void wake();
 void aim(const QString&,QVector3D from,QVector3D to);
 void place(const QString&,QVector3D modelPosition);
};
