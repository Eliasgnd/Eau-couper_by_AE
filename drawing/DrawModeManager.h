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

    void setDrawMode(DrawMode mode);
    DrawMode drawMode() const;
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
    DrawMode m_currentMode = DrawMode::Freehand;
    DrawMode m_lastPrimaryMode = DrawMode::Freehand;
};

#endif // DRAWMODEMANAGER_H
