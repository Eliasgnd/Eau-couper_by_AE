#include "MouseInteractionHandler.h"

#include "DrawModeManager.h"
#include "ShapeManager.h"
#include "ViewTransformer.h"
#include <QtGlobal>

#include <QMouseEvent>
#include <QtGlobal>

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
    m_startPoint = logicalPos;
    m_currentPoint = logicalPos;

    if (event->button() == Qt::MiddleButton || m_modeManager->drawMode() == DrawModeManager::DrawMode::Pan) {
        m_drawing = true;
        return;
    }

    if (event->button() != Qt::LeftButton) {
        return;
    }

    if (m_modeManager->drawMode() == DrawModeManager::DrawMode::Supprimer) {
        const auto &shapes = m_shapeManager->shapes();
        for (int i = shapes.size() - 1; i >= 0; --i) {
            if (shapes[i].path.contains(logicalPos)) {
                m_shapeManager->removeShape(i);
                break;
            }
        }
        emit requestUpdate();
        return;
    }

    if (m_modeManager->drawMode() == DrawModeManager::DrawMode::Deplacer) {
        const auto &shapes = m_shapeManager->shapes();
        int clickedIndex = -1;
        for (int i = shapes.size() - 1; i >= 0; --i) {
            if (shapes[i].path.contains(logicalPos)) {
                clickedIndex = i;
                break;
            }
        }

        if (clickedIndex >= 0) {
            if (!m_shapeManager->selectedShapes().contains(clickedIndex)) {
                m_shapeManager->setSelectedShapes({clickedIndex});
            }
            m_drawing = true;
        } else {
            m_shapeManager->clearSelection();
            m_drawing = false;
        }

        emit requestUpdate();
        return;
    }

    m_drawing = true;
    emit requestUpdate();
}

void MouseInteractionHandler::handleMouseMove(QMouseEvent *event, const QPointF &logicalPos)
{
    if (!m_drawing) return;

    if ((event->buttons() & Qt::MiddleButton) || m_modeManager->drawMode() == DrawModeManager::DrawMode::Pan) {
        const QPointF logicalDelta = logicalPos - m_currentPoint;
        m_transformer->applyPanDelta(logicalDelta * m_transformer->scale());
        m_currentPoint = logicalPos;
        emit requestUpdate();
        return;
    }

    if (m_modeManager->drawMode() == DrawModeManager::DrawMode::Deplacer &&
        (event->buttons() & Qt::LeftButton)) {
        const QPointF delta = logicalPos - m_currentPoint;
        if (!qFuzzyIsNull(delta.x()) || !qFuzzyIsNull(delta.y())) {
            const QVector<int> selected = m_shapeManager->selectedShapes();
            QList<ShapeManager::Shape> updated = m_shapeManager->shapes();
            for (int idx : selected) {
                if (idx >= 0 && idx < updated.size()) {
                    updated[idx].path.translate(delta);
                }
            }
            m_shapeManager->setShapes(updated);
            m_shapeManager->setSelectedShapes(selected);
        }
    }

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
