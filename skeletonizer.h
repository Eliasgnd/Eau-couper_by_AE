#ifndef SKELETONIZER_H
#define SKELETONIZER_H

#include <QImage>
#include <QPainterPath>

/**
 *  Utilitaires simples :
 *    • thin()         → squelettisation Zhang‑Suen (image → image 1 px)
 *    • bitmapToPath() → binaire 1 px → QPainterPath vectoriel
 */
class Skeletonizer
{
public:
    static QImage        thin(const QImage &gray);          // entrée : niveau de gris ou binaire
    static QPainterPath  bitmapToPath(const QImage &binary); // noir = premier‑plan
};

#endif // SKELETONIZER_H
