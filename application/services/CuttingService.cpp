#include "CuttingService.h"
#include "ShapeVisualization.h"
#include "TrajetMotor.h"
#include "MachineViewModel.h"

#include <QCoreApplication>
#include <QtGlobal>
#include <QDebug>

CuttingService::CuttingService(QObject *parent)
    : QObject(parent)
{
}

// ----------------------------------------------------------
//  Helper privé : branche les signaux MachineViewModel → TrajetMotor
// ----------------------------------------------------------

void CuttingService::connectMachineToMotor(MachineViewModel* vm)
{
    // =====================================================================
    // NOUVEAU TEMPS RÉEL : POS → mise à jour visuelle fluide de la tête
    // =====================================================================
    connect(vm, &MachineViewModel::realPositionReceived,
            m_trajetMotor, &TrajetMotor::onPositionUpdated, Qt::UniqueConnection);

    // DONE → débloque le thread worker qui attendait la fin
    connect(vm, &MachineViewModel::doneReceived,
            m_trajetMotor, &TrajetMotor::onMachineDone, Qt::UniqueConnection);

    // VALVE ON/OFF → couleur de tracé en temps réel (rouge = coupe, bleu = déplacement)
    connect(vm, &MachineViewModel::valveOnConfirmed,
            m_trajetMotor, &TrajetMotor::onValveOn, Qt::UniqueConnection);
    connect(vm, &MachineViewModel::valveOffConfirmed,
            m_trajetMotor, &TrajetMotor::onValveOff, Qt::UniqueConnection);
}

// ----------------------------------------------------------
//  Injection des dépendances
// ----------------------------------------------------------

void CuttingService::setMachineViewModel(MachineViewModel *vm)
{
    m_machineViewModel = vm;
    if (m_trajetMotor) {
        m_trajetMotor->setMachineViewModel(vm);
        connectMachineToMotor(vm);
    }
}

void CuttingService::initialize(ShapeVisualization *visualization, QWidget *dialogParent)
{
    m_visualization = visualization;

    if (!m_trajetMotor) {
        m_trajetMotor = new TrajetMotor(m_visualization, dialogParent);

        // Progression → mise à jour de la barre
        connect(m_trajetMotor, &TrajetMotor::decoupeProgress, this,
                [this](int remaining, int total) {
                    const int percent = (total > 0) ? ((total - remaining) * 100) / total : 0;
                    const int timeSec = (remaining * 15) / 1000;
                    emit progressUpdated(percent,
                                         QCoreApplication::tr("Temps restant estimé : %1s").arg(timeSec));
                });

        // Fin de découpe (succès ou interruption) → réactivation des contrôles
        connect(m_trajetMotor, &TrajetMotor::decoupeFinished, this, [this](bool success) {
            m_pauseRequested = false;
            if (m_visualization)
                m_visualization->setInteractionEnabled(true);
            emit controlsEnabledChanged(true);
            emit finished(success);
            emit progressUpdated(0, QCoreApplication::tr("Temps restant estimé : 0s"));
        });

        // Injecter le MachineViewModel si déjà disponible
        if (m_machineViewModel) {
            m_trajetMotor->setMachineViewModel(m_machineViewModel);
            connectMachineToMotor(m_machineViewModel);
        }
    }
}

void CuttingService::setCuttingSpeed(int speed_mm_s)
{
    m_cuttingSpeed = qBound(1, speed_mm_s, 200);
    if (m_trajetMotor)
        m_trajetMotor->setVcut(static_cast<double>(m_cuttingSpeed));
}

// ----------------------------------------------------------
//  Démarrage
// ----------------------------------------------------------

void CuttingService::startCutting()
{
    if (!m_trajetMotor || !m_visualization) return;

    if (m_trajetMotor->isPaused()) {
        // Reprise après pause
        m_trajetMotor->resume();
        if (m_machineViewModel) m_machineViewModel->sendResume();
        m_pauseRequested = false;
        return;
    }

    qDebug() << "[CuttingService] Démarrage coupe — Vcut =" << m_cuttingSpeed << "mm/s";
    m_trajetMotor->setVcut(static_cast<double>(m_cuttingSpeed));

    m_visualization->resetAllShapeColors();
    if (!m_visualization->validateShapes()) {
        emit statusMessage(QCoreApplication::tr("Certaines formes dépassent la zone ou se chevauchent."));
        emit finished(false);
        return;
    }

    emit controlsEnabledChanged(false);
    m_visualization->setInteractionEnabled(false);
    m_visualization->resetAllShapeColors();
    m_trajetMotor->executeTrajet();
}

// ----------------------------------------------------------
//  Pause / Reprise
// ----------------------------------------------------------

void CuttingService::pauseCutting()
{
    if (!m_trajetMotor) return;

    if (!m_pauseRequested) {
        // Mise en pause
        m_trajetMotor->pause();
        m_pauseRequested = true;
        if (m_machineViewModel) m_machineViewModel->sendPause();
    } else {
        // Reprise
        m_trajetMotor->resume();
        m_pauseRequested = false;
        if (m_machineViewModel) m_machineViewModel->sendResume();
    }
}

// ----------------------------------------------------------
//  Arrêt
// ----------------------------------------------------------

void CuttingService::stopCutting()
{
    if (!m_trajetMotor) return;

    // 1. Dire au STM d'arrêter immédiatement (vide le buffer)
    if (m_machineViewModel) m_machineViewModel->sendStop();

    // 2. Réveiller le thread worker (il verra m_stopRequested = true)
    m_trajetMotor->stopCut();

    // 3. Réactivation immédiate de l'UI (decoupeFinished viendra confirmer un peu après)
    if (m_visualization)
        m_visualization->setInteractionEnabled(true);
    emit controlsEnabledChanged(true);
    emit progressUpdated(0, QCoreApplication::tr("Temps restant estimé : 0s"));
    // Note : emit finished() sera déclenché par decoupeFinished
}
