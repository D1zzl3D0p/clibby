#include "application.h"
#include "desktop_host.h"
#include "instance.h"
#include "sleep_observer.h"
#include "reaction_motion.h"
#include "status_label.h"
#include <QLabel>
#include <QDoubleSpinBox>
#include <QFileInfo>
#include <limits>
#include <QAction>
#include <QApplication>
#include <QFile>
#include <QJsonArray>
#include <QMouseEvent>
#include <QPushButton>
#include <QToolButton>
#include <QDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QThread>
#include <QSystemTrayIcon>
#include <QScrollArea>
#include <QScrollBar>
#include <vector>
#include <iostream>
#include <functional>
#include <stdexcept>
#ifdef __APPLE__
#import <AppKit/AppKit.h>
#endif

class SourceReceiver : public QObject {
    Q_OBJECT
public:
    QUrl opened;
public slots:
    void open(const QUrl& url) { opened = url; }
};
namespace {
void check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
void fire(pets::ResidentApplication& shell) {
    auto* timer = shell.findChild<QTimer*>();
    check(timer != nullptr, "shell has actual Qt timer");
    check(QMetaObject::invokeMethod(timer, "timeout", Qt::DirectConnection), "Qt timeout delivered");
}
QQuickWindow* window() {
    for (auto* candidate : QGuiApplication::allWindows())
        if (candidate->title() == "Clibby") return dynamic_cast<QQuickWindow*>(candidate);
    return nullptr;
}
void beginDrag() {
    auto* target = window();
    check(target != nullptr, "pet window exists");
    const QPointF local(128, 160), global = target->position() + local;
    QMouseEvent event(QEvent::MouseButtonPress, local, local, global,
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(target, &event);
}
QByteArray contents(const QString& path) {
    QFile file(path);
    check(file.open(QIODevice::ReadOnly), "settings readable");
    return file.readAll();
}
std::vector<QString>* messages = nullptr;
void captureMessages(QtMsgType, const QMessageLogContext&, const QString& message) {
    if (messages) messages->push_back(message);
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    if (argc != 2) return 2;
    const QString assets = QString::fromLocal8Bit(argv[1]);
    unsigned failures = 0;
    auto test = [&](const char* name, const std::function<void()>& run) {
        try { run(); std::cout << "PASS " << name << '\n'; }
        catch (const std::exception& error) { ++failures; std::cerr << "FAIL " << name << ": " << error.what() << '\n'; }
    };
    test("appearance menu applies size and remembers independent pixelation", [&] {
        QTemporaryDir dir;
        QSize normal;
        const auto companion = QFileInfo(assets).absolutePath() + "/characters/soft-creature";
        {
            pets::ResidentApplication shell(companion, dir.path(), false, [] { return 0.; }, pets::IpcMode::DisabledForTests);
            shell.control(pets::Control::Pause);
            normal = window()->size();
            auto *large = shell.findChild<QAction*>("companionSize150");
            auto *coarse = shell.findChild<QAction*>("pixelation200");
            check(large && coarse, "size and pixelation options are available");
            large->trigger();
            check(window()->size() == normal * 1.5, "size changes actual presentation");
            check(large->isChecked(), "selected size is visible");
            coarse->trigger();
            check(coarse->isChecked(), "selected pixelation is visible");
            check(window()->size() == normal * 1.5, "pixelation does not change companion size");
            check(!shell.timerActive(), "appearance changes do not animate a paused pet");
        }
        pets::ResidentApplication restored(companion, dir.path(), false, [] { return 0.; }, pets::IpcMode::DisabledForTests);
        check(window()->size() == normal * 1.5, "size survives relaunch");
        check(restored.findChild<QAction*>("pixelation200")->isChecked(), "pixelation survives relaunch");
    });
    test("productivity inbox survives restart independently of session expiry", [&] {
        QTemporaryDir dir;
        QJsonObject event{{"source_id", "claude"}, {"event_id", "review"}, {"seq", 1},
                          {"kind", "needs_input"}, {"title", "Review needs your input"}};
        {
            pets::ResidentApplication shell(assets, dir.path(), false, [] { return 0.; }, pets::IpcMode::DisabledForTests);
            auto reply = shell.handleRequest("productivity.publish", event);
            check(reply.errorCode == 0, "productivity publish accepted");
            check(shell.handleRequest("productivity.publish", event).errorCode == -32003, "duplicate rejected");
            auto* summon = shell.findChild<QAction*>("summonProductivity");
            check(summon, "productivity panel reachable from menu");
            shell.control(pets::Control::Hide);
            summon->trigger();
            QWidget* panel = nullptr;
            for (auto* widget : QApplication::topLevelWidgets())
                if (widget->objectName() == "productivityPanel") panel = widget;
            check(panel && panel->isVisible(), "productivity remains available with pet hidden");
        }
        pets::ResidentApplication restored(assets, dir.path(), false, [] { return 0.; }, pets::IpcMode::DisabledForTests);
        const auto list = restored.handleRequest("productivity.list", {{"attention_only", true}});
        check(list.errorCode == 0 && list.result["items"].toArray().size() == 1, "attention restored after restart");
        check(restored.handleRequest("state.get", {}).result["sessions"].toArray().isEmpty(), "restore does not replay session reactions");
        const auto acknowledged = restored.handleRequest("productivity.act", {{"source_id", "claude"}, {"event_id", "review"}, {"seq", 1}, {"action", "acknowledge"}});
        check(acknowledged.errorCode == 0 && !acknowledged.result["item"].toObject()["completed"].toBool(), "seen remains distinct from complete");
        check(restored.handleRequest("productivity.list", {{"attention_only", true}}).result["items"].toArray().isEmpty(), "acknowledgment removes attention");
    });
    test("productivity timers use resident deadline while pet is hidden", [&] {
        QTemporaryDir dir;
        double now = 0;
        qint64 wall = 1000000;
        pets::ResidentApplication shell(assets, dir.path(), false, [&] { return now; },
            pets::IpcMode::DisabledForTests, nullptr, true, {}, {}, [&] { return wall; });
        shell.control(pets::Control::Hide);
        check(!shell.timerActive(), "empty hidden host has no timer");
        auto created = shell.handleRequest("productivity.timer_create", {{"label", "Tea"}, {"seconds", 2}});
        check(created.errorCode == 0, "countdown created");
        check(shell.timerActive() && shell.timerInterval() <= 2000, "existing resident timer includes countdown deadline");
        check(shell.findChildren<QTimer*>().size() == 1, "no extra application polling timer");
        now += 3; wall += 3000;
        check(shell.handleRequest("productivity.list", {}).errorCode == 0, "query reconciles due countdown");
        check(!shell.handleRequest("state.get", {}).result["foreground"].isNull(), "query preserves timer cue for the pet bridge");
        fire(shell);
        const auto list = shell.handleRequest("productivity.list", {{"attention_only", true}});
        check(list.errorCode == 0 && list.result["items"].toArray().size() == 1, "timer creates durable missed item while hidden");
        check(!shell.petVisible(), "timer does not reveal pet");
        now += 300; wall += 300000; fire(shell);
        check(shell.handleRequest("productivity.list", {}).result["items"].toArray().size() == 1, "missed timer does not duplicate");
        check(!shell.timerActive(), "finished hidden countdown returns to no scheduling");
    });
    test("productivity cancellation clears transient agent attention", [&] {
        QTemporaryDir dir;
        pets::ResidentApplication shell(assets, dir.path(), false, [] { return 0.; }, pets::IpcMode::DisabledForTests);
        QJsonObject event{{"source_id", "codex"}, {"event_id", "review"}, {"seq", 1},
                          {"kind", "needs_input"}, {"title", "Approval requested"}};
        check(shell.handleRequest("productivity.publish", event).errorCode == 0, "agent requests input");
        check(!shell.handleRequest("state.get", {}).result["foreground"].isNull(), "attention visible before interruption");
        event["seq"] = 2; event["kind"] = "cancelled"; event["title"] = "Turn interrupted";
        check(shell.handleRequest("productivity.publish", event).errorCode == 0, "interruption accepted");
        check(shell.handleRequest("state.get", {}).result["foreground"].isNull(), "cancelled request no longer claims attention");
    });
    test("every Pomodoro interval can signal through the shared pet cards", [&] {
        QTemporaryDir dir;
        double now = 0; qint64 wall = 1000000;
        pets::ResidentApplication shell(assets, dir.path(), false, [&] { return now; },
            pets::IpcMode::DisabledForTests, nullptr, true, {}, {}, [&] { return wall; });
        auto created = shell.handleRequest("productivity.timer_create", {{"label", "Review"}, {"seconds", 1},
            {"mode", "pomodoro"}, {"break_seconds", 1}});
        const QJsonValue id = created.result["timer"].toObject()["id"];
        now += 2; wall += 2000; fire(shell);
        auto foreground = shell.handleRequest("state.get", {}).result["foreground"].toObject();
        check(!foreground.isEmpty(), "work interval signals completion");
        auto* card = [&]() -> pets::StatusLabel* {
            for (auto* widget : QApplication::topLevelWidgets())
                if (auto* value = dynamic_cast<pets::StatusLabel*>(widget)) return value;
            return nullptr;
        }();
        check(card && card->findChild<QPushButton*>("acknowledgeCard"), "shared notification card available");
        card->findChild<QPushButton*>("acknowledgeCard")->click();
        check(shell.handleRequest("productivity.list", {{"attention_only", true}}).result["items"].toArray().isEmpty(), "card acknowledges persistent timer item");
        check(shell.handleRequest("productivity.timer_act", {{"id", id}, {"action", "next"}}).errorCode == 0, "break starts");
        now += 10; wall += 10000; fire(shell);
        foreground = shell.handleRequest("state.get", {}).result["foreground"].toObject();
        check(!foreground.isEmpty() && !foreground["acknowledged"].toBool(), "break completion signals after acknowledged work completion");
    });
    test("distinct approval occurrences rearm both inbox and pet attention", [&] {
        QTemporaryDir dir;
        pets::ResidentApplication shell(assets, dir.path(), false, [] { return 0.; }, pets::IpcMode::DisabledForTests);
        QJsonObject event{{"source_id", "codex"}, {"event_id", "review"}, {"seq", 1},
            {"kind", "needs_input"}, {"title", "Approval requested"}, {"occurrence_id", "request-1"}};
        check(shell.handleRequest("productivity.publish", event).errorCode == 0, "first approval accepted");
        shell.handleRequest("productivity.act", {{"source_id", "codex"}, {"event_id", "review"}, {"seq", 1}, {"action", "acknowledge"}});
        check(shell.handleRequest("state.get", {}).result["foreground"].isNull(), "first approval seen");
        event["seq"] = 2;
        shell.handleRequest("productivity.publish", event);
        check(shell.handleRequest("state.get", {}).result["foreground"].isNull(), "same occurrence remains seen");
        event["seq"] = 3; event["occurrence_id"] = "request-3";
        shell.handleRequest("productivity.publish", event);
        check(!shell.handleRequest("state.get", {}).result["foreground"].isNull(), "new approval reopens pet attention");
    });
    test("walking speed control persists without resetting locomotion or waking rest", [&] {
        QTemporaryDir dir;
        const QString creature = QFileInfo(assets).absolutePath() + "/characters/soft-creature";
        double now = 0;
        {
            pets::ResidentApplication shell(creature, dir.path(), false, [&] { return now; }, pets::IpcMode::DisabledForTests);
            check(shell.walkingSpeed() == 2.0, "new resident uses faster default");
            pets::UtilityToolbox* toolbox = nullptr;
            for (auto* widget : QApplication::topLevelWidgets())
                if (widget->objectName() == "utilityToolbox") toolbox = dynamic_cast<pets::UtilityToolbox*>(widget);
            auto* speed = toolbox ? toolbox->findChild<QDoubleSpinBox*>("walkingSpeed") : nullptr;
            check(speed && speed->isEnabled() && speed->minimum() == .5 && speed->maximum() == 3.0, "toolbox offers supported walking speed range");
            toolbox->show();
            auto* scroll = toolbox->findChild<QScrollArea*>();
            check(scroll, "toolbox exposes its scrolling utility content");
            for (const int width : {420, 440, 640}) {
                toolbox->resize(width, 700); QApplication::processEvents(); QApplication::processEvents();
                if (scroll->horizontalScrollBar()->maximum() != 0)
                    throw std::runtime_error(QString("toolbox overflows at %1px: viewport %2px, content %3px, minimum hint %4px")
                        .arg(width).arg(scroll->viewport()->width()).arg(scroll->widget()->width()).arg(scroll->widget()->minimumSizeHint().width()).toStdString());
                for (auto* button : toolbox->findChildren<QToolButton*>()) {
                    const auto position = button->mapTo(scroll->viewport(), QPoint());
                    check(position.x() >= 0 && position.x() + button->width() <= scroll->viewport()->width(), "toolbox controls fit within viewport horizontally");
                    check(button->width() >= button->sizeHint().width(), "toolbox control labels retain their full readable width");
                }
            }
            toolbox->resize(440, 700); toolbox->hide();
            if (const auto capture = qEnvironmentVariable("CLIBBY_UTILITY_CAPTURE"); !capture.isEmpty()) {
                check(QDir().mkpath(capture), "speed capture directory created");
                toolbox->show(); QApplication::processEvents();
                check(toolbox->grab().save(capture + "/walking-speed.png"), "walking speed control captured");
                toolbox->hide();
            }
            check(!shell.groundMotionActive(), "creature starts resting");
            shell.control(pets::Control::Pause);
            speed->setValue(2.75);
            check(shell.walkingSpeed() == 2.75 && pets::loadPreferences(dir.filePath("settings.json")).walkingSpeed == 2.75, "toolbox change reaches persistent preference");
            check(!shell.groundMotionActive() && !shell.timerActive(), "speed adjustment at rest does not start animation timer");
            shell.control(pets::Control::Resume);
            check(shell.timerInterval() >= 100 && shell.timerInterval() <= 500 && !shell.groundMotionActive(), "speed change retains a sparse quiet future deadline");
            const auto before = *shell.groundedState();
            shell.setGroundTarget(before.rootX + 80);
            now += 1.0/60; fire(shell);
            const auto moving = *shell.groundedState();
            speed->setValue(1.25);
            const auto after = *shell.groundedState();
            check(after.rootX == moving.rootX && after.targetX == moving.targetX && after.velocity == moving.velocity,
                  "live adjustment preserves current navigation");
            for (std::size_t i = 0; i < after.feet.size(); ++i)
                check(after.feet[i].position.x == moving.feet[i].position.x && after.feet[i].position.y == moving.feet[i].position.y,
                      "live adjustment preserves foot contacts");
            check(!shell.setWalkingSpeed(.4) && !shell.setWalkingSpeed(3.1) && !shell.setWalkingSpeed(std::numeric_limits<double>::quiet_NaN()), "invalid speed changes rejected");
            check(shell.walkingSpeed() == 1.25, "invalid speed does not alter saved preference");
            QString error;
            check(shell.selectCharacter(QFileInfo(assets).absolutePath() + "/characters/werewolf", &error), "character can switch with speed preference");
            check(shell.walkingSpeed() == 1.25 && speed->value() == 1.25, "character switch retains speed");
        }
        pets::ResidentApplication restored(creature, dir.path(), false, [] { return 0.0; }, pets::IpcMode::DisabledForTests);
        check(restored.walkingSpeed() == 1.25, "walking speed survives resident restart");
    });
    test("shared card reports failed launch on the originating surface", [&] {
        pets::UtilityCard card;
        pets::SessionRecord record;
        record.key = {"build", "one"}; record.label = "Build"; record.sequence = 17;
        record.status = pets::SessionStatus::Completed; record.acknowledged = true; record.sourceUrl = "https://example.com/build";
        card.setRecord(record);
        card.dispatch = [](pets::SessionKey key, std::uint64_t seq, pets::UtilityAction action) {
            check(key.sourceId == "build" && seq == 17 && action == pets::UtilityAction::OpenSource, "card dispatch carries displayed version and requested action");
            return QString("Source could not be opened");
        };
        card.findChild<QPushButton*>("openSource")->click();
        auto* feedback = card.findChild<QLabel*>("actionFeedback");
        check(feedback && !feedback->isHidden() && feedback->text() == "Source could not be opened", "launch error is displayed by clicked acknowledged card");
    });
    test("source links reject unsafe launches and survive state serialization", [&] {
        QTemporaryDir dir;
        pets::ResidentApplication shell(assets, dir.path(), false, [] { return 0.0; }, pets::IpcMode::DisabledForTests);
        QJsonObject input{{"source_id", "links"}, {"session_id", "one"}, {"label", "Build"}, {"seq", 1}, {"status", "working"}};
        for (const auto& url : QStringList{"relative/path", "file://remote/tmp", "https://user:secret@example.com", "javascript:alert(1)", "https://example.com/%0a", "https://example.com/\n", QString(2049, 'a')}) {
            input["source_url"] = url;
            check(shell.handleRequest("session.update", input).errorCode == -32602, "unsafe source link rejected");
        }
        input["source_url"] = 42;
        check(shell.handleRequest("session.update", input).errorCode == -32602, "source URL must be string");
        int seq = 1;
        for (const auto& url : QStringList{"https://example.com/build", "http://localhost:8000/build", "file:///tmp/build%20logs"}) {
            input["source_url"] = url; input["seq"] = seq++;
            check(shell.handleRequest("session.update", input).errorCode == 0, "supported absolute source link accepted");
            check(shell.handleRequest("state.get", {}).result["foreground"].toObject()["source_url"] == url, "source link preserved in state");
        }
    });
    test("open source does not acknowledge and expired card cannot launch", [&] {
        QTemporaryDir dir;
        double now = 0;
        pets::ResidentApplication shell(assets, dir.path(), false, [&] { return now; }, pets::IpcMode::DisabledForTests);
        SourceReceiver receiver;
        QDesktopServices::setUrlHandler("https", &receiver, "open");
        struct Reset { ~Reset() { QDesktopServices::unsetUrlHandler("https"); } } reset;
        QJsonObject input{{"source_id", "build"}, {"session_id", "open"}, {"label", "Build"}, {"seq", 1}, {"status", "failed"}, {"ttl", 2}, {"source_url", "https://example.com/task"}};
        check(shell.handleRequest("session.update", input).errorCode == 0, "source event accepted");
        QWidget* card = nullptr;
        for (auto* widget : QApplication::topLevelWidgets()) if (widget->objectName() == "sessionStatusLabel") card = widget;
        auto* open = card->findChild<QPushButton*>("openSource");
        check(open && open->isEnabled(), "source action available");
        open->click();
        check(receiver.opened == QUrl("https://example.com/task"), "user action opens reported source URL");
        check(!shell.handleRequest("state.get", {}).result["foreground"].toObject()["acknowledged"].toBool(), "opening preserves unread status");
        receiver.opened = QUrl();
        now = 3; open->click();
        check(receiver.opened.isEmpty() && shell.statusText().isEmpty(), "expired action is revalidated before launch");
    });
    test("frozen card cannot act on unseen update and expiry hides it", [&] {
        QTemporaryDir dir;
        double now = 0;
        pets::ResidentApplication shell(assets, dir.path(), false, [&] { return now; }, pets::IpcMode::DisabledForTests);
        QJsonObject input{{"source_id", "build"}, {"session_id", "one"}, {"label", "Build done"}, {"seq", 1}, {"status", "completed"}, {"ttl", 2}, {"source_url", "https://example.com/old"}};
        shell.handleRequest("session.update", input);
        QWidget* card = nullptr;
        for (auto* widget : QApplication::topLevelWidgets()) if (widget->objectName() == "sessionStatusLabel") card = widget;
        auto* ack = card->findChild<QPushButton*>("acknowledgeCard");
        shell.handleRequest("session.update", {{"source_id", "other"}, {"session_id", "background"}, {"label", "Still working"}, {"seq", 1}, {"status", "working"}, {"ttl", 30}});
        shell.control(pets::Control::Pause);
        input["seq"] = 2; input["status"] = "needs_input"; input["label"] = "New question";
        shell.handleRequest("session.update", input);
        ack->click();
        const auto unread = shell.handleRequest("state.get", {}).result;
        check(!unread["foreground"].isNull() && !unread["sessions"].toArray()[0].toObject()["acknowledged"].toBool(), "frozen old card does not acknowledge unseen question");
        now = 3; fire(shell);
        check(shell.handleRequest("state.get", {}).result["foreground"].toObject()["source_id"] == "other", "another live session survives expiry");
        check(!card->isVisible(), "paused expired card disappears while another session survives");
    });
    test("completion card and summoned toolbox share acknowledgment", [&] {
        QTemporaryDir dir;
        double now = 0;
        pets::ResidentApplication shell(assets, dir.path(), false, [&] { return now; }, pets::IpcMode::DisabledForTests);
        QJsonObject input{{"source_id", "build"}, {"session_id", "real"}, {"label", "Build finished"}, {"seq", 1}, {"status", "completed"}, {"source_url", "file:///tmp"}};
        check(shell.handleRequest("session.update", input).errorCode == 0, "completion accepted");
        QWidget* card = nullptr;
        for (auto* widget : QApplication::topLevelWidgets())
            if (widget->objectName() == "sessionStatusLabel") card = widget;
        check(card && card->isVisible(), "completion has readable card");
        check(card->testAttribute(Qt::WA_MacAlwaysShowToolWindow), "notification remains available outside app focus");
        auto* open = card->findChild<QPushButton*>("openSource");
        auto* ack = card->findChild<QPushButton*>("acknowledgeCard");
        check(open && open->isEnabled() && ack && ack->isEnabled(), "card exposes separate open and acknowledge actions");
        now = 4; fire(shell);
        check(!shell.runtime().reactionActive() && card->isVisible(), "unread card remains static after gesture");
        QAction* summon = nullptr;
        for (auto* action : shell.findChildren<QAction*>())
            if (action->objectName() == "summonToolbox") summon = action;
        check(summon, "toolbox is summoned from controls");
        summon->trigger();
        QWidget* toolbox = nullptr;
        for (auto* widget : QApplication::topLevelWidgets())
            if (widget->objectName() == "utilityToolbox") toolbox = widget;
        check(toolbox && toolbox->isVisible(), "summoned toolbox visible");
        auto* scroll = toolbox->findChild<QScrollArea*>();
        check(scroll, "notification toolbox has scrollable content");
        toolbox->grab(); // Force the first paint before any user resize.
        if (scroll->widget()->width() > scroll->viewport()->width())
            throw std::runtime_error(QString("notification toolbox initial content exceeds viewport: viewport %1px, content %2px, minimum hint %3px")
                .arg(scroll->viewport()->width()).arg(scroll->widget()->width()).arg(scroll->widget()->minimumSizeHint().width()).toStdString());
        if (scroll->horizontalScrollBar()->maximum() != 0)
            throw std::runtime_error(QString("notification toolbox initially overflows: viewport %1px, content %2px, minimum hint %3px")
                .arg(scroll->viewport()->width()).arg(scroll->widget()->width()).arg(scroll->widget()->minimumSizeHint().width()).toStdString());
        for (const int width : {420, 440, 640}) {
            toolbox->resize(width, 700); QApplication::processEvents(); QApplication::processEvents();
            if (scroll->horizontalScrollBar()->maximum() != 0)
                throw std::runtime_error(QString("notification toolbox overflows at %1px: viewport %2px, content %3px, minimum hint %4px")
                    .arg(width).arg(scroll->viewport()->width()).arg(scroll->widget()->width()).arg(scroll->widget()->minimumSizeHint().width()).toStdString());
            for (auto* button : toolbox->findChildren<QToolButton*>()) {
                const auto position = button->mapTo(scroll->viewport(), QPoint());
                check(position.x() >= 0 && position.x() + button->width() <= scroll->viewport()->width(), "notification toolbox controls fit within viewport horizontally");
            }
        }
        toolbox->resize(440, 700); QApplication::processEvents();
        const auto initialPalette = QApplication::palette();
        auto darkPalette = initialPalette; darkPalette.setColor(QPalette::Window, QColor("#292638")); darkPalette.setColor(QPalette::WindowText, QColor("#F7F4EE"));
        QApplication::setPalette(darkPalette); QApplication::processEvents();
        auto* nestedCard = toolbox->findChild<QWidget*>("utilityCard");
        check(nestedCard && nestedCard->palette().color(QPalette::Window).lightnessF() < .5, "nested notification card follows toolbox dark appearance");
        QApplication::setPalette(initialPalette); QApplication::processEvents();
        if (const auto capture = qEnvironmentVariable("CLIBBY_UTILITY_CAPTURE"); !capture.isEmpty()) {
            check(QDir().mkpath(capture), "capture output directory created");
            check(card->grab().save(capture + "/notification.png"), "notification capture saved");
            check(toolbox->grab().save(capture + "/toolbox.png"), "toolbox capture saved");
            const auto appearance = QApplication::palette();
            auto dark = appearance; dark.setColor(QPalette::Window, QColor("#292638")); dark.setColor(QPalette::WindowText, QColor("#F7F4EE"));
            QApplication::setPalette(dark); QApplication::processEvents();
            check(toolbox->grab().save(capture + "/toolbox-dark.png"), "dark toolbox capture saved");
            check(card->grab().save(capture + "/notification-dark.png"), "dark notification capture saved");
            QApplication::setPalette(appearance); QApplication::processEvents();
        }
        auto* pause = toolbox->findChild<QToolButton*>("toolboxPause");
        check(pause, "toolbox exposes existing pause control");
        pause->click(); check(shell.runtime().state().paused, "toolbox pause reaches same resident control");
        pause->click(); check(!shell.runtime().state().paused, "toolbox resume uses updated control");
        input["seq"] = 2;
        shell.handleRequest("session.update", input);
        auto* sharedAck = toolbox->findChild<QPushButton*>("acknowledgeCard");
        check(sharedAck && sharedAck->isEnabled(), "toolbox uses actionable status card");
        sharedAck->click();
        const auto state = shell.handleRequest("state.get", {}).result;
        check(state["foreground"].isNull() && state["sessions"].toArray()[0].toObject()["acknowledged"].toBool(), "completion acknowledgment clears unread foreground without clearing source state");
        check(shell.statusText().isEmpty() && !card->isVisible(), "acknowledged completion quiets above-head card");
        shell.control(pets::Control::Hide);
        check(!toolbox->isVisible(), "hide also dismisses toolbox");
    });
    test("session inputs respect pause, expiry, drag and sleep", [&] {
        QTemporaryDir dir;
        double now = 0;
        pets::ResidentApplication shell(assets, dir.path(), false, [&] { return now; }, pets::IpcMode::DisabledForTests);
        auto update = [&](int seq, const QString& status) {
            return shell.handleRequest("session.update", {{"source_id", "test"}, {"session_id", "one"},
                {"label", "<b>Build & check</b>"}, {"seq", seq}, {"status", status}, {"ttl", 10}});
        };
        const auto initialPosition = window()->position();
        check(update(1, "working").errorCode == 0, "valid input accepted");
        check(shell.statusText().contains("<b>Build & check</b>") && shell.statusText().contains("working"), "status is literal and understandable");
        check(shell.runtime().reactionActive(), "creation begins bounded reaction");
        shell.control(pets::Control::Pause);
        const auto frozen = shell.statusText();
        now = 6;
        check(update(2, "needs_input").errorCode == 0, "paused data accepted");
        check(shell.statusText() == frozen && !shell.runtime().reactionActive(), "paused label stays frozen and reaction suppressed");
        auto state = shell.handleRequest("state.get", {}).result;
        check(state["foreground"].toObject()["status"] == "needs_input", "paused state query remains current");
        check(shell.timerActive() && shell.timerInterval() == 10000, "paused expiry deadline remains scheduled");
        shell.control(pets::Control::Resume);
        check(shell.statusText().contains("needs_input") && !shell.runtime().reactionActive(), "resume updates label without replay");
        beginDrag();
        now = 12;
        check(update(3, "failed").errorCode == 0 && !shell.runtime().reactionActive(), "drag suppresses notification gesture");
        shell.control(pets::Control::EndDrag);
        check(window()->position() == initialPosition, "notifications never move pet window");
        shell.control(pets::Control::Suspend);
        shell.control(pets::Control::Wake);
        check(shell.handleRequest("state.get", {}).result["sessions"].toArray().isEmpty(), "wake removes records and history");
        check(update(0, "completed").errorCode == 0 && shell.runtime().reactionActive(), "postwake sequence and cooldown are fresh");
        shell.control(pets::Control::Hide);
        now = 22;
        fire(shell);
        check(!shell.timerActive() && shell.handleRequest("state.get", {}).result["sessions"].toArray().isEmpty(), "hidden expiry settles without polling");
    });
    test("session request validation and local acknowledgment", [&] {
        QTemporaryDir dir;
        pets::ResidentApplication shell(assets, dir.path(), false, [] { return 0.0; }, pets::IpcMode::DisabledForTests);
        QJsonObject input{{"source_id", "test"}, {"session_id", "one"}, {"label", "Build"}, {"seq", 1}, {"status", "failed"}};
        check(shell.handleRequest("session.update", input).errorCode == 0, "valid defaultTTL accepted");
        check(shell.handleRequest("session.update", input).errorCode == -32003, "stale update machine error");
        input["seq"] = 2.5;
        check(shell.handleRequest("session.update", input).errorCode == -32602, "fractional sequence rejected");
        input["seq"] = 2;
        input["label"] = QString("bad\nlabel");
        check(shell.handleRequest("session.update", input).errorCode == -32602, "control in label rejected");
        QAction* acknowledge = nullptr;
        for (auto* action : shell.findChildren<QAction*>())
            if (action->objectName() == "acknowledgeSession") acknowledge = action;
        check(acknowledge != nullptr, "menu exposes attention acknowledgment");
        acknowledge->trigger();
        const auto state = shell.handleRequest("state.get", {}).result;
        check(state["foreground"].isNull() && state["sessions"].toArray().size() == 1, "acknowledgment removes attention while retaining reported session");
        check(shell.statusText().isEmpty() && !shell.runtime().reactionActive(), "acknowledgment cancels indicator and gesture");
    });
    test("notification intents have distinct bounded local poses", [&] {
        const auto rig = loadRig(assets);
        const Motion motion(rig);
        const auto rest = motion.pose();
        std::vector<QPointF> roots;
        for (auto intent : {pets::BehaviorIntent::Acknowledge, pets::BehaviorIntent::Attend,
                            pets::BehaviorIntent::Celebrate, pets::BehaviorIntent::ExpressFailure}) {
            check(pets::reactionPose(rest, intent, 0).json(rig) == rest.json(rig), "gesture starts at current pose");
            check(pets::reactionPose(rest, intent, 1).json(rig) == rest.json(rig), "gesture ends at rest");
            const auto pose = pets::reactionPose(rest, intent, 0.37);
            check(pose.root != rest.root && std::abs(pose.root.x() - rest.root.x()) <= 5
                && std::abs(pose.root.y() - rest.root.y()) <= 14, "local offsets stay bounded");
            for (const auto& root : roots) check(root != pose.root, "intents produce distinct movement");
            roots.push_back(pose.root);
        }
        check(motion.pose().json(rig) == rest.json(rig), "reaction never mutates fixture motion");
    });
    test("typed fixture motion matches strings and caches parameters", [&] {
        Rig rig = loadRig(assets);
        Motion typed(rig), strings(rig);
        for (int tick = 0; tick < 120; ++tick) {
            typed.step(MotionPhase::Walk);
            strings.step("walk-desktop");
        }
        check(typed.pose().json(rig) == strings.pose().json(rig), "typed walking preserves fixture equations");
        const auto frozen = typed.pose().json(rig);
        typed.step(MotionPhase::Static);
        check(typed.pose().json(rig) == frozen, "typed static preserves pose");
        const Rig original = rig;
        Motion cached(rig), reference(original);
        rig.motion["tail_stiffness"] = 999.0;
        rig.motion["root_amplitude"] = 999.0;
        cached.step(MotionPhase::Walk);
        reference.step("walk-desktop");
        check(cached.pose().json(rig) == reference.pose().json(original), "motion parameters read only during construction");
    });
    test("suppression stops actual timer and flags remain independent", [&] {
        QTemporaryDir dir;
        double now = 0;
        pets::ResidentApplication shell(assets, dir.path(), false, [&] { return now; }, pets::IpcMode::DisabledForTests);
        check(shell.petVisible(), "resident launches visibly");
        check(shell.timerActive() && shell.timerInterval() == 30000, "quiet resident has one future deadline");
        shell.control(pets::Control::Pause);
        check(!shell.timerActive(), "pause cancels actual timer");
        shell.control(pets::Control::Hide);
        shell.control(pets::Control::Show);
        check(shell.petVisible() && shell.runtime().state().paused && !shell.timerActive(), "show retains pause");
        shell.control(pets::Control::Hide);
        shell.control(pets::Control::Resume);
        check(!shell.petVisible() && !shell.timerActive(), "resume retains hidden state");
        now = 100;
        fire(shell);
        check(shell.runtime().diagnostics().steps == 0 && !shell.timerActive(), "late hidden timeout cannot animate");
        shell.control(pets::Control::Show);
        check(shell.timerActive() && shell.timerInterval() == 30000, "show starts fresh quiet interval");
    });
    test("late callbacks and wake cannot replay backlog", [&] {
        QTemporaryDir dir;
        double now = 0;
        pets::ResidentApplication shell(assets, dir.path(), false, [&] { return now; }, pets::IpcMode::DisabledForTests);
        now = 30;
        fire(shell);
        now = 30.5;
        fire(shell);
        check(shell.runtime().diagnostics().steps == 4, "late callback advances at most four motion steps");
        check(shell.runtime().diagnostics().droppedSteps >= 25, "late steps are counted and dropped");
        shell.control(pets::Control::Suspend);
        check(!shell.timerActive(), "suspend stops timer");
        now = 3600;
        shell.control(pets::Control::Wake);
        check(shell.timerInterval() == 30000 && shell.runtime().diagnostics().steps == 4, "wake gets fresh deadline without catchup");
        now = 7200;
        fire(shell);
        check(shell.runtime().diagnostics().steps == 4 && shell.timerInterval() == 30000, "missed quiet gesture is skipped");
    });
    test("hide and suspend end real dragging", [&] {
        QTemporaryDir dir;
        pets::ResidentApplication shell(assets, dir.path(), false, [] { return 0.0; }, pets::IpcMode::DisabledForTests);
        beginDrag();
        check(shell.runtime().state().dragging && !shell.timerActive(), "pointer drag suppresses timer");
        shell.control(pets::Control::Suspend);
        check(!shell.runtime().state().dragging && shell.runtime().state().suspended, "suspend clears lost pointer drag");
        shell.control(pets::Control::Wake);
        beginDrag();
        shell.control(pets::Control::Hide);
        check(!shell.runtime().state().dragging && !shell.petVisible(), "hide clears pointer drag");
    });
    test("preferences recover and pause persists across visible restart", [&] {
        QTemporaryDir dir;
        QFile invalid(dir.filePath("settings.json"));
        check(invalid.open(QIODevice::WriteOnly), "invalid settings created");
        invalid.write("{bad"); invalid.close();
        {
            pets::ResidentApplication shell(assets, dir.path(), false, [] { return 0.0; }, pets::IpcMode::DisabledForTests);
            check(!shell.runtime().state().paused && shell.petVisible(), "invalid settings recover to visible defaults");
            shell.control(pets::Control::Pause);
            shell.control(pets::Control::Hide);
        }
        pets::ResidentApplication restarted(assets, dir.path(), false, [] { return 0.0; }, pets::IpcMode::DisabledForTests);
        check(restarted.runtime().state().paused && restarted.petVisible() && !restarted.timerActive(), "restart persists pause but launches visible");
    });
    test("placement is saved on drag release instead of each pointer move", [&] {
        QTemporaryDir dir;
        pets::ResidentApplication shell(assets, dir.path(), false, [] { return 0.0; }, pets::IpcMode::DisabledForTests);
        const auto path = dir.filePath("settings.json");
        const auto before = contents(path);
        beginDrag();
        auto* target = window();
        const QPointF local(128, 160), global = target->position() + local + QPointF(-30, -20);
        QMouseEvent move(QEvent::MouseMove, local, local, global,
                         Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(target, &move);
        check(contents(path) == before, "drag move does not write settings");
        QMouseEvent release(QEvent::MouseButtonRelease, local, local, global,
                            Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(target, &release);
        const auto saved = pets::loadPreferences(path);
        check(contents(path) != before && saved.position == target->position(), "drag release persists realized position once");
    });
    test("failed persistence is visible and errors do not repeat", [&] {
        QTemporaryDir dir;
        check(QDir().mkdir(dir.filePath("settings.json")), "settings destination blocked by directory");
        std::vector<QString> captured;
        messages = &captured;
        const auto oldHandler = qInstallMessageHandler(captureMessages);
        // Always restore the process-wide handler even if an assertion fails.
        struct Restore {
            QtMessageHandler handler;
            ~Restore() { qInstallMessageHandler(handler); messages = nullptr; }
        } restore{oldHandler};
        pets::ResidentApplication shell(assets, dir.path(), false, [] { return 0.0; }, pets::IpcMode::DisabledForTests);
        shell.control(pets::Control::Pause);
        shell.control(pets::Control::Resume);
        unsigned errors = 0;
        for (const auto& message : captured) if (message.contains("could not save preferences")) ++errors;
        check(errors == 1, "persistent write failure emits one diagnostic");
        QAction* status = nullptr;
        for (auto* action : shell.findChildren<QAction*>())
            if (action->text() == "Settings could not be saved") status = action;
        check(status && status->isVisible(), "settings failure visible in resident menu");
        check(QDir().rmdir(dir.filePath("settings.json")), "settings destination unblocked");
        shell.control(pets::Control::Pause);
        check(!status->isVisible() && pets::loadPreferences(dir.filePath("settings.json")).paused, "successful save clears error and persists pause");
    });
    test("real instance ownership and errors", [&] {
        QTemporaryDir dir;
        pets::Instance first(dir.path());
        check(first.result() == pets::Instance::Result::Acquired, "first real lock acquired");
        pets::Instance duplicate(dir.path());
        check(duplicate.result() == pets::Instance::Result::AlreadyRunning, "duplicate real lock refused");
        first.release();
        pets::Instance next(dir.path());
        check(next.result() == pets::Instance::Result::Acquired, "real lock available after release");
        QFile blocker(dir.filePath("file")); check(blocker.open(QIODevice::WriteOnly), "lock blocker created"); blocker.close();
        pets::Instance invalid(blocker.fileName());
        check(invalid.result() == pets::Instance::Result::Error && !invalid.error().isEmpty(), "lock IO failure differs from live owner");
    });
    test("unavailable tray fails clearly and startup failure releases lock", [&] {
        if (QSystemTrayIcon::isSystemTrayAvailable()) return;
        QTemporaryDir dir;
        bool rejected = false;
        try { pets::ResidentApplication shell(assets, dir.path(), true, [] { return 0.0; }, pets::IpcMode::DisabledForTests); }
        catch (const std::exception& error) {
            rejected = QString::fromLocal8Bit(error.what()).contains("controls are unavailable");
        }
        check(rejected, "desktop startup requires persistent controls");
        check(window() == nullptr, "failed startup creates no unreachable pet");
        pets::Instance next(dir.path());
        check(next.result() == pets::Instance::Result::Acquired, "failed startup releases instance lock");
    });
    test("quit releases resources and queued callbacks have owner contexts", [&] {
        QTemporaryDir dir;
        auto shell = std::make_unique<pets::ResidentApplication>(assets, dir.path(), false, [] { return 0.0; }, pets::IpcMode::DisabledForTests);
        auto* timer = shell->findChild<QTimer*>();
        auto actions = shell->findChildren<QAction*>();
        check(!actions.isEmpty(), "resident actions exist without tray UI");
        QMetaObject::invokeMethod(timer, "timeout", Qt::QueuedConnection);
        for (auto* action : actions) QMetaObject::invokeMethod(action, "trigger", Qt::QueuedConnection);
        shell->control(pets::Control::Quit);
        check(!shell->timerActive() && !shell->petVisible() && window() == nullptr, "quit destroys host and stops timer");
        pets::Instance next(dir.path());
        check(next.result() == pets::Instance::Result::Acquired, "quit releases instance lock before shell destruction");
        shell.reset();
        QApplication::processEvents();
        check(window() == nullptr, "queued callbacks after destruction cannot recreate windows");
    });
    test("scene graph failure releases resident ownership", [&] {
        QTemporaryDir dir;
        pets::ResidentApplication shell(assets, dir.path(), false, [] { return 0.0; }, pets::IpcMode::DisabledForTests);
        auto* target = window();
        check(target != nullptr, "pet renderer window exists");
        check(QMetaObject::invokeMethod(target, "sceneGraphError", Qt::DirectConnection,
                  Q_ARG(QQuickWindow::SceneGraphError, QQuickWindow::ContextNotAvailable),
                  Q_ARG(QString, QString("synthetic context failure"))), "real scene graph error signal delivered");
        QApplication::processEvents();
        check(shell.runtime().state().quitting && !shell.timerActive() && !shell.petVisible(), "render failure stops resident");
        pets::Instance next(dir.path());
        check(next.result() == pets::Instance::Result::Acquired, "render failure releases lock");
    });
#ifdef __APPLE__
    test("native notification observer deregisters and queued callbacks expire", [&] {
        auto context = std::make_unique<QObject>();
        unsigned calls = 0;
        auto observer = pets::observeSleep(context.get(), [&](pets::Control command) {
            check(QThread::currentThread() == app.thread(), "native notification delivered on application thread");
            check(command == pets::Control::Suspend || command == pets::Control::Wake, "native sleep control typed");
            ++calls;
        });
        auto* center = [[NSWorkspace sharedWorkspace] notificationCenter];
        [center postNotificationName:NSWorkspaceWillSleepNotification object:nil];
        QApplication::processEvents();
        check(calls == 1, "workspace sleep observed");
        [center postNotificationName:NSWorkspaceDidWakeNotification object:nil];
        observer.reset();
        QApplication::processEvents();
        check(calls == 1, "observer destruction invalidates queued wake");
        observer = pets::observeSleep(context.get(), [&](pets::Control) { ++calls; });
        [center postNotificationName:NSWorkspaceDidWakeNotification object:nil];
        context.reset();
        QApplication::processEvents();
        check(calls == 1, "context destruction invalidates queued wake");
        observer.reset();
        [center postNotificationName:NSWorkspaceWillSleepNotification object:nil];
        QApplication::processEvents();
        check(calls == 1, "native observer tokens removed");
    });
#endif
    return failures ? 1 : 0;
}

#include "application_tests.moc"
