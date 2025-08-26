#pragma once
#include <QObject>

class PlacementTests : public QObject {
    Q_OBJECT
private slots:
    void borderContactNotOverlap();
    void interiorIntersection();
    void motorControlSteps();
    void normalizationRemovesDuplicates();
    void complexityGuardRejects();
    void stressWorstCase();
    void lowEndModeAdjusts();
    void selfIntersectingAccepted();
    void selfIntersectingOverlapDetected();
};
