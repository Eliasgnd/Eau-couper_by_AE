#include "DrawModeManager.h"

DrawModeManager::DrawModeManager(QObject *parent)
    : QObject(parent)
{
}

void DrawModeManager::setDrawMode(DrawMode mode)
{
    const bool wasDeplacer = (m_currentMode == DrawMode::Deplacer);
    const bool wasSupprimer = (m_currentMode == DrawMode::Supprimer);
    const bool wasGomme = (m_currentMode == DrawMode::Gomme);

    m_currentMode = mode;
    if (mode == DrawMode::Freehand || mode == DrawMode::PointParPoint || mode == DrawMode::Line ||
        mode == DrawMode::Circle || mode == DrawMode::Rectangle || mode == DrawMode::Text ||
        mode == DrawMode::ThinText) {
        m_lastPrimaryMode = mode;
    }

    const bool isDeplacer = (m_currentMode == DrawMode::Deplacer);
    const bool isSupprimer = (m_currentMode == DrawMode::Supprimer);
    const bool isGomme = (m_currentMode == DrawMode::Gomme);

    if (wasDeplacer != isDeplacer) emit deplacerModeChanged(isDeplacer);
    if (wasSupprimer != isSupprimer) emit supprimerModeChanged(isSupprimer);
    if (wasGomme != isGomme) emit gommeModeChanged(isGomme);

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
void DrawModeManager::startDeplacerMode() { setDrawMode(DrawMode::Deplacer); }
void DrawModeManager::cancelDeplacerMode()
{
    if (m_currentMode == DrawMode::Deplacer) restorePreviousMode();
}
void DrawModeManager::startSupprimerMode() { setDrawMode(DrawMode::Supprimer); }
void DrawModeManager::cancelSupprimerMode()
{
    if (m_currentMode == DrawMode::Supprimer) restorePreviousMode();
}
void DrawModeManager::startGommeMode() { setDrawMode(DrawMode::Gomme); }
void DrawModeManager::cancelGommeMode()
{
    if (m_currentMode == DrawMode::Gomme) restorePreviousMode();
}
