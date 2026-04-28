#include "HistoryManager.h"
#include <QUndoCommand>

// --- Commande : Ajouter une forme ---
class AddShapeCommand : public QUndoCommand {
    ShapeManager* m_mgr;
    ShapeManager::Shape m_shape;
public:
    AddShapeCommand(ShapeManager* mgr, const QPainterPath& path, int id, const QString& label)
        : m_mgr(mgr) {
        m_shape.path = path;
        m_shape.originalId = id;
        setText(label);
    }
    void redo() override { m_mgr->addShape(m_shape); }
    void undo() override {
        // Correction C4267 : ajout du cast static_cast<int>
        m_mgr->removeShape(static_cast<int>(m_mgr->shapes().size()) - 1);
    }
};

// --- Commande : Supprimer des formes ---
class DeleteShapesCommand : public QUndoCommand {
    ShapeManager* m_mgr;
    struct RemovedInfo { int index; ShapeManager::Shape shape; };
    std::vector<RemovedInfo> m_removed;
public:
    DeleteShapesCommand(ShapeManager* mgr, std::vector<int> indices) : m_mgr(mgr) {
        setText("Supprimer sélection");
        // On trie les indices par ordre décroissant pour ne pas invalider les suivants lors du stockage
        std::sort(indices.begin(), indices.end(), std::greater<int>());
        const auto& currentShapes = m_mgr->shapes();
        for(int idx : indices) {
            m_removed.push_back({idx, currentShapes[idx]});
        }
    }
    void redo() override {
        for(const auto& info : m_removed) m_mgr->removeShape(info.index);
    }
    void undo() override {
        // On réinsère dans l'ordre inverse (croissant) pour retrouver les bonnes places
        for(auto it = m_removed.rbegin(); it != m_removed.rend(); ++it) {
            auto shapes = m_mgr->shapes();
            shapes.insert(shapes.begin() + it->index, it->shape);
            m_mgr->setShapes(shapes);
        }
    }
};

// --- Commande : Snapshot (Cas complexes) ---
class SnapshotCommand : public QUndoCommand {
    ShapeManager* m_mgr;
    std::vector<ShapeManager::Shape> m_old, m_new;
public:
    SnapshotCommand(ShapeManager* mgr, std::vector<ShapeManager::Shape> o, std::vector<ShapeManager::Shape> n, QString label)
        : m_mgr(mgr), m_old(o), m_new(n) { setText(label); }
    void redo() override { m_mgr->setShapes(m_new); }
    void undo() override { m_mgr->setShapes(m_old); }
};

HistoryManager::HistoryManager(ShapeManager *shapeManager, QObject *parent)
    : QObject(parent), m_shapeManager(shapeManager) {}

void HistoryManager::commitAddShape(const QPainterPath &path, int id, const QString &label) {
    m_undoStack.push(new AddShapeCommand(m_shapeManager, path, id, label));
}

void HistoryManager::commitDeleteShapes(const std::vector<int> &indices) {
    if(!indices.empty()) m_undoStack.push(new DeleteShapesCommand(m_shapeManager, indices));
}

void HistoryManager::commitSnapshot(const std::vector<ShapeManager::Shape> &oldState,
                                    const std::vector<ShapeManager::Shape> &newState,
                                    const QString &label) {
    m_undoStack.push(new SnapshotCommand(m_shapeManager, oldState, newState, label));
}

void HistoryManager::undo() { m_undoStack.undo(); }
void HistoryManager::redo() { m_undoStack.redo(); }
bool HistoryManager::canUndo() const { return m_undoStack.canUndo(); }
bool HistoryManager::canRedo() const { return m_undoStack.canRedo(); }
QString HistoryManager::undoText() const { return m_undoStack.undoText(); }
QString HistoryManager::redoText() const { return m_undoStack.redoText(); }
std::vector<ShapeManager::Shape> HistoryManager::getCurrentState() const { return m_shapeManager->shapes(); }
