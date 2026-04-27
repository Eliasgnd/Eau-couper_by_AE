#ifndef TRAJETMOTOR_H
#define TRAJETMOTOR_H

#include <QWidget>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QPointF>
#include <QQueue>
#include <QVector>
#include <QTimer>
#include <QtTypes>
#include <atomic>
#include "ShapeVisualization.h"
#include "StmProtocol.h" // Contient probablement FLAG_VALVE_ON, etc.
#include "pathplanner.h"

// Sécurité : si vos constantes ont disparu, on les recrée ici.
#ifndef HOME_X
#define HOME_X 0.0
#endif
#ifndef HOME_Y
#define HOME_Y 0.0
#endif
#ifndef STEPS_PER_MM
#define STEPS_PER_MM 100 // Ajustez avec votre vraie valeur
#endif

class MainWindow;
class MachineViewModel;

// Ancienne structure (conservée pour ne pas casser le reste du code)
struct SegAnimFrame {
    QPointF canvasPos;
    bool isCut;
    int durationMs;
};

// Nouvelle structure de pré-calcul
struct PreparedSegment {
    StmSegment stmSeg;
    bool isCut;
    qint64 estimatedDurationMs = 0;
};

class TrajetMotor : public QWidget
{
    Q_OBJECT
public:
    explicit TrajetMotor(ShapeVisualization* visu, QWidget* parent = nullptr);
    ~TrajetMotor();

    void setMainWindow(MainWindow* mainWindow);
    void setMachineViewModel(MachineViewModel* vm);

    void setVcut(double vitesse_mm_s);
    void setVtravel(double vitesse_mm_s);

    void executeTrajet();
    void pause();
    void resume();
    void stopCut();

    bool isPaused() const { return m_paused; }

    // On remet l'ancienne fonction publique au cas où elle est appelée ailleurs
    static int estimateTotalSteps(const QList<ContinuousCut>& cuts, const QPoint& homePos);

signals:
    void decoupeProgress(int completed, int total, qint64 remainingMs);
    void decoupeFinished(bool success);

public slots:
    void onMachineDone();
    void onPositionUpdated(int x_steps, int y_steps);
    void onSegmentExecuted(int seg, int x_steps, int y_steps);
    void onAnimStep(); // Conservé pour la compilation

    // Valve state received from STM UART — drives drawing color
    void onValveOn();
    void onValveOff();

private:
    void doExecuteTrajet();

    // On remet les anciennes fonctions privées pour le compilateur
    void appendSegPlan(const QPoint& from, const QPoint& to, bool isCut, QVector<bool>& plan) const;
    bool sendMoveToStm(const QPoint& from, const QPoint& to, uint8_t flags, bool isLast, double mmPerPxScale);

    uint16_t speedToArr(double v_mm_s) const {
        double val = 1000000.0 / (qMax(1.0, v_mm_s) * STEPS_PER_MM);
        return static_cast<uint16_t>(qBound(1.0, val, 65535.0));
    }

    ShapeVisualization* m_visu = nullptr;
    MainWindow* m_mainWindow = nullptr;
    MachineViewModel* m_machine = nullptr;

    QThread* m_workerThread = nullptr;
    QTimer* m_animTimer = nullptr;

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_paused{false};
    std::atomic<bool> m_stopRequested{false};
    std::atomic<bool> m_interrupted{false};
    std::atomic<bool> m_doneReceived{false};

    QMutex m_doneMutex;
    QWaitCondition m_doneCond;

    double m_vCut = 10.0;
    double m_vTrav = 50.0;

    // Nouveaux ajouts pour le pré-calcul
    QList<PreparedSegment> m_plannedSegments;
    QVector<qint64> m_remainingMsByCompletedSegments;
    std::atomic<bool> m_isCurrentlyCutting{false};
    QGraphicsEllipseItem* m_head = nullptr;
    QPointF m_lastHeadPos;

    // Anciennes variables conservées
    int m_execCount = 0;
    int m_totalSteps = 0;
    QVector<bool> m_segIsCut;
    QQueue<SegAnimFrame> m_animQueue;
    QPointF m_animCurrentPos;
};

#endif // TRAJETMOTOR_H
