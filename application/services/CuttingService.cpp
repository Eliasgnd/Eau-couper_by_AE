#include "CuttingService.h"
#include "ShapeVisualization.h"
#include "TrajetMotor.h"
#include "MachineViewModel.h"

#include <QChar>
#include <QCoreApplication>
#include <QDebug>
#include <QtGlobal>

namespace {
QString formatRemainingTime(qint64 remainingMs)
{
    const qint64 totalSeconds = qMax<qint64>(0, (remainingMs + 999) / 1000);
    const qint64 minutes = totalSeconds / 60;
    const qint64 seconds = totalSeconds % 60;

    if (minutes > 0) {
        return QCoreApplication::tr("Temps restant estime : %1 min %2 s")
            .arg(minutes)
            .arg(seconds, 2, 10, QChar('0'));
    }

    return QCoreApplication::tr("Temps restant estime : %1 s").arg(totalSeconds);
}
}

CuttingService::CuttingService(QObject *parent)
    : QObject(parent)
{
}

void CuttingService::connectMachineToMotor(MachineViewModel* vm)
{
    connect(vm, &MachineViewModel::realPositionReceived,
            m_trajetMotor, &TrajetMotor::onPositionUpdated, Qt::UniqueConnection);

    connect(vm, &MachineViewModel::doneReceived,
            m_trajetMotor, &TrajetMotor::onMachineDone, Qt::UniqueConnection);

    connect(vm, &MachineViewModel::segmentDone,
            m_trajetMotor, &TrajetMotor::onSegmentExecuted, Qt::UniqueConnection);

    connect(vm, &MachineViewModel::valveOnConfirmed,
            m_trajetMotor, &TrajetMotor::onValveOn, Qt::UniqueConnection);
    connect(vm, &MachineViewModel::valveOffConfirmed,
            m_trajetMotor, &TrajetMotor::onValveOff, Qt::UniqueConnection);
}

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

        connect(m_trajetMotor, &TrajetMotor::decoupeProgress, this,
                [this](int completed, int total, qint64 remainingMs) {
                    const int percent = (total > 0) ? (completed * 100) / total : 0;
                    emit progressUpdated(percent, formatRemainingTime(remainingMs));
                });

        connect(m_trajetMotor, &TrajetMotor::decoupeFinished, this, [this](bool success) {
            m_pauseRequested = false;
            if (m_visualization)
                m_visualization->setInteractionEnabled(true);
            emit controlsEnabledChanged(true);
            emit finished(success);
            emit progressUpdated(0, QCoreApplication::tr("Temps restant estime : 0 s"));
        });

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

void CuttingService::startCutting()
{
    if (!m_trajetMotor || !m_visualization)
        return;

    if (m_trajetMotor->isPaused()) {
        m_trajetMotor->resume();
        if (m_machineViewModel)
            m_machineViewModel->sendResume();
        m_pauseRequested = false;
        return;
    }

    qDebug() << "[CuttingService] Demarrage coupe - Vcut =" << m_cuttingSpeed << "mm/s";
    m_trajetMotor->setVcut(static_cast<double>(m_cuttingSpeed));

    m_visualization->resetAllShapeColors();
    if (!m_visualization->validateShapes()) {
        emit statusMessage(QCoreApplication::tr("Certaines formes depassent la zone ou se chevauchent."));
        emit finished(false);
        return;
    }

    emit controlsEnabledChanged(false);
    m_visualization->setInteractionEnabled(false);
    m_visualization->resetAllShapeColors();
    emit cuttingStarted();
    m_trajetMotor->executeTrajet();
}

void CuttingService::pauseCutting()
{
    if (!m_trajetMotor)
        return;

    if (!m_pauseRequested) {
        m_trajetMotor->pause();
        m_pauseRequested = true;
        if (m_machineViewModel)
            m_machineViewModel->sendPause();
    } else {
        m_trajetMotor->resume();
        m_pauseRequested = false;
        if (m_machineViewModel)
            m_machineViewModel->sendResume();
    }
}

void CuttingService::stopCutting()
{
    if (!m_trajetMotor)
        return;

    if (m_machineViewModel)
        m_machineViewModel->sendStop();

    m_trajetMotor->stopCut();

    if (m_visualization)
        m_visualization->setInteractionEnabled(true);
    emit controlsEnabledChanged(true);
    emit progressUpdated(0, QCoreApplication::tr("Temps restant estime : 0 s"));
}
