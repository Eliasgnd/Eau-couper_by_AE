#include "CustomEditorViewModel.h"
#include "InventoryViewModel.h"
#include "Language.h"
#include "LogoImporter.h"
#include "PathGenerator.h"
#include <algorithm>
#include <cmath>
#include <QLineF>
#include <QTransform>

namespace {
QPainterPath smoothImportedOutline(const QPainterPath &input)
{
    if (input.isEmpty())
        return {};

    QPainterPath result;
    const QList<QPolygonF> polygons = input.toSubpathPolygons();
    for (const QPolygonF &poly : polygons) {
        if (poly.size() < 3)
            continue;

        QList<QPointF> pts = poly.toList();
        if (pts.size() >= 2 && QLineF(pts.first(), pts.last()).length() < 0.5)
            pts.removeLast();
        if (pts.size() < 3)
            continue;

        // Lissage progressif borne: Chaikin double les points a chaque passe.
        int iterations = 0;
        if (pts.size() > 220 && pts.size() < 2500)
            iterations = 2;
        else if (pts.size() > 80 && pts.size() < 5000)
            iterations = 1;

        QList<QPointF> smoothPts = iterations > 0
                                       ? PathGenerator::applyChaikinAlgorithm(pts, iterations)
                                       : pts;
        if (smoothPts.size() < 3)
            continue;

        QPainterPath sub;
        sub.moveTo(smoothPts.first());
        for (int i = 1; i < smoothPts.size(); ++i)
            sub.lineTo(smoothPts[i]);
        sub.closeSubpath();
        result.addPath(sub);
    }

    return result.elementCount() > 8000 ? result : result.simplified();
}
} // namespace

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

    // --- Étape de simplification en préservant les sous-chemins ---
    QList<QPolygonF> optimizedShapes;
    for (const QPolygonF &poly : shapes) {
        if (poly.isEmpty())
            continue;

        QPainterPath path;
        path.addPolygon(poly);

        // simplified() nettoie la géométrie sans perdre la structure globale.
        QPainterPath cleanPath = path.simplified();
        const QList<QPolygonF> subpaths = cleanPath.toSubpathPolygons();
        if (!subpaths.isEmpty())
            optimizedShapes.append(subpaths);
        else
            optimizedShapes.append(poly);
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
    m_imageImporter.params().max_output_points = 4000;
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

    const double areaW = std::max(1.0, drawAreaSize.width());
    const double areaH = std::max(1.0, drawAreaSize.height());
    const double occupancy = 0.8;
    const double scaleFactor = occupancy * std::min(areaW, areaH) / maxDim;

    QTransform transform;
    transform.translate(-br.x(), -br.y());
    transform.scale(scaleFactor, scaleFactor);
    QPainterPath scaled = transform.map(outline);

    QPointF center(areaW / 2.0, areaH / 2.0);
    scaled.translate(center - scaled.boundingRect().center());

    QPainterPath smoothed = smoothImportedOutline(scaled);
    if (!smoothed.isEmpty())
        return PathGenerator::separateIntoSubpaths(smoothed);

    return PathGenerator::separateIntoSubpaths(scaled);
}
