#ifndef MOUSEINTERACTIONHANDLER_H
#define MOUSEINTERACTIONHANDLER_H

#include <QObject>
#include <QPointF>

class QMouseEvent;
class DrawModeManager;
class ShapeManager;
class ViewTransformer;

class MouseInteractionHandler : public QObject
{
    Q_OBJECT
public:
    explicit MouseInteractionHandler(ShapeManager *shapeManager,
                                     DrawModeManager *modeManager,
                                     ViewTransformer *transformer,
                                     QObject *parent = nullptr);

    void handleMousePress(QMouseEvent *event, const QPointF &logicalPos);
    void handleMouseMove(QMouseEvent *event, const QPointF &logicalPos);
    void handleMouseRelease(QMouseEvent *event, const QPointF &logicalPos);

signals:
    void requestUpdate();

private:
    ShapeManager *m_shapeManager = nullptr;
    DrawModeManager *m_modeManager = nullptr;
    ViewTransformer *m_transformer = nullptr;

    QPointF m_startPoint;
    QPointF m_currentPoint;
    bool m_drawing = false;
};

#endif // MOUSEINTERACTIONHANDLER_H
