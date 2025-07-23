#include "TestGpio.h"
#include "ui_TestGpio.h"
#include "MainWindow.h"
#include "ScreenUtils.h"
#include <QCheckBox>
#include <QLabel>
#include <QGridLayout>
#include <QCloseEvent>
#include <QDebug>

TestGpio::TestGpio(QWidget *parent)
    : QWidget(parent), ui(new Ui::TestGpio)
{
    ui->setupUi(this);
    pins.reserve(26);
    for(int p=2;p<=27;++p)
        pins.append(p);

    setupUiPins();
    initGpio();

    ScreenUtils::placeOnSecondaryScreen(this);

    connect(&updateTimer, &QTimer::timeout, this, &TestGpio::updatePinStates);
    updateTimer.start(500);

    connect(ui->buttonMenu, &QPushButton::clicked, this, &TestGpio::goToMainWindow);
}

TestGpio::~TestGpio()
{
    releaseGpio();
    delete ui;
}

void TestGpio::closeEvent(QCloseEvent *event)
{
    releaseGpio();
    QWidget::closeEvent(event);
}

void TestGpio::initGpio()
{
#ifndef _WIN32
    chip = gpiod_chip_open_by_name("gpiochip0");
    if(!chip){
        qWarning() << "Impossible d'ouvrir gpiochip0";
        return;
    }
    for(int pin: pins){
        gpiod_line *line = gpiod_chip_get_line(chip, pin);
        if(line && gpiod_line_request_output(line, "testgpio", 0)==0){
            lines.insert(pin, line);
        } else {
            qWarning() << "Erreur init GPIO" << pin;
        }
    }
#endif
}

void TestGpio::releaseGpio()
{
#ifndef _WIN32
    for(auto it=lines.begin(); it!=lines.end(); ++it){
        gpiod_line_set_value(it.value(), 0);
        gpiod_line_release(it.value());
    }
    lines.clear();
    if(chip){
        gpiod_chip_close(chip);
        chip = nullptr;
    }
#endif
}

void TestGpio::setupUiPins()
{
    QGridLayout *grid = ui->gridLayoutPins;
    int row=0;
    for(int pin: pins){
        QLabel *lblNum = new QLabel(QString::number(pin), this);
        QLabel *lblState = new QLabel("LOW", this);
        QCheckBox *cb = new QCheckBox(this);
        grid->addWidget(lblNum, row, 0);
        grid->addWidget(lblState, row, 1);
        grid->addWidget(cb, row, 2);
        stateLabels.insert(pin, lblState);
        checkBoxes.insert(pin, cb);
        cb->setProperty("gpio", pin);
        connect(cb, &QCheckBox::toggled, this, &TestGpio::onPinToggled);
        row++;
    }
    grid->setColumnStretch(1,1);
}

void TestGpio::onPinToggled(bool checked)
{
    QCheckBox *cb = qobject_cast<QCheckBox*>(sender());
    if(!cb) return;
    int pin = cb->property("gpio").toInt();
#ifndef _WIN32
    if(lines.contains(pin))
        gpiod_line_set_value(lines.value(pin), checked?1:0);
#endif
    if(stateLabels.contains(pin))
        stateLabels.value(pin)->setText(checked?"HIGH":"LOW");
}

void TestGpio::updatePinStates()
{
#ifndef _WIN32
    for(int pin: pins){
        if(!lines.contains(pin)) continue;
        int val = gpiod_line_get_value(lines.value(pin));
        if(val<0) continue;
        QLabel *label = stateLabels.value(pin);
        QCheckBox *cb   = checkBoxes.value(pin);
        bool state = val>0;
        if(cb){
            cb->blockSignals(true);
            cb->setChecked(state);
            cb->blockSignals(false);
        }
        if(label) label->setText(state?"HIGH":"LOW");
    }
#endif
}

void TestGpio::goToMainWindow()
{
    this->close();
    MainWindow::getInstance()->showFullScreen();
}

