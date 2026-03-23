#include "DrawModeManager.h"

DrawModeManager::DrawModeManager(QObject *parent)
    : QObject(parent)
{
}

DrawModeManager::DrawMode DrawModeManager::drawMode() const { return m_drawMode; }
DrawModeManager::DrawMode DrawModeManager::lastPrimaryMode() const { return m_lastPrimaryMode; }

void DrawModeManager::setDrawMode(DrawMode mode)
{
    m_drawMode = mode;
    if (mode == DrawMode::Freehand || mode == DrawMode::PointParPoint || mode == DrawMode::Line ||
        mode == DrawMode::Circle || mode == DrawMode::Rectangle || mode == DrawMode::Text ||
        mode == DrawMode::ThinText) {
        m_lastPrimaryMode = mode;
    }
    emit drawModeChanged(m_drawMode);
}

void DrawModeManager::restorePreviousMode() { setDrawMode(m_lastPrimaryMode); }

void DrawModeManager::startCloseMode() { m_closeMode = true; emit closeModeChanged(true); }
void DrawModeManager::cancelCloseMode() { m_closeMode = false; emit closeModeChanged(false); }
void DrawModeManager::startDeplacerMode() { m_deplacerMode = true; emit deplacerModeChanged(true); }
void DrawModeManager::cancelDeplacerMode() { m_deplacerMode = false; emit deplacerModeChanged(false); }
void DrawModeManager::startSupprimerMode() { m_supprimerMode = true; emit supprimerModeChanged(true); }
void DrawModeManager::cancelSupprimerMode() { m_supprimerMode = false; emit supprimerModeChanged(false); }
void DrawModeManager::startGommeMode() { m_gommeMode = true; emit gommeModeChanged(true); }
void DrawModeManager::cancelGommeMode() { m_gommeMode = false; emit gommeModeChanged(false); }
