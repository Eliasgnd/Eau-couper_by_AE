#ifndef GPIOCONTROLPAGE_H
#define GPIOCONTROLPAGE_H

#include <QWidget>
#include <QVector>
#include <QLabel>
#include <QPushButton>
#include <gpiod.h>

namespace Ui {
class GpioControlPage;
}

class GpioControlPage : public QWidget
{
    Q_OBJECT
public:
    explicit GpioControlPage(QWidget *parent = nullptr);
    ~GpioControlPage();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onBackClicked();
    void onToggleClicked(int index);

private:
    void initPins();
    void releasePins();
    void updatePinState(int index);

    struct PinInfo {
        int number;
        QPushButton *toggleButton;
        QLabel *stateLabel;
        gpiod_line *line;
    };

    QVector<PinInfo> m_pins;
    gpiod_chip *m_chip = nullptr;
    Ui::GpioControlPage *ui;
};

#endif // GPIOCONTROLPAGE_H
