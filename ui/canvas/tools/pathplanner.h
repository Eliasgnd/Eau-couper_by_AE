#pragma once

#include <QGraphicsScene>
#include <QPoint>
#include <QList>

/** Représentation d'un segment de coupe simple. */
struct Segment
{
    QPoint a, b;   //!< Extrémités en pixels
    double len;    //!< Longueur en pixels
};

/** Outil d’extraction de parcours pour la découpe. */
class PathPlanner
{
public:
    //! Extrait les contours et les trous des formes dessinées sur la scène
    static QList<Segment> extractSegments(QGraphicsScene *scene);
};


