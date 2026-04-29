#ifndef PLACEMENTASSIST_H
#define PLACEMENTASSIST_H

#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVector>

class PlacementAssist
{
public:
    enum class Orientation {
        Vertical,
        Horizontal
    };

    struct Guide {
        Orientation orientation = Orientation::Vertical;
        qreal position = 0.0;
        QString label;
        bool snapped = false;
    };

    struct Options {
        bool enabled = true;
        bool magnetEnabled = true;
        qreal guideTolerance = 18.0;
        qreal magnetTolerance = 12.0;
        bool includeViewportCenter = true;
    };

    struct Result {
        QPointF correction;
        QVector<Guide> guides;

        bool hasSnap() const;
        QRectF correctedRect(const QRectF &subject) const;
        QPointF correctedPoint(const QPointF &point) const;
    };

    static Result resolve(const QRectF &subject,
                          const QVector<QRectF> &targets,
                          const QRectF &visibleArea,
                          const Options &options);

private:
    PlacementAssist() = default;
};

#endif // PLACEMENTASSIST_H
