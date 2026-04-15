// Dans pathplanner.h
#pragma once
#include <QGraphicsScene>
#include <QList>
#include <QPolygonF>

struct ContinuousCut {
    QPolygonF points;
    bool isClosed;
    int depth = 0; // NOUVEAU : 0 = Contour externe, 1 = Trou intérieur, 2 = Trou dans un trou...
};

class PathPlanner {
public:
    static QList<ContinuousCut> extractContinuousPaths(QGraphicsScene *sc);
};
