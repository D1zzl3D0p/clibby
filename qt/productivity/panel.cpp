#include "productivity/panel.h"
#include "resident/brand_theme.h"
#include <QCheckBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QStringList>
#include <QSpinBox>
#include <QTabWidget>
#include <QTextEdit>
#include <QUrl>
#include <QUuid>
#include <QVBoxLayout>
#include <stdexcept>

namespace pets::productivity {
namespace {
QString identity(const QJsonObject &item) { return item["source_id"].toString() + '/' + item["event_id"].toString(); }
QString timestamp(const QJsonValue &value) {
    return QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(value.toDouble())).toLocalTime().toString("yyyy-MM-dd HH:mm:ss");
}
bool safeUrl(const QString &text) {
    if (text.isEmpty() || text.toUtf8().size() > 2048) return false;
    for (int i = 0; i < text.size(); ++i)
        if (text[i] == '%' && (i + 2 >= text.size() || !QRegularExpression("^[0-9a-fA-F]{2}$").match(text.mid(i + 1, 2)).hasMatch())) return false;
    for (auto c : QUrl::fromPercentEncoding(text.toUtf8()).toUcs4())
        if (c < 32 || (c >= 127 && c <= 159)) return false;
    const QUrl url(text, QUrl::StrictMode);
    return url.isValid() && !url.isRelative() && url.userInfo().isEmpty()
        && ((url.scheme() == "http" || url.scheme() == "https") ? !url.host().isEmpty()
            : url.scheme() == "file" && url.authority().isEmpty() && url.path().startsWith('/'));
}
QString duration(qint64 ms) {
    const auto seconds = qMax<qint64>(0, (ms + 999) / 1000);
    return QString("%1:%2:%3").arg(seconds / 3600).arg((seconds / 60) % 60, 2, 10, QLatin1Char('0')).arg(seconds % 60, 2, 10, QLatin1Char('0'));
}
}
Panel::Panel(std::function<QJsonObject(const QString &, const QJsonObject &)> call) : call_(std::move(call)) {
    setObjectName("productivityPanel"); setWindowTitle("Clibby — Productivity"); resize(900, 740);
    setMinimumSize(840, 620);
    auto *layout = new QVBoxLayout(this);
    brand::spacing(layout, 24); layout->setSpacing(16);
    layout->addWidget(brand::header(this, "Productivity", "A little space for what needs your attention."));
    error_ = new QLabel(this); error_->setObjectName("productivityError"); error_->setTextFormat(Qt::PlainText); error_->setWordWrap(true); error_->hide(); layout->addWidget(error_);
    auto *tabs = new QTabWidget(this); layout->addWidget(tabs);
    auto *inboxPage = new QWidget(tabs); auto *inboxLayout = new QVBoxLayout(inboxPage); brand::spacing(inboxLayout, 0); inboxLayout->setContentsMargins(0, 16, 0, 0); inboxLayout->setSpacing(12); tabs->addTab(inboxPage, "Attention inbox");
    auto *filters = new QHBoxLayout; inboxLayout->addLayout(filters);
    search_ = new QLineEdit(inboxPage); search_->setObjectName("inboxSearch"); search_->setPlaceholderText("Search events and tasks"); filters->addWidget(search_);
    search_->setAccessibleName("Search events and tasks"); search_->setClearButtonEnabled(true);
    attention_ = new QCheckBox("Needs attention", inboxPage); attention_->setObjectName("attentionOnly"); attention_->setChecked(true); filters->addWidget(attention_);
    inbox_ = new QListWidget(inboxPage); inbox_->setObjectName("inboxList"); inbox_->setWordWrap(true); inboxLayout->addWidget(inbox_, 2);
    detail_ = new QTextEdit(inboxPage); detail_->setObjectName("itemDetail"); detail_->setReadOnly(true); detail_->setAcceptRichText(false); inboxLayout->addWidget(detail_, 1);
    detail_->setAccessibleName("Selected item details"); detail_->setPlaceholderText("Choose an item to read the details.");
    inbox_->setAccessibleName("Attention inbox"); inbox_->setSpacing(2);
    auto button = [this](QLayout *parent, const QString &label, const char *name, auto action) {
        auto *control = new QPushButton(label, this); control->setObjectName(name); parent->addWidget(control);
        connect(control, &QPushButton::clicked, this, [this, action] { perform(action); }); return control;
    };
    auto *actions = new QHBoxLayout; inboxLayout->addLayout(actions);
    open_ = button(actions, "Open source", "openItem", [this] { openSource(); });
    acknowledge_ = button(actions, "Acknowledge", "acknowledgeItem", [this] { act("acknowledge"); });
    acknowledge_->setProperty("clibbyPrimary", true);
    snooze_ = button(actions, "Snooze", "snoozeItem", [this] { act("snooze"); });
    snoozeSeconds_ = new QSpinBox(this); snoozeSeconds_->setObjectName("snoozeSeconds"); snoozeSeconds_->setRange(1, 604800); snoozeSeconds_->setValue(300); snoozeSeconds_->setSuffix(" sec"); actions->addWidget(snoozeSeconds_);
    complete_ = button(actions, "Complete follow-up", "completeItem", [this] { act("complete"); });
    auto *capture = new QHBoxLayout; inboxLayout->addLayout(capture);
    taskTitle_ = new QLineEdit(this); taskTitle_->setObjectName("taskTitle"); taskTitle_->setMaxLength(120); taskTitle_->setPlaceholderText("Capture a task"); capture->addWidget(taskTitle_);
    taskTitle_->setAccessibleName("New task title");
    auto *captureButton = button(capture, "Add task", "captureTask", [this] {
        if (taskTitle_->text().trimmed().isEmpty()) throw std::runtime_error("Enter a task title.");
        call_("publish", {{"source_id", "tasks"}, {"event_id", QUuid::createUuid().toString(QUuid::WithoutBraces)}, {"seq", 0}, {"kind", "info"}, {"title", taskTitle_->text().trimmed()}});
        taskTitle_->clear(); refresh();
    });
    captureButton->setProperty("clibbyPrimary", true);
    auto *timerPage = new QWidget(tabs); auto *timerLayout = new QVBoxLayout(timerPage); brand::spacing(timerLayout, 0); timerLayout->setContentsMargins(0, 16, 0, 0); timerLayout->setSpacing(16); tabs->addTab(timerPage, "Timers and Pomodoro");
    auto *form = new QFormLayout; timerLayout->addLayout(form);
    form->setHorizontalSpacing(24); form->setVerticalSpacing(12);
    timerLabel_ = new QLineEdit(this); timerLabel_->setObjectName("timerLabel"); timerLabel_->setMaxLength(120); timerLabel_->setPlaceholderText("Task or timer name"); form->addRow("Label", timerLabel_);
    auto spin = [this, form](const char *name, const QString &label, int value, int max) {
        auto *control = new QSpinBox(this); control->setObjectName(name); control->setRange(1, max); control->setValue(value); form->addRow(label, control); return control;
    };
    seconds_ = spin("timerSeconds", "Countdown / work seconds", 1500, 604800);
    breakSeconds_ = spin("breakSeconds", "Short break seconds", 300, 604800);
    longBreakSeconds_ = spin("longBreakSeconds", "Long break seconds", 900, 604800);
    rounds_ = spin("pomodoroRounds", "Work intervals before long break", 4, 100);
    auto *create = new QHBoxLayout; timerLayout->addLayout(create);
    button(create, "Start countdown", "createTimer", [this] { createTimer("countdown"); });
    button(create, "Start Pomodoro", "createPomodoro", [this] { createTimer("pomodoro"); })->setProperty("clibbyPrimary", true);
    auto *hint = new QLabel("Remaining time is updated when refreshed. Pomodoro waits for you to start the next interval.", this); hint->setWordWrap(true); timerLayout->addWidget(hint);
    hint->setProperty("clibbyRole", "muted");
    timers_ = new QListWidget(this); timers_->setObjectName("timerList"); timers_->setWordWrap(true); timerLayout->addWidget(timers_);
    timers_->setAccessibleName("Timers and Pomodoro intervals"); timers_->setSpacing(2);
    auto *timerActions = new QHBoxLayout; timerLayout->addLayout(timerActions);
    button(timerActions, "Pause", "pauseTimer", [this] { timerAct("pause"); });
    button(timerActions, "Resume", "resumeTimer", [this] { timerAct("resume"); });
    button(timerActions, "Next interval", "nextTimer", [this] { timerAct("next"); });
    button(timerActions, "Cancel", "cancelTimer", [this] { timerAct("cancel"); });
    auto *footer = new QHBoxLayout; footer->addStretch(); layout->addLayout(footer);
    button(footer, "Refresh", "refreshProductivity", [this] { refresh(); });
    connect(search_, &QLineEdit::textChanged, this, [this] { refresh(); });
    connect(attention_, &QCheckBox::toggled, this, [this] { refresh(); });
    connect(inbox_, &QListWidget::currentRowChanged, this, [this] { updateSelection(); });
    updateSelection();
    brand::install(this);
}
void Panel::setError(const QString &message) { error_->setText(message); error_->setVisible(!message.isEmpty()); }
void Panel::perform(const std::function<void()> &action) {
    try { setError({}); action(); } catch (const std::exception &error) { setError(QString::fromUtf8(error.what())); }
}
void Panel::refresh() {
    perform([this] {
        const QString selected = inbox_->currentItem() ? inbox_->currentItem()->data(Qt::UserRole).toString() : QString();
        const QString selectedTimer = timers_->currentItem() ? timers_->currentItem()->data(Qt::UserRole).toString() : QString();
        QVector<QJsonObject> items;
        QStringList warnings;
        const auto collectWarning = [&warnings](const QJsonObject &result) {
            const QString warning = result["warning"].toString();
            if (!warning.isEmpty() && !warnings.contains(warning)) warnings.append(warning);
        };
        const QString query = search_->text(); const bool attention = attention_->isChecked();
        for (int offset = 0; offset < 512;) {
            const auto page = call_("list", {{"query", query}, {"attention_only", attention}, {"offset", offset}, {"limit", 50}});
            collectWarning(page);
            const auto rows = page["items"].toArray();
            if (rows.isEmpty()) break;
            for (const auto &row : rows) { if (items.size() == 512) break; items.append(row.toObject()); }
            offset += rows.size();
            if (offset >= page["total"].toInt()) break;
        }
        const auto timerResult = call_("timers", {});
        collectWarning(timerResult);
        const auto timerRows = timerResult.value("timers").toArray();
        { QSignalBlocker block(inbox_); inbox_->clear(); items_ = std::move(items);
          for (int i = 0; i < items_.size(); ++i) {
            const auto &item = items_[i];
            QString status = item["completed"].toBool() ? "Follow-up done" : item["acknowledged"].toBool() ? "Seen" : "Unread";
            if (item["snoozed_until"].toDouble() > QDateTime::currentMSecsSinceEpoch()) status += " · Snoozed until " + timestamp(item["snoozed_until"]);
            auto *row = new QListWidgetItem(item["title"].toString() + '\n' + item["source_id"].toString() + " · " + item["kind"].toString() + " · " + timestamp(item["updated_at"]) + " · " + status, inbox_);
            row->setData(Qt::UserRole, identity(item));
            if (identity(item) == selected) inbox_->setCurrentRow(i);
          }
        }
        updateSelection();
        timers_->clear();
        for (const auto &value : timerRows) {
            const auto timer = value.toObject();
            auto *row = new QListWidgetItem(timer["label"].toString() + " · " + timer["mode"].toString() + " · " + timer["phase"].toString() + " · " + timer["state"].toString() + " · " + duration(static_cast<qint64>(timer["remaining_ms"].toDouble())), timers_);
            row->setData(Qt::UserRole, timer["id"]);
            if (timer["id"].toString() == selectedTimer) timers_->setCurrentItem(row);
        }
        setError(warnings.join('\n'));
    });
}
void Panel::updateSelection() {
    const int index = inbox_->currentRow(); const bool selected = index >= 0 && index < items_.size();
    acknowledge_->setEnabled(selected); snooze_->setEnabled(selected); complete_->setEnabled(selected);
    open_->setEnabled(selected && safeUrl(items_[index]["source_url"].toString()));
    detail_->setPlainText(selected ? items_[index]["title"].toString() + "\n\n" + items_[index]["detail"].toString() : QString());
}
void Panel::act(const QString &action) {
    const int index = inbox_->currentRow(); if (index < 0 || index >= items_.size()) return;
    const auto &item = items_[index];
    call_("act", {{"source_id", item["source_id"]}, {"event_id", item["event_id"]}, {"seq", item["seq"]}, {"action", action}, {"seconds", snoozeSeconds_->value()}}); refresh();
}
void Panel::timerAct(const QString &action) {
    if (!timers_->currentItem()) return;
    call_("timer_act", {{"id", timers_->currentItem()->data(Qt::UserRole).toString()}, {"action", action}}); refresh();
}
void Panel::createTimer(const QString &mode) {
    call_("timer_create", {{"label", timerLabel_->text().trimmed().isEmpty() ? (mode == "pomodoro" ? "Pomodoro" : "Timer") : timerLabel_->text().trimmed()}, {"mode", mode}, {"seconds", seconds_->value()}, {"break_seconds", breakSeconds_->value()}, {"long_break_seconds", longBreakSeconds_->value()}, {"rounds", rounds_->value()}}); refresh();
}
void Panel::openSource() {
    const int index = inbox_->currentRow(); if (index < 0 || index >= items_.size()) return;
    const auto snapshot = items_[index];
    for (int offset = 0; offset < 512;) {
        const auto page = call_("list", {{"offset", offset}, {"limit", 50}});
        const auto rows = page["items"].toArray(); if (rows.isEmpty()) break;
        for (const auto &value : rows) {
            const auto item = value.toObject(); if (identity(item) != identity(snapshot)) continue;
            if (item["seq"] != snapshot["seq"]) throw std::runtime_error("This event changed. Refresh before opening its source.");
            const QString url = item["source_url"].toString();
            if (!safeUrl(url)) throw std::runtime_error("This event has no supported source link.");
            if (!QDesktopServices::openUrl(QUrl(url, QUrl::StrictMode))) throw std::runtime_error("Could not open the source.");
            return;
        }
        offset += rows.size(); if (offset >= page["total"].toInt()) break;
    }
    throw std::runtime_error("This event is no longer available. Refresh the inbox.");
}
}
