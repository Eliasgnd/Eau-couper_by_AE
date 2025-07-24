#include "BluetoothReceiverDialog.h"
#include "ui_BluetoothReceiverDialog.h"
#include "ScreenUtils.h"
#include "MainWindow.h"
#include <QDir>
#include <QFileInfoList>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QProcess>
#include <QIcon>

BluetoothReceiverDialog::BluetoothReceiverDialog(QWidget *parent)
    : QWidget(parent), ui(new Ui::BluetoothReceiverDialog)
{
    ui->setupUi(this);
    ScreenUtils::placeOnSecondaryScreen(this);

    m_targetDir = QStringLiteral("/home/pi/BluetoothReceived");
    QDir().mkpath(m_targetDir);

    connect(ui->openFolderButton, &QPushButton::clicked,
            this, &BluetoothReceiverDialog::openFolder);
    connect(ui->backButton, &QPushButton::clicked, this, [this]() {
        this->close();
        MainWindow::getInstance()->showFullScreen();
    });

    startBlueman();
    refreshFileList();
}

BluetoothReceiverDialog::~BluetoothReceiverDialog()
{
    if (m_bluemanProcess) {
        if (m_bluemanProcess->state() != QProcess::NotRunning)
            m_bluemanProcess->terminate();
        m_bluemanProcess->deleteLater();
    }
    delete ui;
}

void BluetoothReceiverDialog::startBlueman()
{
    m_bluemanProcess = new QProcess(this);
    m_bluemanProcess->start(QStringLiteral("blueman-manager"));
    if (!m_bluemanProcess->waitForStarted(3000)) {
        showError(tr("Impossible de lancer blueman-manager"));
    }
}

void BluetoothReceiverDialog::showError(const QString &message)
{
    QMessageBox::warning(this, tr("Erreur"), message);
}

void BluetoothReceiverDialog::openFolder()
{
    QProcess::startDetached(QStringLiteral("xdg-open"), QStringList() << m_targetDir);
}

void BluetoothReceiverDialog::refreshFileList()
{
    ui->fileListWidget->clear();

    QDir dir(m_targetDir);
    QFileInfoList files = dir.entryInfoList(QDir::Files, QDir::Time);
    for (const QFileInfo &info : files) {
        QListWidgetItem *item = new QListWidgetItem(QIcon(":/icons/folderfull.svg"),
                                                   info.fileName(), ui->fileListWidget);
        item->setToolTip(info.absoluteFilePath());
    }
}
