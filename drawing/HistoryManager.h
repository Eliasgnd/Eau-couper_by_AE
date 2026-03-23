#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include <QObject>

class ShapeManager;

/**
 * @class HistoryManager
 * @brief Encapsule les opérations d'annulation/rétablissement basées sur l'état des formes.
 *
 * @note Le stockage effectif des snapshots est maintenu dans ShapeManager.
 * @see ShapeManager
 */
class HistoryManager : public QObject
{
    Q_OBJECT
public:
    explicit HistoryManager(ShapeManager *shapeManager, QObject *parent = nullptr);

    void pushState();
    void undoLastAction();

private:
    ShapeManager *m_shapeManager = nullptr;
};

#endif // HISTORYMANAGER_H
