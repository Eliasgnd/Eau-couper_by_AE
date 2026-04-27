#pragma once

#include <QObject>
#include <QString>

class ShapeVisualization;
class TrajetMotor;
class MachineViewModel;
class QWidget;

// Service responsable de l'exécution de la séquence de découpe avec le moteur.
// Propriétaire du TrajetMotor. Reçoit ShapeVisualization via initialize().
class CuttingService : public QObject
{
    Q_OBJECT

public:
    explicit CuttingService(QObject *parent = nullptr);

    // Initialise le service avec les dépendances UI nécessaires.
    // Doit être appelé avant startCutting().
    void initialize(ShapeVisualization *visualization, QWidget *dialogParent);

    // Injecte le MachineViewModel pour la communication UART avec le STM32.
    void setMachineViewModel(MachineViewModel *vm);

public slots:
    void startCutting();
    void pauseCutting();
    void stopCutting();
    void setCuttingSpeed(int speed_mm_s);  // Vitesse de coupe en mm/s (1–200)

signals:
    void progressUpdated(int percent, const QString &timeRemaining);
    void finished(bool success);
    void controlsEnabledChanged(bool enabled);
    void statusMessage(const QString &message);
    void cuttingStarted();

private:
    void connectMachineToMotor(MachineViewModel* vm);

    ShapeVisualization *m_visualization    = nullptr;
    TrajetMotor        *m_trajetMotor      = nullptr;
    MachineViewModel   *m_machineViewModel = nullptr;
    bool                m_pauseRequested   = false;
    int                 m_cuttingSpeed     = 10;   // mm/s, par défaut (= MotorControl::Vcut)
};
