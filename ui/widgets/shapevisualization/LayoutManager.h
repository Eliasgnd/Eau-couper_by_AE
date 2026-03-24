#pragma once

#include <QGraphicsScene>

#include "Inventory.h"

class LayoutManager
{
public:
    static void applyLayout(QGraphicsScene *scene,
                            const LayoutData &layout,
                            const QList<QPolygonF> &customShapes,
                            bool interactionEnabled);

    static LayoutData captureLayout(const QGraphicsScene *scene,
                                    const QString &name,
                                    int largeur,
                                    int longueur,
                                    int spacing,
                                    const QList<QGraphicsItem*> &excludedItems);
};
