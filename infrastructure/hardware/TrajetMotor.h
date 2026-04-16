#ifndef TRAJETMOTOR_H
#define TRAJETMOTOR_H

#include <QWidget>
#include <QPointF>
#include <atomic>
#include <QThread>

// Forward declarations
class MainWindow;
class MachineViewModel;
class QGraphicsEllipseItem;

#include "ShapeVisualization.h"
#include "StmProtocol.h"
#include "pathplanner.h"

class TrajetMotor : public QWidget {
    Q_OBJECT
public:
    explicit TrajetMotor(ShapeVisualization* visu, QWidget* parent = nullptr);
    ~TrajetMotor() override;

    // Lance le thread de découpe
    void executeTrajet();

    // État de la machine
    bool isPaused() const { return m_running && m_paused.load(); }

    // Setters pour les dépendances
    void setMainWindow(MainWindow* mainWindow);
    void setMachineViewModel(MachineViewModel* vm);

    // Configuration des vitesses (mm/s)
    void setVcut(double vitesse_mm_s);
    void setVtravel(double vitesse_mm_s);

public slots:
    void pause();
    void resume();
    void stopCut();

signals:
    void decoupeProgress(int remaining, int total);

private:
    // Fonction tournant dans le thread séparé
    void doExecuteTrajet();

    // Animation visuelle sur le canvas
    void moveHeadProgressive(const QPoint& start, const QPoint& end,
                             QGraphicsEllipseItem* head, bool cut);

    // Envoi effectif des segments au STM32 via le ViewModel
    bool sendMoveToStm(const QPoint& from, const QPoint& to,
                       uint8_t flags, bool isLast, double mmPerPxScale);

    // Calcul de la progression totale
    int estimateTotalSteps(const QList<ContinuousCut>& cuts, const QPoint& homePos);

    // Dépendances
    ShapeVisualization* m_visu = nullptr;
    MainWindow* m_mainWindow = nullptr;
    MachineViewModel* m_machine = nullptr;

    // Paramètres de vitesse (remplace MotorControl)
    double m_vCut  = 10.0;   // mm/s
    double m_vTrav = 150.0;  // mm/s (vitesse de déplacement rapide)

    // Contrôle du thread
    std::atomic<bool> m_paused{false};
    std::atomic<bool> m_stopRequested{false};
    std::atomic<bool> m_interrupted{false};
    bool              m_running = false;
    QThread* m_workerThread = nullptr;

    // Progression
    int m_totalSteps      = 0;
    int m_progressCounter = 0;
};

#endif // TRAJETMOTOR_H
