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
    /**
     *  Utilitaires de lissage et de simplification de chemins squelette
     *
     */
    static QImage        thin(const QImage &gray);          // entrée : niveau de gris ou binaire
    static QPainterPath  bitmapToPath(const QImage &binary); // noir = premier‑plan

    static QPainterPath  smoothPath(const QPainterPath &path, int chaikinPasses = 3, double epsilon = 1.0);
};

#endif // SKELETONIZER_H
