#include "HistoryManager.h"

HistoryManager::HistoryManager(QObject *parent)
    : QObject(parent)
{
}

void HistoryManager::pushState(const QVariant &state)
{
    m_undoStack.push_back(state);
    emit historyChanged();
}

QVariant HistoryManager::undoLastAction()
{
    if (m_undoStack.isEmpty()) return {};
    const QVariant value = m_undoStack.takeLast();
    emit historyChanged();
    return value;
}

bool HistoryManager::canUndo() const
{
    return !m_undoStack.isEmpty();
}
