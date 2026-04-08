#ifndef TRAJETMOTOR_H
#define TRAJETMOTOR_H

#include <QWidget>
#include <QQueue>
#include <QPointF>
class MainWindow;
#include "MotorControl.h"
#include "ShapeVisualization.h"

// --- Types de commandes machine ---
enum class CommandType {
    MoveRapid,
    MoveCut,
    JetOn,
    JetOff,
    Wait
};

// --- Structure d'une instruction ---
struct MachineCommand {
    CommandType type;
    QPointF targetPos;
    int delayMs;

    static MachineCommand rapid(QPointF pos) { return {CommandType::MoveRapid, pos, 0}; }
    static MachineCommand cut(QPointF pos) { return {CommandType::MoveCut, pos, 0}; }
    static MachineCommand jetOn() { return {CommandType::JetOn, QPointF(), 0}; }
    static MachineCommand jetOff() { return {CommandType::JetOff, QPointF(), 0}; }
    static MachineCommand wait(int ms) { return {CommandType::Wait, QPointF(), ms}; }
};

// --- Le gestionnaire de file d'attente ---
class CommandBuffer {
public:
    void push(const MachineCommand& cmd) { m_queue.enqueue(cmd); }
    MachineCommand pop() { return m_queue.dequeue(); }
    bool isEmpty() const { return m_queue.isEmpty(); }
    int size() const { return m_queue.size(); }
    void clear() { m_queue.clear(); }
private:
    QQueue<MachineCommand> m_queue;
};


class TrajetMotor : public QWidget {
    Q_OBJECT
public:
    explicit TrajetMotor(ShapeVisualization* visu, QWidget* parent = nullptr);

    void executeTrajet();
    bool isPaused() const { return m_running && m_paused; }
    void setMainWindow(MainWindow* mainWindow);

public slots:
    void pause();
    void resume();
    void stopCut();
    void moveHeadProgressive(const QPoint& start, const QPoint& end,QGraphicsEllipseItem* head, bool cut);

signals:
    void decoupeProgress(int remaining, int total);

private:
    ShapeVisualization* m_visu{};
    MotorControl        m_motor;
    CommandBuffer       m_commandBuffer;

    bool m_paused        = false;
    bool m_stopRequested = false;
    bool m_running       = false;

    MainWindow* m_mainWindow = nullptr;

    int m_totalSteps = 0;
    int m_progressCounter = 0;
    bool m_interrupted = false;

};

#endif // TRAJETMOTOR_H
