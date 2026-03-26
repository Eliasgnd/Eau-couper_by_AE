#pragma once
#include <QObject>
#include <QPoint>
#include <QDebug>
#include "IMotorControl.h"

class MotorControl : public QObject, public IMotorControl {
    Q_OBJECT
public:
    explicit MotorControl(QObject* parent=nullptr);

    // Vitesses (mm/s) – ajustez selon votre machine
    double Vcut  = 10.0;   // vitesse de coupe
    double Vtrav = 200.0;  // vitesse de déplacement à vide

    void startJet() override;
    void stopJet() override;

    //! Déplacement à vide (jet OFF) – coordonnées absolues en mm
    void moveRapid(double x, double y) override;

    //! Déplacement avec coupe (jet ON)
    void moveCut(double x, double y) override;

    int getStepsX() const { return stepsX; }
    int getStepsY() const { return stepsY; }

private:
    int stepsX = 0, stepsY = 0;
};
