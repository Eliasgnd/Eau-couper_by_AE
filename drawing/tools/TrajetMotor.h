#ifndef TRAJETMOTOR_H
#define TRAJETMOTOR_H

#include <QWidget>
class MainWindow;
#include "MotorControl.h"
#include "ShapeVisualization.h"

class TrajetMotor : public QWidget {
    Q_OBJECT
public:
    explicit TrajetMotor(ShapeVisualization* visu, QWidget* parent = nullptr);

    /// Démarre le parcours de découpe
    void executeTrajet();
    bool isPaused() const { return m_running && m_paused; }
    void setMainWindow(MainWindow* mainWindow);

public slots:
    /// Met en pause la découpe (le jet s’arrête, la boucle attend)
    void pause();
    /// Reprend après une pause
    void resume();
    /// Arrêt immédiat et définitif de la découpe en cours
    void stopCut();
    void moveHeadProgressive(const QPoint& start, const QPoint& end,QGraphicsEllipseItem* head, bool cut);

signals:
    /**
     * @param remaining  opérations restantes (déplacements + coupes)
     * @param total      nombre total d'opérations
     */
    void decoupeProgress(int remaining, int total);

private:
    ShapeVisualization* m_visu{};  // Vue pour le dessin
    MotorControl        m_motor;   // Simulation des moteurs

    bool m_paused        = false;  // Vrai si pause active
    bool m_stopRequested = false;  // Vrai si arrêt demandé
    bool m_running       = false;   // VRAI = une découpe est déjà en cours

    MainWindow* m_mainWindow = nullptr;

    int m_totalSteps = 0;         // nombre total d'étapes prévues
    int m_progressCounter = 0;    // étapes déjà effectuées
    bool m_interrupted = false;


};

#endif // TRAJETMOTOR_H
