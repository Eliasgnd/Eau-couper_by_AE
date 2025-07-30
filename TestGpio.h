#ifndef TESTGPIO_H
#define TESTGPIO_H

#include <QWidget>
#include <QMap>
#include <QTimer>
#include <QCheckBox>
#include <QLabel>
#include "Language.h"

#ifndef _WIN32
#include <gpiod.h>
#endif

QT_BEGIN_NAMESPACE
namespace Ui { class TestGpio; }
QT_END_NAMESPACE

class TestGpio : public QWidget
{
    Q_OBJECT
public:
    explicit TestGpio(QWidget *parent = nullptr);
    ~TestGpio() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onPinToggled(bool checked);
    void updatePinStates();
    void goToMainWindow();


private:
    void init_gpio();
    void releaseGpio();
    void setupUiPins();

    Ui::TestGpio *ui {nullptr};
    QVector<int> outputPins {20,21,26,13,6,7,8,25,17,27};
    QVector<int> inputPins {23,24,18};
    QMap<int, QLabel*> stateLabels;
    QMap<int, QCheckBox*> checkBoxes;
    QMap<int, QLabel*> inputStateLabels;
    QTimer updateTimer;


#ifndef _WIN32
    gpiod_chip *chip {nullptr};
    QMap<int, gpiod_line*> outputLines;
    QMap<int, gpiod_line*> inputLines;
#endif
};

#endif // TESTGPIO_H
