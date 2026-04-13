#include "CuttingService.h"
#include "ShapeVisualization.h"
#include "TrajetMotor.h"

#include <QCoreApplication>

CuttingService::CuttingService(QObject *parent)
    : QObject(parent)
{
}

void CuttingService::initialize(ShapeVisualization *visualization, QWidget *dialogParent)
{
    m_visualization = visualization;

    if (!m_trajetMotor) {
        m_trajetMotor = new TrajetMotor(m_visualization, dialogParent);

        connect(m_trajetMotor, &TrajetMotor::decoupeProgress, this,
                [this](int remaining, int total) {
                    const int percent = (total > 0) ? ((total - remaining) * 100) / total : 0;
                    const int timeSec = (remaining * 15) / 1000;
                    emit progressUpdated(percent,
                                        QCoreApplication::tr("Temps restant estimé : %1s").arg(timeSec));
                });
    }
}

void CuttingService::startCutting()
{
    if (!m_trajetMotor || !m_visualization) return;

    if (m_trajetMotor->isPaused()) {
        m_trajetMotor->resume();
        m_pauseRequested = false;
        return;
    }

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

void CuttingService::pauseCutting()
{
    if (!m_trajetMotor) return;

    if (!m_pauseRequested) {
        m_trajetMotor->pause();
        m_pauseRequested = true;
    } else {
        m_trajetMotor->resume();
        m_pauseRequested = false;
    }
}

void CuttingService::stopCutting()
{
    if (!m_trajetMotor) return;

    m_trajetMotor->stopCut();
    if (m_visualization)
        m_visualization->setInteractionEnabled(true);

    emit progressUpdated(0, QCoreApplication::tr("Temps restant estimé : 0s"));
    emit controlsEnabledChanged(true);
    emit finished(true);
}
