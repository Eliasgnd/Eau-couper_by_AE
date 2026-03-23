#include "HistoryManager.h"

#include "ShapeManager.h"
#include <QtGlobal>

HistoryManager::HistoryManager(ShapeManager *shapeManager, QObject *parent)
    : QObject(parent)
    , m_shapeManager(shapeManager)
{
    Q_ASSERT(m_shapeManager != nullptr);
}

void HistoryManager::pushState()
{
    if (!m_shapeManager) return;
    m_shapeManager->pushState();
}

void HistoryManager::undoLastAction()
{
    if (!m_shapeManager) return;
    m_shapeManager->undoLastAction();
}
