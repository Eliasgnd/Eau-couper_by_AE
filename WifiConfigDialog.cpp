#include "WifiConfigDialog.h"
#include "ui_WifiConfigDialog.h"
#include "MainWindow.h"

#include <QProcess>
#include <QMessageBox>

WifiConfigDialog::WifiConfigDialog(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::WifiConfigDialog)
{
    ui->setupUi(this);

    connect(ui->refreshButton, &QPushButton::clicked,
            this, &WifiConfigDialog::scanNetworks);
    connect(ui->connectButton, &QPushButton::clicked,
            this, &WifiConfigDialog::connectSelected);
    connect(ui->backButton, &QPushButton::clicked, this, [this]{
        close();
        if (auto mw = MainWindow::getInstance())
            mw->showFullScreen();
    });

    scanNetworks();
}

WifiConfigDialog::~WifiConfigDialog()
{
    delete ui;
}

void WifiConfigDialog::scanNetworks()
{
    ui->networkComboBox->clear();

    QProcess proc;
    proc.start("bash", QStringList() << "-c" << "nmcli -t -f SSID dev wifi list | sort -u");
    proc.waitForFinished();
    QString output = proc.readAllStandardOutput();
    QStringList ssids = output.split('\n', Qt::SkipEmptyParts);
    for (const QString &s : ssids) {
        QString trimmed = s.trimmed();
        if (!trimmed.isEmpty())
            ui->networkComboBox->addItem(trimmed);
    }
}

void WifiConfigDialog::connectSelected()
{
    const QString ssid = ui->networkComboBox->currentText();
    const QString password = ui->passwordEdit->text();
    if (ssid.isEmpty())
        return;

    QString cmd = QString("nmcli dev wifi connect '%1'").arg(ssid.replace("'", "\\'"));
    if (!password.isEmpty())
        cmd += QString(" password '%1'").arg(password.replace("'", "\\'"));

    QProcess proc;
    proc.start("bash", QStringList() << "-c" << cmd);
    proc.waitForFinished();
    QString out = proc.readAllStandardOutput() + proc.readAllStandardError();
    if (proc.exitStatus() == QProcess::NormalExit && proc.exitCode() == 0) {
        ui->statusLabel->setText(tr("✅ Connect\u00e9 à %1").arg(ssid));
    } else {
        QMessageBox::warning(this, tr("Connexion Wi-Fi"), tr("\u274C %1").arg(out.trimmed()));
        ui->statusLabel->setText(tr("\u274C \u00c9chec de la connexion"));
    }
}

void WifiConfigDialog::closeEvent(QCloseEvent *event)
{
    QWidget::closeEvent(event);
    if (auto mw = MainWindow::getInstance())
        mw->showFullScreen();
}

