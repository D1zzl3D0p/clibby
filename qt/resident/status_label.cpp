#include "status_label.h"
#include "spaces_policy.h"
#include "brand_theme.h"
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QSignalBlocker>
#include <QGuiApplication>
#include <QScreen>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QToolButton>
#include <QAction>
#include <QGridLayout>
#include <algorithm>
namespace pets {
UtilityCard::UtilityCard(QWidget* parent, Qt::WindowFlags flags) : QWidget(parent, flags) {
    setObjectName("utilityCard");
    auto* layout = new QVBoxLayout(this);
    brand::spacing(layout);
    state_ = new QLabel("Clibby · Attention", this); state_->setTextFormat(Qt::PlainText);
    state_->setProperty("clibbyRole", "eyebrow"); layout->addWidget(state_);
    label_ = new QLabel(this); label_->setTextFormat(Qt::PlainText); label_->setWordWrap(true);
    feedback_ = new QLabel(this); feedback_->setObjectName("actionFeedback"); feedback_->setTextFormat(Qt::PlainText); feedback_->setWordWrap(true); feedback_->hide();
    layout->addWidget(label_); layout->addWidget(feedback_);
    auto* buttons = new QHBoxLayout;
    open_ = new QPushButton("Open source", this); open_->setObjectName("openSource");
    acknowledge_ = new QPushButton("Acknowledge", this); acknowledge_->setObjectName("acknowledgeCard");
    acknowledge_->setProperty("clibbyPrimary", true);
    buttons->addWidget(open_); buttons->addWidget(acknowledge_); layout->addLayout(buttons);
    setFixedWidth(352);
    brand::install(this);
    connect(open_, &QPushButton::clicked, this, [this] { if (record_ && dispatch) setFeedback(dispatch(record_->key, record_->sequence, UtilityAction::OpenSource)); });
    connect(acknowledge_, &QPushButton::clicked, this, [this] { if (record_ && dispatch) setFeedback(dispatch(record_->key, record_->sequence, UtilityAction::Acknowledge)); });
}
QString UtilityCard::text() const { return label_->text(); }
void UtilityCard::setFeedback(const QString& message) { feedback_->setText(message); feedback_->setVisible(!message.isEmpty()); }
void UtilityCard::setRecord(const std::optional<SessionRecord>& record) {
    record_ = record;
    const auto value = record ? QString::fromStdString(record->key.sourceId) + " · " + QString::fromStdString(record->label) + " — " + sessionStatusName(record->status) + (record->acknowledged ? " (acknowledged)" : "") : QString();
    if (value != label_->text()) { label_->setText(value); setFeedback({}); }
    const auto status = record ? record->status : SessionStatus::Working;
    state_->setText(record ? QStringLiteral("Clibby · ") + sessionStatusName(status) : QStringLiteral("Clibby · Attention"));
    state_->setProperty("clibbyRole", status == SessionStatus::Failed ? "danger" : status == SessionStatus::Completed ? "success" : status == SessionStatus::NeedsInput ? "warning" : "eyebrow");
    state_->style()->unpolish(state_); state_->style()->polish(state_);
    open_->setEnabled(record && !record->sourceUrl.empty());
    acknowledge_->setEnabled(record && !record->acknowledged && record->status != SessionStatus::Working);
    setAccessibleName(value);
}
StatusLabel::StatusLabel() : UtilityCard(nullptr, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint) {
    setObjectName("sessionStatusLabel"); setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_MacAlwaysShowToolWindow);
}
void StatusLabel::showEvent(QShowEvent* event) {
    UtilityCard::showEvent(event);
    joinAllSpaces(windowHandle());
}
void StatusLabel::placeBeside(QPoint petPosition, QPoint headAnchor) {
    adjustSize();
    const QPoint anchor = petPosition + headAnchor;
    auto* screen = QGuiApplication::screenAt(anchor);
    if (!screen) screen = QGuiApplication::primaryScreen();
    QPoint target(anchor.x() - width()/2, anchor.y() - height() - 12);
    if (screen) {
        const auto area = screen->availableGeometry();
        if (target.y() < area.top()) target.setY(anchor.y() + 24);
        target.setX(std::clamp(target.x(), area.left(), std::max(area.left(), area.right()+1-width())));
        target.setY(std::clamp(target.y(), area.top(), std::max(area.top(), area.bottom()+1-height())));
    }
    if (pos() != target) move(target);
}
UtilityToolbox::UtilityToolbox() : QDialog(nullptr, Qt::Tool | Qt::WindowStaysOnTopHint) {
    setObjectName("utilityToolbox"); setWindowTitle("Clibby — Toolbox");
    setAttribute(Qt::WA_MacAlwaysShowToolWindow);
    auto* outer = new QVBoxLayout(this);
    brand::spacing(outer, 20); outer->setSpacing(16);
    outer->addWidget(brand::header(this, "Toolbox"));
    auto* scroll = new QScrollArea(this); scroll->setWidgetResizable(true);
    // Reserve the scrollbar lane before the first paint. Adding notification
    // cards must not narrow the viewport after Qt has sized the control grid.
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scroll->viewport()->setObjectName("toolboxViewport");
    auto* contents = new QWidget(scroll); contents->setObjectName("toolboxContents");
    auto* body = new QVBoxLayout(contents); brand::spacing(body, 4); body->setSpacing(16);
    body->setSizeConstraint(QLayout::SetMinAndMaxSize);
    controls_ = new QGridLayout; controls_->setSpacing(8); body->addLayout(controls_);
    auto* speedRow = new QHBoxLayout;
    auto* speedLabel = new QLabel("Travel speed", this); speedRow->addWidget(speedLabel);
    walkingSpeed_ = new QDoubleSpinBox(this); walkingSpeed_->setObjectName("walkingSpeed");
    walkingSpeed_->setRange(0.5, 3.0); walkingSpeed_->setSingleStep(0.25); walkingSpeed_->setDecimals(2);
    walkingSpeed_->setSuffix(" ×"); walkingSpeed_->setValue(2.0);
    walkingSpeed_->setToolTip("Multiplies the companion's natural walking pace. Longer legs cover more ground per step.");
    walkingSpeed_->setAccessibleName("Travel speed multiplier"); speedLabel->setBuddy(walkingSpeed_);
    speedRow->addStretch(); speedRow->addWidget(walkingSpeed_); body->addLayout(speedRow);
    connect(walkingSpeed_, &QDoubleSpinBox::valueChanged, this, [this](double value) {
        if (walkingSpeedChanged) walkingSpeedChanged(value);
    });
    auto* heading = new QLabel("Work notifications", contents); heading->setProperty("clibbyRole", "eyebrow"); body->addWidget(heading);
    rows_ = new QVBoxLayout; rows_->setSpacing(8); body->addLayout(rows_);
    scroll->setWidget(contents); outer->addWidget(scroll);
    auto* close = new QPushButton("Close", this); outer->addWidget(close);
    connect(close, &QPushButton::clicked, this, &QDialog::hide);
    setMinimumSize(420, 360); resize(440, 700);
    brand::install(this);
}
void UtilityToolbox::setWalkingSpeed(double value, bool enabled) {
    const QSignalBlocker blocker(walkingSpeed_);
    walkingSpeed_->setValue(value); walkingSpeed_->setEnabled(enabled);
}
void UtilityToolbox::addControl(QAction* action, const QString& name) {
    auto* button = new QToolButton(this);
    button->setObjectName(name); button->setDefaultAction(action);
    button->setToolButtonStyle(Qt::ToolButtonTextOnly);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    controls_->addWidget(button, controlCount_ / 2, controlCount_ % 2); ++controlCount_;
}
void UtilityToolbox::setRecords(const std::vector<SessionRecord>& records) {
    bool same = records.size() == records_.size();
    for (std::size_t i=0; same && i<records.size(); ++i) {
        const auto& a=records[i]; const auto& b=records_[i];
        same = a.key==b.key && a.label==b.label && a.status==b.status && a.acknowledged==b.acknowledged && a.sourceUrl==b.sourceUrl;
    }
    if (same && rows_->count()) {
        records_=records;
        for (std::size_t i=0; i<records.size(); ++i)
            if (auto* card=dynamic_cast<UtilityCard*>(rows_->itemAt(int(i))->widget())) card->setRecord(records[i]);
        return;
    }
    records_=records;
    while (auto* item=rows_->takeAt(0)) { if (item->widget()) { item->widget()->hide(); item->widget()->deleteLater(); } delete item; }
    if (records.empty()) {
        auto* empty = new QLabel("You're all caught up.\nNew work notifications will appear here.");
        empty->setWordWrap(true); empty->setProperty("clibbyRole", "muted"); empty->setContentsMargins(8, 12, 8, 12); rows_->addWidget(empty);
    }
    for (const auto& record : records) {
        auto* card = new UtilityCard;
        card->setRecord(record);
        card->dispatch = [this](SessionKey key, std::uint64_t sequence, UtilityAction action) {
            return dispatch ? dispatch(key, sequence, action) : QString();
        };
        rows_->addWidget(card);
    }
    rows_->addStretch();
}
}
