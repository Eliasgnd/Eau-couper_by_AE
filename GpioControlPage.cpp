#include "GpioControlPage.h"
#include "ui_GpioControlPage.h"
#include "MainWindow.h"
#include "ScreenUtils.h"

#include <QCloseEvent>
#include <QDebug>

GpioControlPage::GpioControlPage(QWidget *parent)
    : QWidget(parent), ui(new Ui::GpioControlPage)
{
    ui->setupUi(this);
    ScreenUtils::placeOnSecondaryScreen(this);

    connect(ui->buttonBack, &QPushButton::clicked, this, &GpioControlPage::onBackClicked);

    m_chip = gpiod_chip_open("/dev/gpiochip0");
    if (!m_chip) {
        qWarning() << "Failed to open gpiochip0";
    }

    initPins();
}

GpioControlPage::~GpioControlPage()
{
    releasePins();
    delete ui;
}

void GpioControlPage::closeEvent(QCloseEvent *event)
{
    releasePins();
    QWidget::closeEvent(event);
}

void GpioControlPage::onBackClicked()
{
    close();
    MainWindow::getInstance()->showFullScreen();
}

void GpioControlPage::initPins()
{
    const int startPin = 2;
    const int endPin = 27;
    int row = 0;
    for (int pin = startPin; pin <= endPin; ++pin) {
        QLabel *lbl = new QLabel(QString("GPIO %1").arg(pin));
        QPushButton *btn = new QPushButton(tr("Activer"));
        btn->setCheckable(true);
        QLabel *state = new QLabel("LOW");

        ui->gridLayout->addWidget(lbl, row, 0);
        ui->gridLayout->addWidget(btn, row, 1);
        ui->gridLayout->addWidget(state, row, 2);

        gpiod_line *line = nullptr;
        if (m_chip) {
            line = gpiod_chip_get_line(m_chip, pin);
            if (line && gpiod_line_request_output(line, "machineDecoupe", 0) != 0) {
                qWarning() << "Cannot request line" << pin;
                line = nullptr;
            }
        }

        PinInfo info{pin, btn, state, line};
        m_pins.append(info);

        connect(btn, &QPushButton::clicked, this, [this, row]() { onToggleClicked(row); });

        updatePinState(row);
        ++row;
    }
}

void GpioControlPage::releasePins()
{
    for (PinInfo &p : m_pins) {
        if (p.line) {
            gpiod_line_release(p.line);
            p.line = nullptr;
        }
    }
    if (m_chip) {
        gpiod_chip_close(m_chip);
        m_chip = nullptr;
    }
}

void GpioControlPage::onToggleClicked(int index)
{
    if (index < 0 || index >= m_pins.size())
        return;
    PinInfo &p = m_pins[index];
    if (!p.line)
        return;
    int val = gpiod_line_get_value(p.line);
    val = !val;
    gpiod_line_set_value(p.line, val);
    updatePinState(index);
}

void GpioControlPage::updatePinState(int index)
{
    if (index < 0 || index >= m_pins.size())
        return;
    PinInfo &p = m_pins[index];
    int val = 0;
    if (p.line)
        val = gpiod_line_get_value(p.line);

    p.stateLabel->setText(val ? "HIGH" : "LOW");
    p.toggleButton->setChecked(val);
    p.toggleButton->setText(val ? tr("Désactiver") : tr("Activer"));
}


