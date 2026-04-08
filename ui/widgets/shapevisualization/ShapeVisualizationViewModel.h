#ifndef SHAPEVISUALIZATIONVIEWMODEL_H
#define SHAPEVISUALIZATIONVIEWMODEL_H

#include <QObject>
#include <QPolygonF>
#include <QPainterPath>
#include <QList>

class ShapeVisualizationViewModel : public QObject
{
    Q_OBJECT

public:
    explicit ShapeVisualizationViewModel(QObject *parent = nullptr);

    int currentModel() const;
    int currentLargeur() const;
    int currentLongueur() const;
    int shapeCount() const;
    int spacing() const;
    QList<QPolygonF> customShapes() const;

    void setCurrentModel(int model);
    void setDimensions(int largeur, int longueur);
    void setShapeCount(int count);
    void setSpacing(int spacing);
    // Mise à jour groupée — émet dataChanged() une seule fois
    void setShapeConfig(int model, int largeur, int longueur, int count);
    void setCustomShapes(const QList<QPolygonF> &shapes);
    void clearCustomShapes();

    // --- État du mode d'affichage ---
    bool    isCustomMode()        const { return m_isCustomMode; }
    bool    isOptimizationRunning() const { return m_optimizationRunning; }
    QString customShapeName()     const { return m_customShapeName; }

    void setCustomMode(bool custom);
    void setOptimizationRunning(bool running);
    void setCustomShapeName(const QString &name);

    // Retourne le QPainterPath de la forme prototype (utilisé par la View pour le rendu)
    // En mode prédéfini : calcule via ShapeModel. En mode custom : utilise les formes custom.
    QPainterPath prototypeShapePath() const;

signals:
    void dataChanged();
    void customModeChanged(bool custom);
    void optimizationRunningChanged(bool running);

private:
    int m_currentModel {0};
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
