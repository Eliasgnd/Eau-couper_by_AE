#pragma once
#include <QGraphicsScene>
#include <QPoint>
#include <QVector>
#include <QList>

struct Segment {
    QPoint a, b;    // extrémités en px
    double len;     // longueur en px
};

class PathPlanner {
public:
    //! Extrait tous les segments des items + fusion des coupes communes
    static QList<Segment> extractSegments(QGraphicsScene* scene);

    //! Construit un chemin eulérien (Heuristique) et renvoie la liste de points
    static QVector<QPoint> buildEulerPath(const QList<Segment>& segs);
};
