#ifndef DRAWINGSTATE_H
#define DRAWINGSTATE_H

#include <QList>
#include <QPainterPath>
#include <QPointF>

/**
 * @struct DrawingState
 * @brief État géométrique partagé entre CustomDrawArea et MouseInteractionHandler.
 *
 * Centralise les données volatiles du tracé en cours pour éviter la duplication
 * de m_startPoint / m_currentPoint entre les deux classes.
 *
 * @note Propriété de CustomDrawArea (unique_ptr). MouseInteractionHandler en reçoit
 *       un pointeur nu en lecture/écriture.
 */
struct DrawingState {
    QPointF        startPoint;
    QPointF        currentPoint;
    QList<QPointF> strokePoints;
    QPainterPath   freehandPreviewPath;
    int            freehandPreviewPointCount = -1;
    int            freehandPreviewSmoothingLevel = -1;
    QList<QPointF> pointByPointPoints;
    bool           extendingExistingPath = false;
    int            extendingShapeIndex = -1;
    bool           extendingFromStart = false;
    QPointF        extendingAnchor;

    bool    gommeErasing  = false;
    QPointF gommeCenter;
    QPointF lastEraserPos;
};

#endif // DRAWINGSTATE_H
