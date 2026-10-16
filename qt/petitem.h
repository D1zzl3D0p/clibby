#pragma once
#include "motion.h"
#include <QImage>
#include <QQuickItem>

class PetItem final : public QQuickItem {
public:
    PetItem(const Rig &rig, const QImage &atlas, bool rootInsideWindow, QQuickItem *parent);
    void setPose(const Pose &pose);
protected:
    QSGNode *updatePaintNode(QSGNode *old, UpdatePaintNodeData *) override;
private:
    Rig rig_;
    QImage atlas_;
    Pose pose_;
    bool rootInsideWindow_;
};
