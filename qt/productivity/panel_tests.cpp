#include "productivity/panel.h"
#include "productivity/service.h"
#include <QApplication>
#include <QCheckBox>
#include <QDateTime>
#include <QDir>
#include <QTabWidget>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPalette>
#include <QPushButton>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QTextEdit>
#include <iostream>
#include <stdexcept>

using namespace pets::productivity;
static void require(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
template<class T> static T *control(Panel &panel, const char *name) {
    auto *found = panel.findChild<T *>(QString::fromLatin1(name));
    require(found != nullptr, name); return found;
}
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    try {
        QTemporaryDir dir; require(dir.isValid(), "temporary storage");
        qint64 now = 100000;
        Service service(dir.filePath("productivity.json"));
        auto call = [&](const QString &op, const QJsonObject &params) { return service.call(op, params, now); };
        call("publish", {{"source_id", "claude"}, {"event_id", "one"}, {"seq", 0},
                         {"kind", "needs_input"}, {"title", "<b>Permission?</b>"},
                         {"detail", "<img src=x onerror=alert(1)>"}});
        const QPalette systemPalette = app.palette();
        Panel panel(call); panel.refresh();
        require(app.palette() == systemPalette, "Clibby panel leaves application palette unchanged");
        const auto checkAppearance = [&](const QColor &background, const QColor &foreground) {
            auto appearance = systemPalette;
            appearance.setColor(QPalette::Window, background);
            appearance.setColor(QPalette::WindowText, foreground);
            app.setPalette(appearance); app.processEvents();
            const auto &palette = panel.palette();
            require(qAbs(palette.color(QPalette::Window).lightnessF() - palette.color(QPalette::WindowText).lightnessF()) > 0.5,
                    "panel retains readable window contrast after appearance change");
            require(qAbs(palette.color(QPalette::Highlight).lightnessF() - palette.color(QPalette::HighlightedText).lightnessF()) > 0.4,
                    "selected text retains contrast in both appearances");
            require((palette.color(QPalette::Window).lightnessF() < 0.5) == (background.lightnessF() < 0.5),
                    "panel follows system light and dark appearance");
        };
        checkAppearance(QColor("#F7F4EE"), QColor("#292638"));
        checkAppearance(QColor("#292638"), QColor("#F7F4EE"));
        app.setPalette(systemPalette); app.processEvents();
        auto *inbox = control<QListWidget>(panel, "inboxList");
        require(inbox->count() == 1, "inbox renders published event");
        inbox->setCurrentRow(0);
        require(inbox->currentItem()->text().contains("<b>Permission?</b>"), "title remains literal text");
        require(control<QTextEdit>(panel, "itemDetail")->toPlainText().contains("<img src=x onerror=alert(1)>"), "detail remains plain text");
        control<QPushButton>(panel, "acknowledgeItem")->click();
        auto item = call("list", {}).value("items").toArray().at(0).toObject();
        require(item.value("acknowledged").toBool() && !item.value("completed").toBool(), "acknowledgment does not complete work");
        control<QCheckBox>(panel, "attentionOnly")->setChecked(false);
        inbox->setCurrentRow(0);
        control<QPushButton>(panel, "snoozeItem")->click();
        item = call("list", {}).value("items").toArray().at(0).toObject();
        require(item.value("snoozed_until").toDouble() > now && !item.value("completed").toBool(), "snooze does not complete work");
        control<QPushButton>(panel, "completeItem")->click();
        require(call("list", {}).value("items").toArray().at(0).toObject().value("completed").toBool(), "complete marks local follow-up done");
        call("publish", {{"source_id", "codex"}, {"event_id", "stale-link"}, {"seq", 0}, {"kind", "needs_input"}, {"title", "Source changes"}, {"source_url", "https://example.com/old"}});
        control<QLineEdit>(panel, "inboxSearch")->setText("Source changes");
        inbox->setCurrentRow(0);
        call("publish", {{"source_id", "codex"}, {"event_id", "stale-link"}, {"seq", 1}, {"kind", "needs_input"}, {"title", "Source changes"}, {"source_url", "https://example.com/new"}});
        control<QPushButton>(panel, "openItem")->click();
        require(control<QLabel>(panel, "productivityError")->text().contains("changed"), "source return revalidates current sequence");
        item = call("list", {{"query", "Source changes"}}).value("items").toArray().at(0).toObject();
        require(!item["acknowledged"].toBool() && !item["completed"].toBool(), "source return does not acknowledge or complete");
        control<QLineEdit>(panel, "inboxSearch")->clear();
        control<QLineEdit>(panel, "taskTitle")->setText("Review tomorrow");
        control<QPushButton>(panel, "captureTask")->click();
        auto tasks = call("list", {{"query", "Review tomorrow"}}).value("items").toArray();
        require(tasks.size() == 1 && tasks.at(0).toObject().value("source_id") == "tasks", "quick capture creates persistent task");
        for (int i = 0; i < 115; ++i)
            call("publish", {{"source_id", "codex"}, {"event_id", QString("job%1").arg(i)}, {"seq", 0}, {"kind", "info"}, {"title", QString("Batch job %1").arg(i)}});
        control<QLineEdit>(panel, "inboxSearch")->setText("Batch job");
        require(inbox->count() == 115, "panel aggregates bounded pages with stable search");
        inbox->setCurrentRow(72); const QString selected = inbox->currentItem()->text(); panel.refresh();
        require(inbox->currentItem() && inbox->currentItem()->text() == selected, "selection survives refresh");
        control<QLineEdit>(panel, "timerLabel")->setText("Writing");
        control<QSpinBox>(panel, "timerSeconds")->setValue(60);
        control<QSpinBox>(panel, "breakSeconds")->setValue(10);
        control<QSpinBox>(panel, "longBreakSeconds")->setValue(30);
        control<QSpinBox>(panel, "pomodoroRounds")->setValue(2);
        control<QPushButton>(panel, "createPomodoro")->click();
        auto *timers = control<QListWidget>(panel, "timerList");
        require(timers->count() == 1 && timers->item(0)->text().contains("work"), "Pomodoro work phase displayed");
        timers->setCurrentRow(0); control<QPushButton>(panel, "pauseTimer")->click();
        require(call("timers", {}).value("timers").toArray().at(0).toObject().value("state") == "paused", "pause timer persists");
        control<QPushButton>(panel, "resumeTimer")->click();
        now += 60000; service.advance(now); panel.refresh();
        require(timers->currentItem()->text().contains("waiting"), "refresh shows due waiting state");
        control<QPushButton>(panel, "nextTimer")->click();
        auto timer = call("timers", {}).value("timers").toArray().at(0).toObject();
        require(timer.value("phase") == "short_break" && timer.value("remaining_ms").toDouble() == 10000, "configured break duration reaches service");
        control<QPushButton>(panel, "cancelTimer")->click();
        require(call("timers", {}).value("timers").toArray().at(0).toObject().value("state") == "cancelled", "cancel persists");
        panel.setError("<b>offline</b>");
        require(control<QLabel>(panel, "productivityError")->textFormat() == Qt::PlainText, "errors use plain text");
        Panel warningPanel([&](const QString &op, const QJsonObject &params) {
            auto result = call(op, params);
            if (op == "list") result["warning"] = "Storage unavailable; scheduling suspended";
            if (op == "timers") result["warning"] = "Timers require storage recovery";
            return result;
        });
        warningPanel.refresh();
        require(control<QLabel>(warningPanel, "productivityError")->text().contains("scheduling suspended"), "successful inbox query preserves reconciliation warning");
        require(control<QLabel>(warningPanel, "productivityError")->text().contains("Timers require storage recovery"), "timer reconciliation warning survives successful refresh");
        require(control<QListWidget>(warningPanel, "inboxList")->count() > 0, "history stays readable during reconciliation warning");
        const QString captureDir = qEnvironmentVariable("CLIBBY_PRODUCTIVITY_CAPTURE");
        if (!captureDir.isEmpty()) {
            require(QDir().mkpath(captureDir), "create capture directory");
            // Independent presentation data; the functional assertions above keep
            // their deterministic clock, adversarial strings and pagination load.
            qint64 showcaseNow = QDateTime::currentMSecsSinceEpoch() - 12 * 60000;
            Service showcaseService(dir.filePath("showcase.json"));
            auto showcaseCall = [&](const QString &op, const QJsonObject &params) {
                return showcaseService.call(op, params, showcaseNow);
            };
            showcaseCall("publish", {{"source_id", "tasks"}, {"event_id", "tomorrow"}, {"seq", 0}, {"kind", "info"},
                {"title", "Plan tomorrow's focus"}, {"detail", "Choose one thing to finish before opening the next project."}});
            showcaseNow += 5 * 60000;
            showcaseCall("publish", {{"source_id", "codex"}, {"event_id", "interface-review"}, {"seq", 0}, {"kind", "response_ready"},
                {"title", "Your interface review is ready"}, {"detail", "The latest changes are ready for you to review when you have a moment."}});
            showcaseNow += 7 * 60000;
            showcaseCall("publish", {{"source_id", "claude"}, {"event_id", "permission"}, {"seq", 0}, {"kind", "needs_input"},
                {"title", "A quick decision before the next step"}, {"detail", "The agent is waiting for your input. Return to the source to review the request and decide how to continue."},
                {"source_url", "https://example.com/review"}});
            auto light = systemPalette; light.setColor(QPalette::Window, QColor("#F7F4EE")); light.setColor(QPalette::WindowText, QColor("#292638"));
            app.setPalette(light); app.processEvents();
            Panel showcase(showcaseCall); showcase.refresh();
            control<QListWidget>(showcase, "inboxList")->setCurrentRow(0);
            showcase.show(); app.processEvents();
            require(showcase.grab().save(QDir(captureDir).filePath("inbox.png")), "save inbox capture");
            control<QLineEdit>(showcase, "timerLabel")->setText("Focused writing");
            control<QPushButton>(showcase, "createPomodoro")->click();
            control<QLineEdit>(showcase, "timerLabel")->setText("Take a stretch break");
            control<QSpinBox>(showcase, "timerSeconds")->setValue(900);
            control<QPushButton>(showcase, "createTimer")->click();
            control<QLineEdit>(showcase, "timerLabel")->setText("Focused writing");
            control<QSpinBox>(showcase, "timerSeconds")->setValue(1500);
            control<QListWidget>(showcase, "timerList")->setCurrentRow(0);
            showcase.findChild<QTabWidget *>()->setCurrentIndex(1); app.processEvents();
            require(showcase.grab().save(QDir(captureDir).filePath("timers.png")), "save timer capture");
            auto dark = systemPalette; dark.setColor(QPalette::Window, QColor("#292638")); dark.setColor(QPalette::WindowText, QColor("#F7F4EE"));
            app.setPalette(dark); app.processEvents();
            require(showcase.grab().save(QDir(captureDir).filePath("timers-dark.png")), "save dark timer capture");
            showcase.findChild<QTabWidget *>()->setCurrentIndex(0); app.processEvents();
            require(showcase.grab().save(QDir(captureDir).filePath("inbox-dark.png")), "save dark inbox capture");
            app.setPalette(systemPalette);
        }
        std::cout << "Productivity panel tests passed\n";
    } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
    return 0;
}
