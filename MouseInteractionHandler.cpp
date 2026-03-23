#include "MouseInteractionHandler.h"

#include <QMouseEvent>

MouseInteractionHandler::MouseInteractionHandler(QObject *parent)
    : QObject(parent)
{
}

void MouseInteractionHandler::handleMousePress(QMouseEvent *event, const QPointF &logicalPos)
{
    Q_UNUSED(event);
    emit mousePressed(logicalPos);
}

void MouseInteractionHandler::handleMouseMove(QMouseEvent *event, const QPointF &logicalPos)
{
    Q_UNUSED(event);
    emit mouseMoved(logicalPos);
}

void MouseInteractionHandler::handleMouseRelease(QMouseEvent *event, const QPointF &logicalPos)
{
    Q_UNUSED(event);
    emit mouseReleased(logicalPos);
}
