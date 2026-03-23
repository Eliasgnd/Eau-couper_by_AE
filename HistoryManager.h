#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include <QObject>
#include <QVariant>
#include <QList>

class HistoryManager : public QObject
{
    Q_OBJECT
public:
    explicit HistoryManager(QObject *parent = nullptr);

    void pushState(const QVariant &state);
    QVariant undoLastAction();
    bool canUndo() const;

signals:
    void historyChanged();

private:
    QList<QVariant> m_undoStack;
};

#endif // HISTORYMANAGER_H
