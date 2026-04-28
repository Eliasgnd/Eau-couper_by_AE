#ifndef MOUSEINTERACTIONHANDLER_H
#define MOUSEINTERACTIONHANDLER_H

#include <QObject>
#include <QPointF>

class QMouseEvent;
class DrawModeManager;
class ShapeManager;
class ViewTransformer;
struct DrawingState;

/**
 * @class MouseInteractionHandler
 * @brief Gère les interactions souris bas niveau (pan, déplacement, suppression).
 *
 * @note Cette classe délègue les modifications de données à ShapeManager et écrit
 *       la position courante dans le DrawingState partagé avec CustomDrawArea.
 * @see CustomDrawArea, DrawingState
 */
class MouseInteractionHandler : public QObject
{
    Q_OBJECT
public:
    explicit MouseInteractionHandler(ShapeManager    *shapeManager,
                                     DrawModeManager *modeManager,
                                     ViewTransformer *transformer,
                                     DrawingState    *state,
                                     QObject         *parent = nullptr);

    void handleMousePress  (QMouseEvent *event, const QPointF &logicalPos);
    void handleMouseMove   (QMouseEvent *event, const QPointF &logicalPos);
    void handleMouseRelease(QMouseEvent *event, const QPointF &logicalPos);
    bool isSelectionDragActive() const;

signals:
    void requestUpdate();

private:
    ShapeManager    *m_shapeManager = nullptr;
    DrawModeManager *m_modeManager  = nullptr;
    ViewTransformer *m_transformer  = nullptr;
    DrawingState    *m_state        = nullptr;  // non-owning, appartient à CustomDrawArea

    // Sémantique différente de CustomDrawArea::m_drawing :
    // true = le handler traite les mouvements souris (pan, déplacement).
    bool m_active = false;
    bool m_draggingSelection = false;
    bool m_selectionDragMoved = false;
    int  m_pressedSelectedIndex = -1;
};

#endif // MOUSEINTERACTIONHANDLER_H
