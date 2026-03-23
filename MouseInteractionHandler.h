#ifndef MOUSEINTERACTIONHANDLER_H
#define MOUSEINTERACTIONHANDLER_H

#include <QObject>
#include <QPointF>

class QMouseEvent;

class MouseInteractionHandler : public QObject
{
    Q_OBJECT
public:
    explicit MouseInteractionHandler(QObject *parent = nullptr);

    void handleMousePress(QMouseEvent *event, const QPointF &logicalPos);
    void handleMouseMove(QMouseEvent *event, const QPointF &logicalPos);
    void handleMouseRelease(QMouseEvent *event, const QPointF &logicalPos);

signals:
    void mousePressed(const QPointF &logicalPos);
    void mouseMoved(const QPointF &logicalPos);
    void mouseReleased(const QPointF &logicalPos);
};

#endif // MOUSEINTERACTIONHANDLER_H
