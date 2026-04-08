#include "CustomEditorViewModel.h"
#include "InventoryViewModel.h"
#include "Language.h"
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

QStringList CustomEditorViewModel::getAllShapeNames() const
{
    return m_inventoryVm.getAllShapeNames(Language::French);
}

void CustomEditorViewModel::saveShape(const QList<QPolygonF> &shapes,
                                      const QString &name)
{
    // (Optionnel) Vérification métier : le nom existe-t-il ?
    if (shapeNameExists(name)) {
        // Gérer l'erreur si nécessaire, par exemple émettre un signal d'erreur
        // return;
    }

    // --- Étape de simplification (Ramer-Douglas-Peucker via Qt) ---
    QList<QPolygonF> optimizedShapes;
    for (const QPolygonF &poly : shapes) {
        QPainterPath path;
        path.addPolygon(poly);

        // simplified() fusionne, nettoie les anomalies et allège la géométrie
        QPainterPath cleanPath = path.simplified();

        optimizedShapes.append(cleanPath.toFillPolygon());
    }

    // CORRECTION : Appel de addCustomShape (et non saveCustomShape)
    m_inventoryVm.addCustomShape(optimizedShapes, name);
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
    // CORRECTION : Accès aux paramètres via la méthode publique params()
    m_imageImporter.params().epsilon_percent = 0.001; // permet de changer la qualité de l'image importer
    m_imageImporter.params().final_simplify = true;

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
