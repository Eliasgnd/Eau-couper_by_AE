#include "MouseInteractionHandler.h"

#include "DrawingState.h"
#include "DrawModeManager.h"
#include "ShapeManager.h"
#include "ViewTransformer.h"

#include <QMouseEvent>
#include <QPainterPathStroker>
#include <QtGlobal>

#include <algorithm>

MouseInteractionHandler::MouseInteractionHandler(ShapeManager    *shapeManager,
                                                 DrawModeManager *modeManager,
                                                 ViewTransformer *transformer,
                                                 DrawingState    *state,
                                                 QObject         *parent)
    : QObject(parent)
    , m_shapeManager(shapeManager)
    , m_modeManager(modeManager)
    , m_transformer(transformer)
    , m_state(state)
{
    Q_ASSERT(m_shapeManager != nullptr);
    Q_ASSERT(m_modeManager  != nullptr);
    Q_ASSERT(m_transformer  != nullptr);
    Q_ASSERT(m_state        != nullptr);
}

void MouseInteractionHandler::handleMousePress(QMouseEvent *event, const QPointF &logicalPos)
{
    // Initialise la position courante dans le state partagé.
    m_state->currentPoint = logicalPos;
    m_draggingSelection = false;
    m_selectionDragMoved = false;
    m_pressedSelectedIndex = -1;

    if (event->button() == Qt::MiddleButton ||
        m_modeManager->drawMode() == DrawModeManager::DrawMode::Pan) {
        m_active = true;
        return;
    }

    if (event->button() != Qt::LeftButton) return;

    if (m_modeManager->drawMode() == DrawModeManager::DrawMode::Supprimer) {
        const auto &shapes = m_shapeManager->shapes();
        QPainterPathStroker stroker;
        stroker.setWidth(18.0);
        for (int i = static_cast<int>(shapes.size()) - 1; i >= 0; --i) {
            if (!shapes[i].bounds.adjusted(-18.0, -18.0, 18.0, 18.0).contains(logicalPos))
                continue;
            if (shapes[i].path.contains(logicalPos) ||
                stroker.createStroke(shapes[i].path).contains(logicalPos)) {
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
        QPainterPathStroker stroker;
        stroker.setWidth(26.0);
        stroker.setCapStyle(Qt::RoundCap);
        stroker.setJoinStyle(Qt::RoundJoin);
        for (int i = static_cast<int>(shapes.size()) - 1; i >= 0; --i) {
            const QPainterPath &path = shapes[i].path;
            if (!shapes[i].bounds.adjusted(-26.0, -26.0, 26.0, 26.0).contains(logicalPos))
                continue;
            if (path.contains(logicalPos) ||
                stroker.createStroke(path).contains(logicalPos)) {
                clickedIndex = i;
                break;
            }
        }

        if (clickedIndex >= 0) {
            const auto &sel = m_shapeManager->selectedShapes();
            if (std::find(sel.begin(), sel.end(), clickedIndex) == sel.end())
                m_shapeManager->setSelectedShapes({clickedIndex});
            m_active = true;
        } else {
            m_shapeManager->clearSelection();
            m_active = false;
        }

        emit requestUpdate();
        return;
    }

    if (!m_shapeManager->selectedShapes().empty()) {
        const auto &shapes = m_shapeManager->shapes();
        const auto &selected = m_shapeManager->selectedShapes();
        QPainterPathStroker stroker;
        stroker.setWidth(26.0);
        stroker.setCapStyle(Qt::RoundCap);
        stroker.setJoinStyle(Qt::RoundJoin);

        for (auto it = selected.rbegin(); it != selected.rend(); ++it) {
            const int idx = *it;
            if (idx < 0 || idx >= static_cast<int>(shapes.size()))
                continue;
            const QPainterPath &path = shapes[idx].path;
            if (!shapes[idx].bounds.adjusted(-26.0, -26.0, 26.0, 26.0).contains(logicalPos))
                continue;
            if (path.contains(logicalPos) ||
                stroker.createStroke(path).contains(logicalPos)) {
                m_draggingSelection = true;
                m_pressedSelectedIndex = idx;
                m_active = true;
                emit requestUpdate();
                return;
            }
        }
    }

    m_active = true;
    emit requestUpdate();
}

void MouseInteractionHandler::handleMouseMove(QMouseEvent *event, const QPointF &logicalPos)
{
    if (!m_active) return;

    if ((event->buttons() & Qt::MiddleButton) ||
        m_modeManager->drawMode() == DrawModeManager::DrawMode::Pan) {
        const QPointF logicalDelta = logicalPos - m_state->currentPoint;
        m_transformer->applyPanDelta(logicalDelta * m_transformer->scale());
        m_state->currentPoint = logicalPos;
        emit requestUpdate();
        return;
    }

    if ((m_modeManager->drawMode() == DrawModeManager::DrawMode::Deplacer || m_draggingSelection) &&
        (event->buttons() & Qt::LeftButton)) {
        const QPointF delta = logicalPos - m_state->currentPoint;
        if (!qFuzzyIsNull(delta.x()) || !qFuzzyIsNull(delta.y())) {
            m_selectionDragMoved = true;
            m_shapeManager->translateSelectedShapes(delta);
        }
    }

    m_state->currentPoint = logicalPos;
    emit requestUpdate();
}

void MouseInteractionHandler::handleMouseRelease(QMouseEvent *event, const QPointF &logicalPos)
{
    Q_UNUSED(event)
    if (!m_active) return;
    if (m_draggingSelection && !m_selectionDragMoved &&
        m_modeManager->isMultiSelectMode() && m_pressedSelectedIndex >= 0) {
        std::vector<int> selected = m_shapeManager->selectedShapes();
        selected.erase(std::remove(selected.begin(), selected.end(), m_pressedSelectedIndex), selected.end());
        m_shapeManager->setSelectedShapes(selected);
    }
    m_state->currentPoint = logicalPos;
    m_active = false;
    m_draggingSelection = false;
    m_selectionDragMoved = false;
    m_pressedSelectedIndex = -1;
    emit requestUpdate();
}

bool MouseInteractionHandler::isSelectionDragActive() const
{
    return m_draggingSelection;
}
