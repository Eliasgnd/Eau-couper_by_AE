#ifndef PLACEMENT_ASSIST_TESTS_H
#define PLACEMENT_ASSIST_TESTS_H

#include <QObject>

class PlacementAssistTests : public QObject
{
    Q_OBJECT
private slots:
    void snapsCenterToCenter();
    void snapsLeftEdgeToRightEdge();
    void doesNotSnapOutsideTolerance();
    void ignoresExcludedSelectionTargets();
    void toleranceScalesFromScreenPixels();
};

#endif // PLACEMENT_ASSIST_TESTS_H
