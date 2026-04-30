#ifndef SHAPEMODEL_H
#define SHAPEMODEL_H

#include <QGraphicsItem>
#include <QPolygonF>
#include <QList>

// Classe représentant un modèle de formes géométriques
class ShapeModel {
public:
    // Enumération des types de formes disponibles
    enum class Type { Circle, Rectangle, Triangle, Star, Heart };

    // Génère une liste d'éléments graphiques correspondant au type de forme sélectionné
    // - type : Type de la forme à générer
    // - largeur : Largeur de la forme
    // - longueur : Longueur de la forme
    // - plateauWidth : Largeur du plateau (par défaut 600)
    // - plateauHeight : Longueur du plateau (par défaut 400)
    static QList<QGraphicsItem*> generateShapes(Type type, int largeur, int longueur, int plateauWidth = 600, int plateauHeight = 400);
    static QList<QPolygonF> shapePolygons(Type type, int largeur, int longueur);
};

#endif // SHAPEMODEL_H
