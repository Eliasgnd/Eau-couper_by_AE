#pragma once
#include <QWidget>

#ifdef DISABLE_GPIO
// ==== Stub TestGpio (no GPIO) ====
class TestGpio : public QWidget {
    Q_OBJECT
public:
    explicit TestGpio(QWidget *parent = nullptr) : QWidget(parent) {}
    ~TestGpio() override {}

protected:
    void closeEvent(QCloseEvent *event) override { QWidget::closeEvent(event); }
};
#else
// ==== Version Linux ====
#include <QMap>
#include <QTimer>
#include <QCheckBox>
#include <QLabel>
#include <gpiod.h>

namespace Ui { class TestGpio; }

class TestGpio : public QWidget {
    Q_OBJECT
public:
    explicit TestGpio(QWidget *parent = nullptr);
    ~TestGpio() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::TestGpio *ui {nullptr};
    gpiod_chip *chip {nullptr};
    QMap<int, gpiod_line*> outputLines;
    QMap<int, gpiod_line*> inputLines;
};
#endif
