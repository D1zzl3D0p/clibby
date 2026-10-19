#pragma once
#include <QDialog>
#include <QJsonObject>
#include <QVector>
#include <functional>

class QCheckBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QSpinBox;
class QTextEdit;

namespace pets::productivity {
class Panel : public QDialog {
public:
    explicit Panel(std::function<QJsonObject(const QString &, const QJsonObject &)> call);
    void refresh();
    void setError(const QString &message);
private:
    std::function<QJsonObject(const QString &, const QJsonObject &)> call_;
    QLineEdit *search_, *taskTitle_, *timerLabel_;
    QCheckBox *attention_;
    QListWidget *inbox_, *timers_;
    QTextEdit *detail_;
    QLabel *error_;
    QSpinBox *seconds_, *breakSeconds_, *longBreakSeconds_, *rounds_, *snoozeSeconds_;
    QPushButton *open_, *acknowledge_, *snooze_, *complete_;
    QVector<QJsonObject> items_;
    void perform(const std::function<void()> &action);
    void updateSelection();
    void act(const QString &action);
    void timerAct(const QString &action);
    void createTimer(const QString &mode);
    void openSource();
};
}
