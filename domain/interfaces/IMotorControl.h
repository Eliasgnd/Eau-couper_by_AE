#pragma once

class IMotorControl
{
public:
    virtual ~IMotorControl() = default;

    virtual void startJet() = 0;
    virtual void stopJet() = 0;
    virtual void moveRapid(double x, double y) = 0;
    virtual void moveCut(double x, double y) = 0;
};
