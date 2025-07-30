#include "TestGpio.h"
#include "ui_TestGpio.h"
#include "MainWindow.h"
#include "ScreenUtils.h"
#include "raspberry.h"
#include <QCheckBox>
#include <QLabel>
#include <QGridLayout>
#include <QCloseEvent>
#include <QDebug>
#include <QTimer>

TestGpio::TestGpio(QWidget *parent)
    : QWidget(parent), ui(new Ui::TestGpio)
{
    ui->setupUi(this);
    setupUiPins();
    for(int pin: inputPins){
        QLabel *label = findChild<QLabel*>(QString("labelPin%1").arg(pin));
        if(label)
            inputStateLabels.insert(pin, label);
    }
    init_gpio();

    // Création et initialisation du pilote
    Raspberry* rasp = new Raspberry(this);
    if (!rasp->init()) {
        qWarning() << "Échec init Raspberry (GPIO/SPI)";
    }
    connect(rasp, &Raspberry::spiTransfered,
            this, &TestGpio::appendSpiLog);

    // Timer pour déclencher régulièrement une lecture SPI
    QTimer* spiTimer = new QTimer(this);
    connect(spiTimer, &QTimer::timeout, this, [this, rasp]() {
        // Double-phase read pour DRV8711 : commande de lecture puis lecture de la donnée
        rasp->transfer(0x8000);                    // envoie le bit R/W=1, addr=0
        uint16_t status = rasp->transfer(0x0000);  // récupère la valeur du registre
        ui->spiLog->appendPlainText(
            QString("STATUS=0x%1").arg(status, 3, 16, QChar('0')));
    });
    spiTimer->start(1000); // toutes les secondes

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

void TestGpio::init_gpio()
{
#ifndef _WIN32
    chip = gpiod_chip_open_by_name("gpiochip0");
    if(!chip){
        qWarning() << "Impossible d'ouvrir gpiochip0";
        return;
    }
    for(int pin: outputPins){
        gpiod_line *line = gpiod_chip_get_line(chip, pin);
        if(line && gpiod_line_request_output_flags(line, "testgpio", 0, 0)==0){
            outputLines.insert(pin, line);
        } else {
            qWarning() << "Erreur init GPIO sortie" << pin;
        }
    }
    for(int pin: inputPins){
        gpiod_line *line = gpiod_chip_get_line(chip, pin);
        if(line && gpiod_line_request_input_flags(line, "testgpio", GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLE)==0){
            inputLines.insert(pin, line);
        } else {
            qWarning() << "Erreur init GPIO entree" << pin;
        }
    }
#endif
}

void TestGpio::releaseGpio()
{
#ifndef _WIN32
    for(auto it=outputLines.begin(); it!=outputLines.end(); ++it){
        gpiod_line_set_value(it.value(), 0);
        gpiod_line_release(it.value());
    }
    outputLines.clear();
    for(auto it=inputLines.begin(); it!=inputLines.end(); ++it){
        gpiod_line_release(it.value());
    }
    inputLines.clear();
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
    for(int pin: outputPins){
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
    if(outputLines.contains(pin))
        gpiod_line_set_value(outputLines.value(pin), checked?1:0);
#endif
    if(stateLabels.contains(pin))
        stateLabels.value(pin)->setText(checked?"HIGH":"LOW");
}

void TestGpio::updatePinStates()
{
#ifndef _WIN32
    for(int pin: outputPins){
        if(!outputLines.contains(pin)) continue;
        int val = gpiod_line_get_value(outputLines.value(pin));
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
    for(int pin: inputPins){
        if(!inputLines.contains(pin)) continue;
        int val = gpiod_line_get_value(inputLines.value(pin));
        if(val<0) continue;
        QLabel *lbl = inputStateLabels.value(pin);
        static QMap<int, QString> names = {
            {Raspberry::STALLN_PIN, "STALLn"},
            {Raspberry::FAULTN_PIN, "FAULTn"},
            {Raspberry::BEMF_PIN, "BEMF"}
        };
        if(lbl) lbl->setText(QString("%1 (GPIO %2): %3")
                             .arg(names.value(pin))
                             .arg(pin)
                             .arg(val ? "HIGH" : "LOW"));
    }
#endif
}

void TestGpio::appendSpiLog(quint16 tx, quint16 rx)
{
    ui->spiLog->appendPlainText(
        QString("TX=0x%1  RX=0x%2")
            .arg(tx, 4, 16, QChar('0'))
            .arg(rx, 4, 16, QChar('0')));
}

void TestGpio::goToMainWindow()
{
    this->close();
    MainWindow::getInstance()->showFullScreen();
}
