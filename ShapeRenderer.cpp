#include "ShapeRenderer.h"
#include "ShapeManager.h"

#include <QPainter>
#include <QPen>
#include <QtGlobal>

ShapeRenderer::ShapeRenderer(QObject *parent)
    : QObject(parent)
{
}

void ShapeRenderer::setGridVisible(bool visible) { m_showGrid = visible; }
bool ShapeRenderer::isGridVisible() const { return m_showGrid; }
void ShapeRenderer::setSnapToGridEnabled(bool enabled) { m_snapToGrid = enabled; }
bool ShapeRenderer::isSnapToGridEnabled() const { return m_snapToGrid; }
void ShapeRenderer::setGridSpacing(int spacing) { m_gridSpacing = qMax(1, spacing); }
int ShapeRenderer::gridSpacing() const { return m_gridSpacing; }

void ShapeRenderer::render(QPainter &painter, const ShapeManager &shapeManager) const
{
    if (m_showGrid) drawGrid(painter);

    QPen pen(Qt::black, 2);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    for (const auto &shape : shapeManager.shapes()) {
        painter.drawPath(shape.path);
    }
}

void ShapeRenderer::drawGrid(QPainter &painter) const
{
    const QRect clip = painter.clipBoundingRect().toAlignedRect();
    painter.save();
    painter.setPen(QPen(QColor(230, 230, 230), 1));
    for (int x = clip.left(); x <= clip.right(); x += m_gridSpacing) {
        painter.drawLine(x, clip.top(), x, clip.bottom());
    }
    for (int y = clip.top(); y <= clip.bottom(); y += m_gridSpacing) {
        painter.drawLine(clip.left(), y, clip.right(), y);
    }
    painter.restore();
}
