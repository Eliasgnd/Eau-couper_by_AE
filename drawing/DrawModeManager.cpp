#include "DrawModeManager.h"

DrawModeManager::DrawModeManager(QObject *parent)
    : QObject(parent)
{
}

void DrawModeManager::setDrawMode(DrawMode mode)
{
    m_currentMode = mode;
    if (mode == DrawMode::Freehand || mode == DrawMode::PointParPoint || mode == DrawMode::Line ||
        mode == DrawMode::Circle || mode == DrawMode::Rectangle || mode == DrawMode::Text ||
        mode == DrawMode::ThinText) {
        m_lastPrimaryMode = mode;
    }
    emit drawModeChanged(m_currentMode);
}

DrawModeManager::DrawMode DrawModeManager::drawMode() const
{
    return m_currentMode;
}

void DrawModeManager::restorePreviousMode()
{
    setDrawMode(m_lastPrimaryMode);
}

void DrawModeManager::startCloseMode() { emit closeModeChanged(true); }
void DrawModeManager::cancelCloseMode() { emit closeModeChanged(false); }
void DrawModeManager::startDeplacerMode() { setDrawMode(DrawMode::Deplacer); emit deplacerModeChanged(true); }
void DrawModeManager::cancelDeplacerMode() { emit deplacerModeChanged(false); restorePreviousMode(); }
void DrawModeManager::startSupprimerMode() { setDrawMode(DrawMode::Supprimer); emit supprimerModeChanged(true); }
void DrawModeManager::cancelSupprimerMode() { emit supprimerModeChanged(false); restorePreviousMode(); }
void DrawModeManager::startGommeMode() { setDrawMode(DrawMode::Gomme); emit gommeModeChanged(true); }
void DrawModeManager::cancelGommeMode() { emit gommeModeChanged(false); restorePreviousMode(); }
