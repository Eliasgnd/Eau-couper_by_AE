#include "motorcontrol.h"
#include <QtMath>

MotorControl::MotorControl(QObject* p):QObject(p){}

void MotorControl::startJet(){ qDebug() << "JET ON"; }
void MotorControl::stopJet() { qDebug() << "JET OFF"; }

static int toSteps(double mm, double mmPerStep=0.1) {
    return qRound(mm / mmPerStep); // TODO : calibrer mmParStep
}

void MotorControl::moveRapid(double x, double y) {
    qDebug() << "RAPID " << x << y;
    stepsX += toSteps(x);
    stepsY += toSteps(y);
}

void MotorControl::moveCut(double x, double y) {
    qDebug() << "CUT   " << x << y;
    stepsX += toSteps(x);
    stepsY += toSteps(y);
}
