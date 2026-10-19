#pragma once

#include "brand_assets.h"

#include <QApplication>
#include <QEvent>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QPalette>
#include <QPixmap>
#include <QStyle>
#include <QVBoxLayout>
#include <QWidget>

// A local widget theme: the desktop pet and the application's native palette
// remain untouched. System appearance changes are applied without a timer.
namespace pets::brand {
struct Colors {
    QString background, surface, ink, muted, border, hover, danger, success, warning;
};
inline bool darkAppearance() { return QApplication::palette().color(QPalette::Window).lightnessF() < 0.5; }
inline Colors colors() {
    if (darkAppearance())
        return {"#292638", "#343143", "#F7F4EE", "#C6C0D4", "#575166", "#454054", "#F3AAAA", "#A5D4B7", "#E6CA91"};
    return {"#F7F4EE", "#FFFDFA", "#292638", "#686174", "#DCD5E2", "#EDE7F6", "#A13949", "#356B4C", "#866019"};
}
inline void spacing(QLayout *layout, int margin = 16) {
    layout->setContentsMargins(margin, margin, margin, margin);
    layout->setSpacing(8);
}
inline void apply(QWidget *widget) {
    branding::initialize();
    const auto c = colors();
    QPalette palette = QApplication::palette();
    palette.setColor(QPalette::Window, QColor(c.background));
    palette.setColor(QPalette::WindowText, QColor(c.ink));
    palette.setColor(QPalette::Base, QColor(c.surface));
    palette.setColor(QPalette::AlternateBase, QColor(c.background));
    palette.setColor(QPalette::Text, QColor(c.ink));
    palette.setColor(QPalette::Button, QColor(c.surface));
    palette.setColor(QPalette::ButtonText, QColor(c.ink));
    palette.setColor(QPalette::Highlight, QColor("#B9ADEB"));
    palette.setColor(QPalette::HighlightedText, QColor("#292638"));
    palette.setColor(QPalette::PlaceholderText, QColor(c.muted));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(c.muted));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(c.muted));
    widget->setPalette(palette);
    widget->setAutoFillBackground(true);
    widget->setStyleSheet(QString(R"(
        QWidget#productivityPanel, QWidget#utilityToolbox, QWidget#utilityCard,
        QWidget#sessionStatusLabel, QWidget#toolboxContents, QWidget#toolboxViewport { background: %1; color: %3; }
        QWidget#utilityCard, QWidget#sessionStatusLabel { border: 1px solid %5; border-radius: 12px; }
        QLabel { color: %3; background: transparent; border: none; }
        QLabel[clibbyRole="muted"] { color: %4; }
        QLabel[clibbyRole="eyebrow"] { color: %4; font-weight: 600; }
        QLabel[clibbyRole="danger"], QLabel#productivityError, QLabel#actionFeedback {
            color: %7; padding: 8px; border: 1px solid %5; border-radius: 8px;
        }
        QLabel[clibbyRole="success"] { color: %8; }
        QLabel[clibbyRole="warning"] { color: %9; }
        QPushButton, QToolButton { color: %3; background: %2; border: 1px solid %5;
            border-radius: 8px; padding: 6px 12px; min-height: 20px; }
        QToolButton { text-align: left; }
        QPushButton:hover, QToolButton:hover { background: %6; }
        QPushButton:pressed, QToolButton:pressed, QToolButton:checked,
        QPushButton[clibbyPrimary="true"] { background: #B9ADEB; color: #292638; border-color: #B9ADEB; }
        QPushButton:disabled, QToolButton:disabled { color: %4; background: %1; }
        QPushButton:focus, QToolButton:focus { border: 2px solid #B9ADEB; padding: 5px 11px; }
        QLineEdit, QTextEdit, QSpinBox, QDoubleSpinBox, QListWidget {
            background: %2; color: %3; border: 1px solid %5; border-radius: 8px;
            selection-background-color: #B9ADEB; selection-color: #292638;
        }
        QLineEdit, QSpinBox, QDoubleSpinBox { padding: 6px 8px; min-height: 20px; }
        QLineEdit:focus, QTextEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QListWidget:focus { border-color: #B9ADEB; }
        QTextEdit { padding: 8px; }
        QListWidget { padding: 4px; outline: none; }
        QListWidget::item { padding: 10px 8px; border-radius: 6px; }
        QListWidget::item:selected { background: #B9ADEB; color: #292638; }
        QListWidget::item:hover:!selected { background: %6; }
        QTabWidget::pane { border: none; background: %1; }
        QTabBar::tab { color: %4; background: transparent; border: none; border-bottom: 2px solid transparent;
            padding: 10px 12px; margin-right: 8px; }
        QTabBar::tab:selected { color: %3; border-bottom-color: #B9ADEB; }
        QScrollArea { border: none; background: transparent; }
        QScrollBar:vertical { background: %2; width: 12px; margin: 2px 0; }
        QScrollBar:horizontal { background: %2; height: 12px; margin: 0 2px; }
        QScrollBar::handle { background: %4; border: 3px solid %2; border-radius: 6px; }
        QScrollBar::handle:vertical { min-height: 24px; }
        QScrollBar::handle:horizontal { min-width: 24px; }
        QScrollBar::handle:hover { background: #B9ADEB; }
        QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; border: none; }
        QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }
        QCheckBox { color: %3; spacing: 8px; }
    )").arg(c.background, c.surface, c.ink, c.muted, c.border, c.hover, c.danger, c.success, c.warning));
    for (auto *mark : widget->findChildren<QLabel *>("clibbyMark")) {
        QPixmap image(darkAppearance() ? ":/clibby/mark-reversed.png" : ":/clibby/mark-256.png");
        if (!image.isNull()) {
            const qreal scale = widget->devicePixelRatioF();
            image = image.scaled(qRound(32 * scale), qRound(32 * scale), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            image.setDevicePixelRatio(scale);
            mark->setPixmap(image);
        }
    }
    // ApplicationPaletteChange is delivered to windows. Cards moved into the
    // toolbox keep a scoped palette of their own, so refresh those descendants
    // together with the containing window.
    for (auto *card : widget->findChildren<QWidget *>("utilityCard")) apply(card);
}
class AppearanceObserver final : public QObject {
public:
    explicit AppearanceObserver(QWidget *target) : QObject(target), target_(target) { target->installEventFilter(this); }
protected:
    bool eventFilter(QObject *object, QEvent *event) override {
        if (object == target_ && event->type() == QEvent::ApplicationPaletteChange) apply(target_);
        return QObject::eventFilter(object, event);
    }
private:
    QWidget *target_;
};
inline void install(QWidget *widget) {
    new AppearanceObserver(widget);
    apply(widget);
}
inline QWidget *header(QWidget *parent, const QString &section, const QString &description = {}) {
    auto *container = new QWidget(parent);
    auto *row = new QHBoxLayout(container); spacing(row, 0); row->setSpacing(12);
    auto *mark = new QLabel(container); mark->setObjectName("clibbyMark");
    mark->setFixedSize(32, 32); mark->setAccessibleName("Clibby"); row->addWidget(mark, 0, Qt::AlignTop);
    auto *text = new QVBoxLayout; spacing(text, 0); text->setSpacing(2);
    auto *title = new QLabel("Clibby", container); title->setTextFormat(Qt::PlainText);
    auto titleFont = title->font(); titleFont.setPointSizeF(titleFont.pointSizeF() + 4); titleFont.setWeight(QFont::DemiBold); title->setFont(titleFont);
    text->addWidget(title);
    auto *subtitle = new QLabel(section, container); subtitle->setTextFormat(Qt::PlainText); subtitle->setProperty("clibbyRole", "muted"); text->addWidget(subtitle);
    row->addLayout(text, 1);
    if (!description.isEmpty()) {
        auto *hint = new QLabel(description, container); hint->setTextFormat(Qt::PlainText); hint->setWordWrap(true);
        hint->setProperty("clibbyRole", "muted"); row->addWidget(hint, 1);
    }
    return container;
}
}
