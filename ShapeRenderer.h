#ifndef SHAPERENDERER_H
#define SHAPERENDERER_H

#include <QObject>

class QPainter;
class ShapeManager;

class ShapeRenderer : public QObject
{
    Q_OBJECT
public:
    explicit ShapeRenderer(QObject *parent = nullptr);

    void setGridVisible(bool visible);
    bool isGridVisible() const;
    void setSnapToGridEnabled(bool enabled);
    bool isSnapToGridEnabled() const;
    void setGridSpacing(int spacing);
    int gridSpacing() const;

    void render(QPainter &painter, const ShapeManager &shapeManager) const;

private:
    void drawGrid(QPainter &painter) const;

    bool m_showGrid = true;
    bool m_snapToGrid = false;
    int m_gridSpacing = 20;
};

#endif // SHAPERENDERER_H
