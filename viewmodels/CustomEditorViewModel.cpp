#include "CustomEditorViewModel.h"
#include "InventoryViewModel.h"
#include "LogoImporter.h"
#include "PathGenerator.h"
#include <cmath>
#include <QTransform>

CustomEditorViewModel::CustomEditorViewModel(InventoryViewModel &inventoryVm,
                                             QObject *parent)
    : QObject(parent)
    , m_inventoryVm(inventoryVm)
{
}

bool CustomEditorViewModel::shapeNameExists(const QString &name) const
{
    return m_inventoryVm.shapeNameExists(name);
}

void CustomEditorViewModel::saveShape(const QList<QPolygonF> &shapes,
                                      const QString &name)
{
    m_inventoryVm.addCustomShape(shapes, name);
    emit shapeSaved();
}

void CustomEditorViewModel::importLogo(const QString &filePath,
                                       bool includeInternal,
                                       QSizeF drawAreaSize)
{
    LogoImporter importer;
    QPainterPath outline = importer.importLogo(filePath, includeInternal, 128);
    if (outline.isEmpty()) {
        emit importFailed(tr("Le chemin importé est vide."));
        return;
    }

    QList<QPainterPath> subpaths = scaledAndCentered(outline, drawAreaSize);
    emit subpathsReady(subpaths);
}

void CustomEditorViewModel::importImageColor(const QString &filePath,
                                             QSizeF drawAreaSize)
{
    QPainterPath edge;
    if (!m_imageImporter.loadAndProcess(filePath, edge)) {
        emit importFailed(tr("Contour introuvable."));
        return;
    }

    QList<QPainterPath> subpaths = scaledAndCentered(edge, drawAreaSize);
    emit subpathsReady(subpaths);
}

QList<QPainterPath> CustomEditorViewModel::scaledAndCentered(
    const QPainterPath &outline, QSizeF drawAreaSize)
{
    QRectF br = outline.boundingRect();
    double maxDim = std::max(br.width(), br.height());
    if (maxDim < 0.0001)
        return {};

    double scaleFactor = 300.0 / maxDim;
    QTransform transform;
    transform.translate(-br.x(), -br.y());
    transform.scale(scaleFactor, scaleFactor);
    QPainterPath scaled = transform.map(outline);

    QPointF center(drawAreaSize.width() / 2.0, drawAreaSize.height() / 2.0);
    scaled.translate(center - scaled.boundingRect().center());

    return PathGenerator::separateIntoSubpaths(scaled);
}
