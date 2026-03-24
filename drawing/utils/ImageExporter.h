#pragma once

#include <QList>
#include <QPoint>

class QGraphicsScene;

class ImageExporter
{
public:
    static QList<QPoint> extractBlackPixels(const QGraphicsScene *scene);
};
