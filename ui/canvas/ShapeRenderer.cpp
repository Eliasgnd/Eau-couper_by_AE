#include "ShapeRenderer.h"

#include "ShapeManager.h"

#include <QPainter>
#include <QPen>
#include <QtGlobal>

#include <vector>

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

void ShapeRenderer::render(QPainter &painter, const ShapeManager &shapeManager,
                           const QRectF &visibleArea, RenderQuality quality) const
{
    if (m_showGrid) {
        drawGrid(painter, visibleArea);
    }

    drawShapes(painter, shapeManager, visibleArea, quality);
    drawSelectionHandles(painter, shapeManager);
}

void ShapeRenderer::drawShapes(QPainter &painter, const ShapeManager &shapeManager,
                               const QRectF &visibleArea, RenderQuality quality) const
{
    QPen pen(Qt::black, 2);
    pen.setCosmetic(true);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);

    painter.save();
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    const bool preferProxy = quality == RenderQuality::Interactive;
    const std::vector<int> visible = shapeManager.visibleShapeIndices(visibleArea, 12.0);
    const auto &shapes = shapeManager.shapes();
    for (int idx : visible) {
        if (idx < 0 || idx >= static_cast<int>(shapes.size()))
            continue;
        painter.drawPath(shapes[idx].renderPath(preferProxy));
    }
    painter.restore();
}

void ShapeRenderer::drawSelectionHandles(QPainter &painter, const ShapeManager &shapeManager) const
{
    Q_UNUSED(painter)
    Q_UNUSED(shapeManager)
    return;

    painter.save();
    painter.setPen(QPen(QColor(43, 122, 255), 2));
    painter.setBrush(QColor(43, 122, 255));

    for (int index : shapeManager.selectedShapes()) {
        if (index < 0 || index >= shapeManager.shapes().size()) continue;
        const QRectF b = shapeManager.shapes().at(index).path.boundingRect();
        painter.fillRect(b.adjusted(-6, -6, 6, 6), QColor(43, 122, 255, 18));
        painter.drawRect(b);
        const qreal h = 9.0;
        painter.drawEllipse(b.topLeft(), h, h);
        painter.drawEllipse(b.topRight(), h, h);
        painter.drawEllipse(b.bottomLeft(), h, h);
        painter.drawEllipse(b.bottomRight(), h, h);
    }

    painter.restore();
}

void ShapeRenderer::drawGrid(QPainter &painter, const QRectF &visibleArea) const
{
    // 1. Création d'une texture de cellule unique si besoin
    static QPixmap tile(m_gridSpacing, m_gridSpacing);
    static int lastSpacing = -1;

    if (m_gridSpacing != lastSpacing) {
        tile = QPixmap(m_gridSpacing, m_gridSpacing);
        tile.fill(Qt::transparent);
        QPainter p(&tile);
        p.setPen(QPen(QColor(230, 230, 230), 1));
        p.drawLine(0, 0, m_gridSpacing, 0); // Ligne horizontale
        p.drawLine(0, 0, 0, m_gridSpacing); // Ligne verticale
        lastSpacing = m_gridSpacing;
    }

    // 2. Utilisation de la texture répétée (très rapide)
    painter.save();
    painter.drawTiledPixmap(visibleArea, tile, visibleArea.topLeft());
    painter.restore();
}
