#pragma once

#include <QGraphicsScene>
#include <QPoint>
#include <QVector>
#include <QList>

/** Représentation d'un segment de coupe. */
struct Segment
{
    QPoint a, b;   //!< Extrémités en pixels
    double len;    //!< Longueur en pixels
};

/** Outils d’extraction / calcul de parcours eulérien. */
class PathPlanner
{
public:
    //! Extrait tous les segments présents dans la scène (fusion des côtés communs).
    static QList<Segment> extractSegments(QGraphicsScene *scene);

    //! Construit (heuristiquement) un chemin eulérien et renvoie la liste des points.
    static QVector<QPoint> buildEulerPath(const QList<Segment> &segs);
};
