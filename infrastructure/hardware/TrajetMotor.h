#ifndef TRAJETMOTOR_H
#define TRAJETMOTOR_H

#include <QWidget>
#include <QPointF>
#include <QMutex>
#include <QWaitCondition>
#include <QVector>
#include <QQueue>
#include <QTimer>
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

    // ==============================================================
    // NOUVEAU SLOT PUBLIC : Doit être ici pour que CuttingService puisse s'y connecter
    // ==============================================================
    void onPositionUpdated(int x_steps, int y_steps);

    // Appelés depuis le thread principal via les signaux MachineViewModel
    void onSegmentExecuted(int seg, int x_steps, int y_steps);
    void onMachineDone();

signals:
    void decoupeProgress(int remaining, int total);
    void decoupeFinished(bool success);

private:
    // Fonction tournant dans le thread séparé
    void doExecuteTrajet();

    // Pré-construction du plan de segments (un bool isCut par chunk STM)
    void appendSegPlan(const QPoint& from, const QPoint& to,
                       bool isCut, QVector<bool>& plan) const;

    // Envoi effectif des segments au STM32 via le ViewModel
    bool sendMoveToStm(const QPoint& from, const QPoint& to,
                       uint8_t flags, bool isLast, double mmPerPxScale);

    // Calcul de la progression totale (utilisé comme fallback si déconnecté)
    int estimateTotalSteps(const QList<ContinuousCut>& cuts, const QPoint& homePos);

    // Dépendances
    ShapeVisualization* m_visu = nullptr;
    MainWindow* m_mainWindow = nullptr;
    MachineViewModel* m_machine = nullptr;

    // Paramètres de vitesse
    double m_vCut  = 10.0;   // mm/s
    double m_vTrav = 150.0;  // mm/s (vitesse de déplacement rapide)

    // Contrôle du thread
    std::atomic<bool> m_paused{false};
    std::atomic<bool> m_stopRequested{false};
    std::atomic<bool> m_interrupted{false};
    bool              m_running = false;
    QThread* m_workerThread = nullptr;

    // Attente signal DONE du STM
    QMutex             m_doneMutex;
    QWaitCondition     m_doneCond;
    std::atomic<bool>  m_doneReceived{false};

    // Visualisation pilotée par le timer local (Pi-driven)
    QGraphicsEllipseItem* m_head = nullptr;
    QVector<bool>         m_segIsCut;    // plan pré-construit : true = coupe (rouge)
    int                   m_execCount = 0;

    // Progression
    int m_totalSteps = 0;

    // ------------------------------------------------------------------
    //  Ancienne Animation locale (Vous pourrez supprimer cette partie plus tard si tout marche)
    // ------------------------------------------------------------------
    struct SegAnimFrame {
        QPointF canvasPos;   // position canvas (pixels) en fin de chunk
        bool    isCut;       // true = coupe (rouge), false = voyage (bleu)
        int     durationMs;  // temps simulé d'exécution de ce chunk (ms)
    };

    QQueue<SegAnimFrame> m_animQueue;
    QTimer* m_animTimer    = nullptr;
    QPointF              m_animCurrentPos;

    // Point de repos machine (en pixels = mm)
    static constexpr int HOME_X = 600;
    static constexpr int HOME_Y = 400;

    // Taille maximale d'un chunk STM (identique à sendMoveToStm)
    static constexpr int MAX_STEPS_CHUNK = 30000;

private slots:
    // Pilote d'animation local — avance la tête d'un frame à la fois
    void onAnimStep();
};

#endif // TRAJETMOTOR_H
