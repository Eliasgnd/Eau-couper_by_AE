#pragma once

#include <QGraphicsScene>
#include <QList>
#include <QPolygonF>

// NOUVELLE STRUCTURE : Un chemin complet
struct ContinuousCut {
    QPolygonF points; // La liste des points qui forment la ligne continue
    bool isClosed;    // Vrai si le premier et le dernier point se touchent
};

class PathPlanner
{
public:
    // NOUVELLE FONCTION (remplace extractSegments)
    static QList<ContinuousCut> extractContinuousPaths(QGraphicsScene *sc);
};
