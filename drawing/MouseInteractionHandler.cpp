#include "MouseInteractionHandler.h"

#include "DrawModeManager.h"
#include "ShapeManager.h"
#include "ViewTransformer.h"

#include <Q_ASSERT>
#include <QMouseEvent>

MouseInteractionHandler::MouseInteractionHandler(ShapeManager *shapeManager,
                                                 DrawModeManager *modeManager,
                                                 ViewTransformer *transformer,
                                                 QObject *parent)
    : QObject(parent)
    , m_shapeManager(shapeManager)
    , m_modeManager(modeManager)
    , m_transformer(transformer)
{
    Q_ASSERT(m_shapeManager != nullptr);
    Q_ASSERT(m_modeManager != nullptr);
    Q_ASSERT(m_transformer != nullptr);
}

void MouseInteractionHandler::handleMousePress(QMouseEvent *event, const QPointF &logicalPos)
{
    Q_UNUSED(event)
    m_startPoint = logicalPos;
    m_currentPoint = logicalPos;
    m_drawing = true;
    emit requestUpdate();
}

void MouseInteractionHandler::handleMouseMove(QMouseEvent *event, const QPointF &logicalPos)
{
    Q_UNUSED(event)
    if (!m_drawing) return;
    m_currentPoint = logicalPos;
    emit requestUpdate();
}

void MouseInteractionHandler::handleMouseRelease(QMouseEvent *event, const QPointF &logicalPos)
{
    Q_UNUSED(event)
    if (!m_drawing) return;
    m_currentPoint = logicalPos;
    m_drawing = false;
    emit requestUpdate();
}
