#ifndef SHAPEPROJECTMODEL_H
#define SHAPEPROJECTMODEL_H

#include <QObject>
#include <QPolygonF>
#include <QList>

#include "ShapeModel.h"

class ShapeProjectModel : public QObject
{
    Q_OBJECT

public:
    explicit ShapeProjectModel(QObject *parent = nullptr);

    ShapeModel::Type currentModel() const;
    int currentLargeur() const;
    int currentLongueur() const;
    int shapeCount() const;
    int spacing() const;
    QList<QPolygonF> customShapes() const;

    void setCurrentModel(ShapeModel::Type model);
    void setDimensions(int largeur, int longueur);
    void setShapeCount(int count);
    void setSpacing(int spacing);
    void setCustomShapes(const QList<QPolygonF> &shapes);
    void clearCustomShapes();

signals:
    void dataChanged();

private:
    ShapeModel::Type m_currentModel {ShapeModel::Type::Circle};
    int m_currentLargeur {0};
    int m_currentLongueur {0};
    int m_shapeCount {0};
    int m_spacing {0};
    QList<QPolygonF> m_customShapes;
};

#endif // SHAPEPROJECTMODEL_H
