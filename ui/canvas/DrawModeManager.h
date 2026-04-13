#ifndef DRAWMODEMANAGER_H
#define DRAWMODEMANAGER_H

#include <QObject>

class DrawModeManager : public QObject
{
    Q_OBJECT
public:
    enum class DrawMode {
        Freehand,
        PointParPoint,
        Line,
        Circle,
        Rectangle,
        Supprimer,
        Gomme,
        Deplacer,
        Pan,
        Text,
        ThinText
    };
    Q_ENUM(DrawMode)

    explicit DrawModeManager(QObject *parent = nullptr);

    // --- Primary draw mode ---
    void     setDrawMode(DrawMode mode);
    DrawMode drawMode() const;
    void     restorePreviousMode();

    // --- Overlay mode: Close shape ---
    void startCloseMode();
    void cancelCloseMode();
    bool isCloseMode() const;

    // --- Overlay mode: Deplacer / Supprimer / Gomme (primary modes) ---
    void startDeplacerMode();
    void cancelDeplacerMode();
    void startSupprimerMode();
    void cancelSupprimerMode();
    void startGommeMode();
    void cancelGommeMode();

    // --- Overlay mode: Selection (Connect or Multi) ---
    void startSelectConnect();   // sélection pour relier 2 formes
    void cancelSelectConnect();
    void startSelectMulti();     // sélection multiple libre
    void cancelSelectMulti();
    void cancelAnySelection();   // annule quelle que soit la sélection active

    bool isSelectMode()    const;  // true si SelectConnect ou SelectMulti est actif
    bool isConnectMode()   const;  // true uniquement en SelectConnect
    bool isMultiSelectMode() const;// true uniquement en SelectMulti

    // --- Overlay mode: Paste ---
    void enablePasteMode();
    void cancelPasteMode();
    bool isPasteMode() const;

signals:
    void drawModeChanged(DrawMode mode);
    void closeModeChanged(bool enabled);
    void deplacerModeChanged(bool enabled);
    void supprimerModeChanged(bool enabled);
    void gommeModeChanged(bool enabled);
    // Selection overlays
    void shapeSelectionChanged(bool enabled);    // SelectConnect on/off
    void multiSelectionChanged(bool enabled);    // SelectMulti on/off

private:
    DrawMode m_currentMode      = DrawMode::Freehand;
    DrawMode m_lastPrimaryMode  = DrawMode::Freehand;

    // Overlay states
    bool m_closeMode    = false;
    bool m_selectMode   = false;
    bool m_connectMode  = false; // valid only when m_selectMode
    bool m_pasteMode    = false;
};

#endif // DRAWMODEMANAGER_H
