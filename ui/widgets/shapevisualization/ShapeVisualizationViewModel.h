#ifndef SHAPEVISUALIZATIONVIEWMODEL_H
#define SHAPEVISUALIZATIONVIEWMODEL_H

#include <QObject>
#include <QPolygonF>
#include <QList>

#include "ShapeModel.h"

class ShapeVisualizationViewModel : public QObject
{
    Q_OBJECT

public:
    explicit ShapeVisualizationViewModel(QObject *parent = nullptr);

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

    // --- État du mode d'affichage ---
    bool    isCustomMode()        const { return m_isCustomMode; }
    bool    isOptimizationRunning() const { return m_optimizationRunning; }
    QString customShapeName()     const { return m_customShapeName; }

    void setCustomMode(bool custom);
    void setOptimizationRunning(bool running);
    void setCustomShapeName(const QString &name);

signals:
    void dataChanged();
    void customModeChanged(bool custom);
    void optimizationRunningChanged(bool running);

private:
    ShapeModel::Type m_currentModel {ShapeModel::Type::Circle};
    int m_currentLargeur {0};
    int m_currentLongueur {0};
    int m_shapeCount {0};
    int m_spacing {0};
    QList<QPolygonF> m_customShapes;

    bool    m_isCustomMode        {false};
    bool    m_optimizationRunning {false};
    QString m_customShapeName;
};

#endif // SHAPEVISUALIZATIONVIEWMODEL_H
