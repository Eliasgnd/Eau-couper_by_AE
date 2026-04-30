#include "ShapeModel.h"
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QGraphicsPolygonItem>
#include <QPainterPath>
#include <cmath>
#include <QDebug>

QList<QGraphicsItem*> ShapeModel::generateShapes(Type type, int largeur, int longueur, int plateauWidth, int plateauHeight) {
    //qDebug() << "generateShapes appelé avec type:" << static_cast<int>(type)
    //         << " largeur:" << largeur << " longueur:" << longueur;

    QList<QGraphicsItem*> shapes;

    if (largeur <= 0 || longueur <= 0) {
        //qDebug() << "Erreur: dimensions invalides, largeur ou longueur <= 0";
        return shapes;
    }

    // Ici, on n'ajoute pas de marge additionnelle
    int espacementX = largeur;   // espacement horizontal égal à la largeur
    int espacementY = longueur;  // espacement vertical égal à la longueur

    // On démarre exactement en haut à gauche (0,0)
    int startX = 0;
    int startY = 0;

    // Calculer le nombre maximal de colonnes et de rangées pour éviter les débordements
    int maxCols = (plateauWidth - espacementX) / espacementX;
    int maxRows = (plateauHeight - espacementY) / espacementY;

    // Forcer au moins 1 colonne et 1 rangée si la forme est trop grande
    if(maxCols < 1) maxCols = 1;
    if(maxRows < 1) maxRows = 1;

    for (int i = 0; i < maxRows; ++i) {
        for (int j = 0; j < maxCols; ++j) {
            QGraphicsItem *item = nullptr;

            switch (type) {
            case Type::Circle:
                item = new QGraphicsEllipseItem(startX + j * espacementX, startY + i * espacementY, largeur, longueur);
                break;

            case Type::Rectangle:
                item = new QGraphicsRectItem(startX + j * espacementX, startY + i * espacementY, largeur, longueur);
                break;

            case Type::Triangle: {
                QPolygonF triangle;
                triangle << QPointF(startX + j * espacementX, startY + i * espacementY + longueur)
                         << QPointF(startX + j * espacementX + largeur / 2, startY + i * espacementY)
                         << QPointF(startX + j * espacementX + largeur, startY + i * espacementY + longueur);
                item = new QGraphicsPolygonItem(triangle);
                break;
            }

            case Type::Star: {
                // 1) Construction "brute" d’une étoile normalisée dans un repère (ex: [-1,1]x[-1,1])
                //    Pour simplifier, on peut se baser sur un rayon = 1, et 5 branches.
                QPolygonF rawStar;
                const int branches = 5;   // 5 branches
                for (int k = 0; k < branches; ++k) {
                    // angle pour la pointe "extérieure"
                    qreal outerAngle = (k * 2.0 * M_PI / branches);
                    // pointe extérieure (rayon 1)
                    rawStar << QPointF(std::cos(outerAngle), std::sin(outerAngle));
                    // angle pour la pointe "intérieure" (milieu entre deux pointes extérieures)
                    qreal innerAngle = outerAngle + M_PI / branches;
                    // pointe intérieure (rayon 0.5)
                    rawStar << QPointF(std::cos(innerAngle) * 0.5, std::sin(innerAngle) * 0.5);
                }

                // 2) Récupérer le boundingRect "brut" de cette étoile normalisée
                QRectF normBounds = rawStar.boundingRect();

                // 3) Calculer les facteurs d’échelle pour occuper exactement [0..largeur]x[0..longueur]
                qreal scaleX = largeur / normBounds.width();
                qreal scaleY = longueur / normBounds.height();

                // 4) Construire un polygone "scale" qui occupera [0..largeur]x[0..longueur]
                QPolygonF scaledStar;
                for (const QPointF &p : rawStar) {
                    // translation du repère normalisé pour partir à (0,0)
                    qreal px = p.x() - normBounds.x();
                    qreal py = p.y() - normBounds.y();
                    // mise à l’échelle
                    scaledStar << QPointF(px * scaleX, py * scaleY);
                }

                // 5) Placer l’étoile dans la grille à (startX + j*espacementX, startY + i*espacementY)
                QPolygonF finalStar;
                for (const QPointF &p : scaledStar) {
                    finalStar << QPointF(startX + j * espacementX + p.x(),
                                         startY + i * espacementY + p.y());
                }

                item = new QGraphicsPolygonItem(finalStar);
                break;
            }

            case Type::Heart: {
                // Générer un polygone représentant un cœur
                QPolygonF rawHeart;
                for (int k = 0; k < 360; ++k) {
                    qreal angle = k * M_PI / 180.0;
                    qreal x = 16 * pow(sin(angle), 3);
                    qreal y = 13 * cos(angle) - 5 * cos(2 * angle) - 2 * cos(3 * angle) - cos(4 * angle);
                    rawHeart << QPointF(x, y);
                }
                QRectF bounds = rawHeart.boundingRect();
                QPolygonF normalizedHeart;
                for (const QPointF &p : rawHeart) {
                    normalizedHeart << QPointF(p.x() - bounds.x(), p.y() - bounds.y());
                }
                qreal scaleX = largeur / bounds.width();
                qreal scaleY = longueur / bounds.height();
                QPolygonF scaledHeart;
                for (const QPointF &p : normalizedHeart) {
                    scaledHeart << QPointF(p.x() * scaleX, p.y() * scaleY);
                }
                QPolygonF finalHeart;
                for (const QPointF &p : scaledHeart) {
                    finalHeart << QPointF(startX + j * espacementX + p.x(),
                                          startY + i * espacementY + p.y());
                }
                item = new QGraphicsPolygonItem(finalHeart);
                break;
            }

            default:
                break;
            }

            if (item) {
                shapes.append(item);
            }
        }
    }

    return shapes;
}

QList<QPolygonF> ShapeModel::shapePolygons(Type type, int largeur, int longueur)
{
    QList<QPolygonF> polys;
    switch (type) {
    case Type::Circle: {
        QPainterPath p; p.addEllipse(0,0,largeur,longueur);
        polys.append(p.toFillPolygon());
        break; }
    case Type::Rectangle: {
        QPolygonF r; r << QPointF(0,0) << QPointF(largeur,0)
                       << QPointF(largeur,longueur) << QPointF(0,longueur);
        polys.append(r);
        break; }
    case Type::Triangle: {
        QPolygonF tri; tri << QPointF(0,longueur)
                           << QPointF(largeur/2.0,0)
                           << QPointF(largeur,longueur);
        polys.append(tri);
        break; }
    case Type::Star: {
        QPolygonF rawStar; const int branches = 5;
        for (int k=0;k<branches;++k){
            qreal outerAngle = (k*2.0*M_PI/branches);
            rawStar << QPointF(std::cos(outerAngle), std::sin(outerAngle));
            qreal innerAngle = outerAngle + M_PI/branches;
            rawStar << QPointF(std::cos(innerAngle)*0.5, std::sin(innerAngle)*0.5);
        }
        QRectF norm = rawStar.boundingRect();
        qreal scaleX = largeur / norm.width();
        qreal scaleY = longueur / norm.height();
        QPolygonF finalStar;
        for (const QPointF &p : rawStar)
            finalStar << QPointF((p.x()-norm.x())*scaleX,
                                 (p.y()-norm.y())*scaleY);
        polys.append(finalStar);
        break; }
    case Type::Heart: {
        QPolygonF rawHeart;
        for (int k=0;k<360;++k){
            qreal angle = k * M_PI / 180.0;
            qreal x = 16*pow(sin(angle),3);
            qreal y = 13*cos(angle) -5*cos(2*angle) -2*cos(3*angle) - cos(4*angle);
            rawHeart << QPointF(x,y);
        }
        QRectF b = rawHeart.boundingRect();
        qreal scaleX = largeur / b.width();
        qreal scaleY = longueur / b.height();
        QPolygonF finalHeart;
        for(const QPointF &p: rawHeart)
            finalHeart << QPointF((p.x()-b.x())*scaleX,
                                  (p.y()-b.y())*scaleY);
        polys.append(finalHeart);
        break; }
    }
    return polys;
}
