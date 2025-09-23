#include "TestGpio.h"

#ifndef DISABLE_GPIO   // Compile uniquement si GPIO activé

#include "ui_TestGpio.h"
#include <QCloseEvent>

TestGpio::TestGpio(QWidget *parent)
    : QWidget(parent), ui(new Ui::TestGpio) {
    ui->setupUi(this);
}

TestGpio::~TestGpio() {
    delete ui;
}

void TestGpio::closeEvent(QCloseEvent *event) {
    QWidget::closeEvent(event);
}

#endif // DISABLE_GPIO
