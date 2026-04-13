#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include <QObject>
#include <QUndoStack>
#include <QPainterPath>
#include <vector>
#include "ShapeManager.h"

class HistoryManager : public QObject
{
    Q_OBJECT
public:
    explicit HistoryManager(ShapeManager *shapeManager, QObject *parent = nullptr);

    // Actions atomiques (Performantes)
    void commitAddShape(const QPainterPath &path, int id, const QString &label = "Ajouter Forme");
    void commitDeleteShapes(const std::vector<int> &indices);
    void commitPasteShapes(const std::vector<ShapeManager::Shape> &shapes);

    // Action globale (Pour les cas complexes : import, fusion, etc.)
    void commitSnapshot(const std::vector<ShapeManager::Shape> &oldState,
                        const std::vector<ShapeManager::Shape> &newState,
                        const QString &label = "Modification");

    void undo();
    void redo();

    // Helper pour récupérer l'état actuel avant une modif complexe
    std::vector<ShapeManager::Shape> getCurrentState() const;

private:
    ShapeManager *m_shapeManager = nullptr;
    QUndoStack m_undoStack;
};

#endif // HISTORYMANAGER_H
