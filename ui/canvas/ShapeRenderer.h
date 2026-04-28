#ifndef SHAPERENDERER_H
#define SHAPERENDERER_H

#include <QObject>
#include <QRectF>
#include <QPixmap>

class QPainter;
class ShapeManager;

class ShapeRenderer : public QObject
{
    Q_OBJECT
public:
    enum class RenderQuality {
        Full,
        Interactive
    };

    explicit ShapeRenderer(QObject *parent = nullptr);

    void setGridVisible(bool visible);
    bool isGridVisible() const;
    void setSnapToGridEnabled(bool enabled);
    bool isSnapToGridEnabled() const;
    void setGridSpacing(int spacing);
    int gridSpacing() const;

    void render(QPainter &painter, const ShapeManager &shapeManager,
                const QRectF &visibleArea, RenderQuality quality) const;

private:
    void drawGrid(QPainter &painter, const QRectF &visibleArea) const;
    void drawShapes(QPainter &painter, const ShapeManager &shapeManager,
                    const QRectF &visibleArea, RenderQuality quality) const;
    void drawSelectionHandles(QPainter &painter, const ShapeManager &shapeManager) const;

    mutable QPixmap m_gridCache; // Cache pour la texture de la grille
    void updateGridCache() const;
    bool m_showGrid = true;
    bool m_snapToGrid = false;
    int m_gridSpacing = 20;
};

#endif // SHAPERENDERER_H
