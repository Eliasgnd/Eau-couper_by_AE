#include "GestureHandler.h"

#include <QGestureEvent>
#include <QPinchGesture>

GestureHandler::GestureHandler(QObject *parent)
    : QObject(parent)
{
}

bool GestureHandler::handleGestureEvent(QGestureEvent *event)
{
    if (!event) return false;
    if (QGesture *pinch = event->gesture(Qt::PinchGesture)) {
        return handlePinchGesture(static_cast<QPinchGesture *>(pinch));
    }
    return false;
}

bool GestureHandler::handlePinchGesture(QPinchGesture *gesture)
{
    if (!gesture) return false;
    emit pinchZoomRequested(gesture->centerPoint(), gesture->scaleFactor());
    return true;
}
