#include "petitem.h"
#include <QQuickWindow>
#include <QSGImageNode>
#include <QSGTexture>
#include <QSGTransformNode>
#include <QMatrix4x4>

namespace {
// This entire tree, including its shared texture, is render-thread-owned.
class PetNode final : public QSGTransformNode {
public:
    QSGTexture *texture = nullptr;
    QVector<QSGTransformNode *> parts;
    ~PetNode() override {
        while (firstChild()) delete firstChild();
        delete texture;
    }
};
}
PetItem::PetItem(const Rig &rig, const QImage &atlas, bool rootInsideWindow, QQuickItem *parent)
    : QQuickItem(parent), rig_(rig), atlas_(atlas), pose_(Motion(rig).pose()), rootInsideWindow_(rootInsideWindow) {
    setFlag(ItemHasContents, true);
    setSize(QSizeF(256, 256));
}
void PetItem::setPose(const Pose &pose) { pose_ = pose; update(); }
QSGNode *PetItem::updatePaintNode(QSGNode *old, UpdatePaintNodeData *) {
    auto *root = static_cast<PetNode *>(old);
    if (!root) {
        root = new PetNode;
        root->texture = window()->createTextureFromImage(atlas_, QQuickWindow::TextureHasAlphaChannel);
        root->texture->setFiltering(QSGTexture::Linear);
        for (const auto &part : rig_.parts) {
            auto *transform = new QSGTransformNode;
            auto *image = window()->createImageNode();
            image->setTexture(root->texture);
            image->setOwnsTexture(false);
            image->setFiltering(QSGTexture::Linear);
            image->setRect(QRectF(-part.pivot, QSizeF(part.size.x(), part.size.y())));
            image->setSourceRect(part.rect);
            transform->appendChildNode(image);
            if (part.parent < 0) root->appendChildNode(transform);
            else root->parts[part.parent]->appendChildNode(transform);
            root->parts.append(transform);
        }
    }
    QMatrix4x4 rootMatrix;
    const auto offset = rig_.origin + (rootInsideWindow_ ? pose_.root : QPointF());
    rootMatrix.translate(offset.x(), offset.y()); root->setMatrix(rootMatrix);
    for (int i = 0; i < pose_.parts.size(); ++i) {
        const auto &part = pose_.parts[i];
        QMatrix4x4 matrix;
        matrix.translate(part.x, part.y);
        matrix.rotate(part.rotation * (180.0 / 3.14159265358979323846), 0, 0, 1);
        matrix.scale(part.scaleX, part.scaleY);
        root->parts[i]->setMatrix(matrix);
    }
    return root;
}
