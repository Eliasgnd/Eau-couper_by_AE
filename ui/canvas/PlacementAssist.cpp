#include "PlacementAssist.h"

#include <QObject>
#include <QSizeF>
#include <QtGlobal>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {
struct Anchor {
    qreal value = 0.0;
    QString label;
};

QVector<Anchor> xAnchors(const QRectF &rect)
{
    if (rect.isNull())
        return {{rect.center().x(), QObject::tr("Centre")}};
    return {
        {rect.left(), QObject::tr("Bord gauche")},
        {rect.center().x(), QObject::tr("Centre")},
        {rect.right(), QObject::tr("Bord droit")}
    };
}

QVector<Anchor> yAnchors(const QRectF &rect)
{
    if (rect.isNull())
        return {{rect.center().y(), QObject::tr("Centre")}};
    return {
        {rect.top(), QObject::tr("Bord haut")},
        {rect.center().y(), QObject::tr("Centre")},
        {rect.bottom(), QObject::tr("Bord bas")}
    };
}

bool containsGuide(const QVector<PlacementAssist::Guide> &guides,
                   PlacementAssist::Orientation orientation,
                   qreal position)
{
    return std::any_of(guides.cbegin(), guides.cend(),
                       [orientation, position](const PlacementAssist::Guide &guide) {
        return guide.orientation == orientation && std::abs(guide.position - position) < 0.01;
    });
}

void appendGuide(QVector<PlacementAssist::Guide> &guides,
                 PlacementAssist::Orientation orientation,
                 qreal position,
                 const QString &label,
                 bool snapped)
{
    if (containsGuide(guides, orientation, position))
        return;
    guides.push_back({orientation, position, label, snapped});
}

void collectAxisGuides(const QVector<Anchor> &subjectAnchors,
                       const QVector<Anchor> &targetAnchors,
                       PlacementAssist::Orientation orientation,
                       qreal guideTolerance,
                       qreal magnetTolerance,
                       bool magnetEnabled,
                       qreal *bestCorrection,
                       qreal *bestDistance,
                       QVector<PlacementAssist::Guide> *guides)
{
    for (const Anchor &subject : subjectAnchors) {
        for (const Anchor &target : targetAnchors) {
            const qreal delta = target.value - subject.value;
            const qreal distance = std::abs(delta);
            if (distance > guideTolerance)
                continue;

            const bool snapped = magnetEnabled && distance <= magnetTolerance;
            appendGuide(*guides, orientation, target.value, target.label, snapped);

            if (snapped && distance < *bestDistance) {
                *bestDistance = distance;
                *bestCorrection = delta;
            }
        }
    }
}
} // namespace

bool PlacementAssist::Result::hasSnap() const
{
    return !qFuzzyIsNull(correction.x()) || !qFuzzyIsNull(correction.y());
}

QRectF PlacementAssist::Result::correctedRect(const QRectF &subject) const
{
    return subject.translated(correction);
}

QPointF PlacementAssist::Result::correctedPoint(const QPointF &point) const
{
    return point + correction;
}

PlacementAssist::Result PlacementAssist::resolve(const QRectF &subject,
                                                 const QVector<QRectF> &targets,
                                                 const QRectF &visibleArea,
                                                 const Options &options)
{
    Result result;
    if (!options.enabled || subject.width() < 0.0 || subject.height() < 0.0)
        return result;

    QVector<QRectF> allTargets = targets;
    if (options.includeViewportCenter && visibleArea.isValid()) {
        const QPointF center = visibleArea.center();
        allTargets.push_back(QRectF(center, QSizeF(0.0, 0.0)));
    }

    qreal bestX = 0.0;
    qreal bestY = 0.0;
    qreal bestXDistance = std::numeric_limits<qreal>::max();
    qreal bestYDistance = std::numeric_limits<qreal>::max();

    const QVector<Anchor> subjectX = xAnchors(subject);
    const QVector<Anchor> subjectY = yAnchors(subject);

    for (const QRectF &target : allTargets) {
        if (target.width() < 0.0 || target.height() < 0.0)
            continue;

        collectAxisGuides(subjectX, xAnchors(target), Orientation::Vertical,
                          options.guideTolerance, options.magnetTolerance,
                          options.magnetEnabled, &bestX, &bestXDistance,
                          &result.guides);
        collectAxisGuides(subjectY, yAnchors(target), Orientation::Horizontal,
                          options.guideTolerance, options.magnetTolerance,
                          options.magnetEnabled, &bestY, &bestYDistance,
                          &result.guides);
    }

    result.correction = QPointF(bestX, bestY);
    return result;
}
