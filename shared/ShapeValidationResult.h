#pragma once

#include <QSet>

struct ShapeValidationResult {
    bool allValid = true;
    QSet<int> outOfBoundsIndices;
    QSet<int> collisionIndices;
};
