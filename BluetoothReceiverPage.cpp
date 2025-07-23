#include "BluetoothReceiverPage.h"
#include "ui_BluetoothReceiverPage.h"
#include "MainWindow.h"
#include "ScreenUtils.h"

#include <QPlainTextEdit>
#include <QLabel>

BluetoothReceiverPage::BluetoothReceiverPage(QWidget *parent)
    : QWidget(parent), ui(new Ui::BluetoothReceiverPage)
{
    ui->setupUi(this);
    ScreenUtils::placeOnSecondaryScreen(this);

    ui->logTextEdit->setReadOnly(true);
    ui->logTextEdit->setStyleSheet("background:white; color:black; font-size:12px;");

    updateStatus(tr("En attente de fichiers..."), "green");

    startReceiver();
}

BluetoothReceiverPage::~BluetoothReceiverPage()
{
    if(m_process){
        if(m_process->state() != QProcess::NotRunning){
            m_process->terminate();
            m_process->waitForFinished(2000);
        }
        delete m_process;
    }
    delete ui;
}

void BluetoothReceiverPage::startReceiver()
{
    QProcess::execute("sudo", QStringList() << "pkill" << "obexd");

    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &BluetoothReceiverPage::readStdOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &BluetoothReceiverPage::readStdError);
    connect(m_process, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
            this, &BluetoothReceiverPage::processFinished);

    m_process->start("/home/AE/start_bt_receiver.sh");
    if(!m_process->waitForStarted(3000)) {
        appendLog(tr("Erreur: impossible de démarrer le script"));
        updateStatus(tr("Erreur lors du démarrage"), "orange");
    }
}

void BluetoothReceiverPage::appendLog(const QString &text)
{
    ui->logTextEdit->appendPlainText(text.trimmed());
}

void BluetoothReceiverPage::updateStatus(const QString &text, const QString &color)
{
    ui->statusLabel->setText(text);
    ui->statusLabel->setStyleSheet(QString("color:%1; font-size:14px;").arg(color));
}

void BluetoothReceiverPage::parseOutput(const QString &text)
{
    QString lower = text.toLower();
    if(lower.contains("error") || lower.contains("erreur") || lower.contains("fail")) {
        updateStatus(tr("Erreur..."), "orange");
    } else if(lower.contains("waiting") || lower.contains("en attente") || lower.contains("listening")) {
        updateStatus(tr("En attente de fichiers..."), "green");
    } else if(lower.contains("receive") || lower.contains("saving") || lower.contains("progress")) {
        updateStatus(tr("Transfert en cours"), "blue");
    } else if(lower.contains("complete") || lower.contains("fichier")) {
        updateStatus(tr("Fichier reçu"), "green");
    }
}

void BluetoothReceiverPage::readStdOutput()
{
    QString data = QString::fromLocal8Bit(m_process->readAllStandardOutput());
    appendLog(data);
    parseOutput(data);
}

void BluetoothReceiverPage::readStdError()
{
    QString data = QString::fromLocal8Bit(m_process->readAllStandardError());
    appendLog(data);
    parseOutput(data);
}

void BluetoothReceiverPage::processFinished(int, QProcess::ExitStatus)
{
    appendLog(tr("Processus terminé"));
}

void BluetoothReceiverPage::on_backButton_clicked()
{
    this->close();
    MainWindow::getInstance()->showFullScreen();
}

