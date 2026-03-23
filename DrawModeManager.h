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

    DrawMode drawMode() const;
    DrawMode lastPrimaryMode() const;
    void setDrawMode(DrawMode mode);
    void restorePreviousMode();

    void startCloseMode();
    void cancelCloseMode();
    void startDeplacerMode();
    void cancelDeplacerMode();
    void startSupprimerMode();
    void cancelSupprimerMode();
    void startGommeMode();
    void cancelGommeMode();

signals:
    void drawModeChanged(DrawMode mode);
    void closeModeChanged(bool enabled);
    void deplacerModeChanged(bool enabled);
    void supprimerModeChanged(bool enabled);
    void gommeModeChanged(bool enabled);

private:
    DrawMode m_drawMode = DrawMode::Freehand;
    DrawMode m_lastPrimaryMode = DrawMode::Freehand;
    bool m_closeMode = false;
    bool m_deplacerMode = false;
    bool m_supprimerMode = false;
    bool m_gommeMode = false;
};

#endif // DRAWMODEMANAGER_H
