#pragma once

#include <QList>
#include <QPainterPath>
#include <QRectF>
#include <functional>

struct PlacementParams {
    QPainterPath prototypePath;
    QRectF containerRect;
    int shapeCount = 0;
    int spacing = 0;
    int step = 5; // <-- REMIS : Requis par ShapeCoordinator
    QList<int> angles;
};

struct PlacementResult {
    QList<QPainterPath> placedPaths;
    int totalPositions = 0; // <-- REMIS : Requis par ShapeCoordinator
    int processedPositions = 0;
    bool cancelled = false;
};

class PlacementOptimizer
{
public:
    static PlacementResult run(const PlacementParams &params,
                               const std::function<bool(int current, int total)> &onProgress = {});
};
