#include "LayoutManager.h"

#include <QAbstractGraphicsShapeItem>
#include <QGraphicsPathItem>
#include <QPen>
#include <QTransform>

void LayoutManager::applyLayout(QGraphicsScene *scene,
                                const LayoutData &layout,
                                const QList<QPolygonF> &customShapes,
                                bool interactionEnabled)
{
    if (!scene)
        return;

    scene->clear();

    QPainterPath combinedPath;
    for (const QPolygonF &poly : customShapes)
        combinedPath.addPolygon(poly);

    const QRectF bounds = combinedPath.boundingRect();
    const qreal scaleX = (bounds.width() > 0) ? static_cast<qreal>(layout.largeur) / bounds.width() : 1.0;
    const qreal scaleY = (bounds.height() > 0) ? static_cast<qreal>(layout.longueur) / bounds.height() : 1.0;

    QTransform scale;
    scale.scale(scaleX, scaleY);
    const QPainterPath scaledPath = scale.map(combinedPath);

    for (const LayoutItem &li : layout.items) {
        auto *item = new QGraphicsPathItem(scaledPath);
        item->setPen(QPen(Qt::black, 1));
        item->setBrush(Qt::NoBrush);
        item->setFlag(QGraphicsItem::ItemIsMovable, interactionEnabled);
        item->setFlag(QGraphicsItem::ItemIsSelectable, interactionEnabled);
        item->setTransformOriginPoint(item->boundingRect().center());

        const QRectF itemBounds = item->boundingRect();
        const QPointF offset(-itemBounds.x(), -itemBounds.y());
        item->setRotation(li.rotation);
        item->setPos(li.x + offset.x(), li.y + offset.y());
        scene->addItem(item);
        item->setSelected(false);
    }

    scene->clearSelection();
}

LayoutData LayoutManager::captureLayout(const QGraphicsScene *scene,
                                        const QString &name,
                                        int largeur,
                                        int longueur,
                                        int spacing,
                                        const QList<QGraphicsItem*> &excludedItems)
{
    LayoutData layout;
    layout.name = name;
    layout.largeur = largeur;
    layout.longueur = longueur;
    layout.spacing = spacing;

    if (!scene)
        return layout;

    for (QGraphicsItem *item : scene->items()) {
        if (excludedItems.contains(item))
            continue;

        if (auto shape = dynamic_cast<QAbstractGraphicsShapeItem*>(item)) {
            const QRectF bounds = shape->boundingRect();
            const QPointF offset(-bounds.x(), -bounds.y());

            LayoutItem li;
            li.x = shape->pos().x() - offset.x();
            li.y = shape->pos().y() - offset.y();
            li.rotation = shape->rotation();
            layout.items.append(li);
        }
    }

    return layout;
}
