#ifndef TRAJETMOTOR_H
#define TRAJETMOTOR_H

#include <QWidget>
#include <QQueue>
#include <QPointF>
#include <atomic>
#include <QThread>
class MainWindow;
class MachineViewModel;
#include "MotorControl.h"
#include "ShapeVisualization.h"
#include "StmProtocol.h"

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
    ~TrajetMotor() override;

    void executeTrajet();
    bool isPaused() const { return m_running && m_paused.load(); }
    void setMainWindow(MainWindow* mainWindow);
    void setMachineViewModel(MachineViewModel* vm);
    void setVcut(double vitesse_mm_s);   // Vitesse de coupe (mm/s) — lue à chaque segment

public slots:
    void pause();
    void resume();
    void stopCut();

signals:
    void decoupeProgress(int remaining, int total);

private:
    // Exécution réelle — tourne dans le thread worker (jamais sur le thread UI)
    void doExecuteTrajet();

    // Animation pas-à-pas — appelée depuis le thread worker uniquement.
    // Les opérations scène sont dispatché sur le thread principal via invokeMethod.
    void moveHeadProgressive(const QPoint& start, const QPoint& end,
                             QGraphicsEllipseItem* head, bool cut);

    void sendMoveToStm(const QPoint& from, const QPoint& to,
                       uint8_t flags, bool isLast, double mmPerPxScale);

    ShapeVisualization* m_visu{};
    MotorControl        m_motor;
    CommandBuffer       m_commandBuffer;

    // Flags partagés entre thread principal et worker → atomiques
    std::atomic<bool> m_paused{false};
    std::atomic<bool> m_stopRequested{false};
    std::atomic<bool> m_interrupted{false};

    // Uniquement modifié sur le thread principal
    bool m_running = false;

    MainWindow*       m_mainWindow   = nullptr;
    MachineViewModel* m_machine      = nullptr;

    int  m_totalSteps      = 0;
    int  m_progressCounter = 0;

    QThread *m_workerThread = nullptr;
};

#endif // TRAJETMOTOR_H
