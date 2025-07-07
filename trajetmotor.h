#ifndef TRAJETMOTOR_H
#define TRAJETMOTOR_H

#include <QWidget>
#include "motorcontrol.h"
#include "FormeVisualization.h" //rudy le boss

class TrajetMotor : public QWidget {
    Q_OBJECT
public:
    explicit TrajetMotor(FormeVisualization* visu, QWidget* parent = nullptr);

    /// Démarre le parcours de découpe
    void executeTrajet();
    bool isPaused() const { return m_running && m_paused; }

public slots:
    /// Met en pause la découpe (le jet s’arrête, la boucle attend)
    void pause();
    /// Reprend après une pause
    void resume();
    /// Arrêt immédiat et définitif de la découpe en cours
    void stopCut();
    void moveHeadProgressive(const QPoint& start, const QPoint& end, QGraphicsEllipseItem* head);

signals:
    /**
     * @param remaining  segments restants à découper
     * @param total      nombre total de segments
     */
    void decoupeProgress(int remaining, int total);

private:
    FormeVisualization* m_visu{};  // Vue pour le dessin
    MotorControl        m_motor;   // Simulation des moteurs

    bool m_paused        = false;  // Vrai si pause active
    bool m_stopRequested = false;  // Vrai si arrêt demandé
    bool m_running       = false;   // VRAI = une découpe est déjà en cours

};

#endif // TRAJETMOTOR_H
