#pragma once
#include <QObject>
#include <QPoint>
#include <QDebug>

class MotorControl : public QObject {
    Q_OBJECT
public:
    explicit MotorControl(QObject* parent=nullptr);

    // Vitesses (mm/s) – ajustez selon votre machine
    double Vcut  = 10.0;   // vitesse de coupe
    double Vtrav = 200.0;  // vitesse de déplacement à vide

    void startJet();
    void stopJet();

    //! Déplacement à vide (jet OFF) – coordonnées absolues en mm
    void moveRapid(double x, double y);

    //! Déplacement avec coupe (jet ON)
    void moveCut(double x, double y);

    int getStepsX() const { return stepsX; }
    int getStepsY() const { return stepsY; }

private:
    int stepsX = 0, stepsY = 0;
};
