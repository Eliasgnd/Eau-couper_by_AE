#ifndef GESTUREHANDLER_H
#define GESTUREHANDLER_H

#include <QObject>
#include <QPointF>

class QGestureEvent;
class QPinchGesture;

class GestureHandler : public QObject
{
    Q_OBJECT
public:
    explicit GestureHandler(QObject *parent = nullptr);

    bool handleGestureEvent(QGestureEvent *event);
    bool handlePinchGesture(QPinchGesture *gesture);

signals:
    void pinchZoomRequested(const QPointF &center, qreal factor);
    void panRequested(const QPointF &delta);
};

#endif // GESTUREHANDLER_H
