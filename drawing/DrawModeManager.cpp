#include "DrawModeManager.h"

DrawModeManager::DrawModeManager(QObject *parent)
    : QObject(parent)
{}

// ---------------------------------------------------------------------------
// Primary draw mode
// ---------------------------------------------------------------------------

void DrawModeManager::setDrawMode(DrawMode mode)
{
    const bool wasDeplacer  = (m_currentMode == DrawMode::Deplacer);
    const bool wasSupprimer = (m_currentMode == DrawMode::Supprimer);
    const bool wasGomme     = (m_currentMode == DrawMode::Gomme);

    m_currentMode = mode;

    if (mode == DrawMode::Freehand || mode == DrawMode::PointParPoint ||
        mode == DrawMode::Line     || mode == DrawMode::Circle        ||
        mode == DrawMode::Rectangle|| mode == DrawMode::Text          ||
        mode == DrawMode::ThinText)
    {
        m_lastPrimaryMode = mode;
    }

    const bool isDeplacer  = (m_currentMode == DrawMode::Deplacer);
    const bool isSupprimer = (m_currentMode == DrawMode::Supprimer);
    const bool isGomme     = (m_currentMode == DrawMode::Gomme);

    if (wasDeplacer  != isDeplacer)  emit deplacerModeChanged(isDeplacer);
    if (wasSupprimer != isSupprimer) emit supprimerModeChanged(isSupprimer);
    if (wasGomme     != isGomme)     emit gommeModeChanged(isGomme);

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

// ---------------------------------------------------------------------------
// Overlay: Close shape
// ---------------------------------------------------------------------------

void DrawModeManager::startCloseMode()
{
    if (m_closeMode) return;
    m_closeMode = true;
    emit closeModeChanged(true);
}

void DrawModeManager::cancelCloseMode()
{
    if (!m_closeMode) return;
    m_closeMode = false;
    emit closeModeChanged(false);
}

bool DrawModeManager::isCloseMode() const { return m_closeMode; }

// ---------------------------------------------------------------------------
// Primary modes: Deplacer / Supprimer / Gomme
// ---------------------------------------------------------------------------

void DrawModeManager::startDeplacerMode()  { setDrawMode(DrawMode::Deplacer); }
void DrawModeManager::cancelDeplacerMode()
{
    if (m_currentMode == DrawMode::Deplacer) restorePreviousMode();
}

void DrawModeManager::startSupprimerMode() { setDrawMode(DrawMode::Supprimer); }
void DrawModeManager::cancelSupprimerMode()
{
    if (m_currentMode == DrawMode::Supprimer) restorePreviousMode();
}

void DrawModeManager::startGommeMode()     { setDrawMode(DrawMode::Gomme); }
void DrawModeManager::cancelGommeMode()
{
    if (m_currentMode == DrawMode::Gomme) restorePreviousMode();
}

// ---------------------------------------------------------------------------
// Overlay: Selection modes
// ---------------------------------------------------------------------------

void DrawModeManager::startSelectConnect()
{
    cancelAnySelection();
    m_selectMode  = true;
    m_connectMode = true;
    emit shapeSelectionChanged(true);
}

void DrawModeManager::cancelSelectConnect()
{
    if (!m_selectMode || !m_connectMode) return;
    m_selectMode  = false;
    m_connectMode = false;
    emit shapeSelectionChanged(false);
}

void DrawModeManager::startSelectMulti()
{
    cancelAnySelection();
    m_selectMode  = true;
    m_connectMode = false;
    emit multiSelectionChanged(true);
}

void DrawModeManager::cancelSelectMulti()
{
    if (!m_selectMode || m_connectMode) return;
    m_selectMode = false;
    emit multiSelectionChanged(false);
}

void DrawModeManager::cancelAnySelection()
{
    if (!m_selectMode) return;
    const bool wasConnect = m_connectMode;
    m_selectMode  = false;
    m_connectMode = false;
    if (wasConnect) emit shapeSelectionChanged(false);
    else            emit multiSelectionChanged(false);
}

bool DrawModeManager::isSelectMode()     const { return m_selectMode; }
bool DrawModeManager::isConnectMode()    const { return m_selectMode && m_connectMode; }
bool DrawModeManager::isMultiSelectMode()const { return m_selectMode && !m_connectMode; }

// ---------------------------------------------------------------------------
// Overlay: Paste
// ---------------------------------------------------------------------------

void DrawModeManager::enablePasteMode()
{
    m_pasteMode = true;
}

void DrawModeManager::cancelPasteMode()
{
    m_pasteMode = false;
}

bool DrawModeManager::isPasteMode() const { return m_pasteMode; }
