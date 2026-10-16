#pragma once
#include "sessions.h"
#include <QDialog>
#include <QWidget>
#include <functional>
class QAction;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QVBoxLayout;
namespace pets {
enum class UtilityAction { OpenSource, Acknowledge };
using UtilityDispatch = std::function<QString(SessionKey, std::uint64_t, UtilityAction)>;
// One accessible card shared by the attached notification and summoned inbox.
class UtilityCard : public QWidget {
public:
    explicit UtilityCard(QWidget* parent = nullptr, Qt::WindowFlags flags = {});
    void setRecord(const std::optional<SessionRecord>& record);
    QString text() const;
    const std::optional<SessionRecord>& record() const { return record_; }
    void setFeedback(const QString& message);
    UtilityDispatch dispatch;
private:
    QLabel* label_;
    QLabel* feedback_;
    QPushButton* open_;
    QPushButton* acknowledge_;
    std::optional<SessionRecord> record_;
};
class StatusLabel final : public UtilityCard {
public:
    StatusLabel();
    void placeBeside(QPoint petPosition, QPoint headAnchor = QPoint(128, 80));
protected:
    void showEvent(QShowEvent* event) override;
};
class UtilityToolbox final : public QDialog {
public:
    UtilityToolbox();
    void setRecords(const std::vector<SessionRecord>& records);
    void addControl(QAction* action, const QString& name);
    void setWalkingSpeed(double value, bool enabled);
    std::function<void(double)> walkingSpeedChanged;
    UtilityDispatch dispatch;
private:
    QDoubleSpinBox* walkingSpeed_;
    QVBoxLayout* rows_;
    QVBoxLayout* controls_;
    std::vector<SessionRecord> records_;
};
}
