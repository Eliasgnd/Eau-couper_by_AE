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
    if (m_showGrid) {
        drawGrid(painter, painter.clipBoundingRect());
    }

    drawShapes(painter, shapeManager);
    drawSelectionHandles(painter, shapeManager);
}

void ShapeRenderer::drawGrid(QPainter &painter, const QRectF &visibleArea) const
{
    const QRect clip = visibleArea.toAlignedRect();
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

void ShapeRenderer::drawShapes(QPainter &painter, const ShapeManager &shapeManager) const
{
    QPen pen(Qt::black, 2);
    pen.setCosmetic(true);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);

    painter.save();
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    for (const auto &shape : shapeManager.shapes()) {
        painter.drawPath(shape.path);
    }
    painter.restore();
}

void ShapeRenderer::drawSelectionHandles(QPainter &painter, const ShapeManager &shapeManager) const
{
    painter.save();
    painter.setPen(QPen(QColor(43, 122, 255), 1));
    painter.setBrush(QColor(43, 122, 255));

    for (int index : shapeManager.selectedShapes()) {
        if (index < 0 || index >= shapeManager.shapes().size()) continue;
        const QRectF b = shapeManager.shapes().at(index).path.boundingRect();
        painter.drawRect(b);
        const qreal h = 3.0;
        painter.drawEllipse(b.topLeft(), h, h);
        painter.drawEllipse(b.topRight(), h, h);
        painter.drawEllipse(b.bottomLeft(), h, h);
        painter.drawEllipse(b.bottomRight(), h, h);
    }

    painter.restore();
}
