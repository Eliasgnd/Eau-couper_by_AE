#include "ImageExporter.h"

#include <QGraphicsScene>
#include <QImage>
#include <QPainter>

QList<QPoint> ImageExporter::extractBlackPixels(QGraphicsScene *scene)
{
    QList<QPoint> blackPixels;
    if (!scene)
        return blackPixels;

    QImage image(scene->sceneRect().size().toSize(), QImage::Format_RGB32);
    image.fill(Qt::white);

    QPainter painter(&image);
    // Maintenant, scene n'est plus const, l'appel est autorisé
    scene->render(&painter);

    for (int x = 0; x < image.width(); ++x) {
        for (int y = 0; y < image.height(); ++y) {
            if (image.pixelColor(x, y) == Qt::black)
                blackPixels.append(QPoint(x, y));
        }
    }

    return blackPixels;
}
