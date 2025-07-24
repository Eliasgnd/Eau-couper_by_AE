#include "BluetoothReceiverDialog.h"
#include "ui_BluetoothReceiverDialog.h"
#include "ScreenUtils.h"
#include "MainWindow.h"

#include <QDir>
#include <QFileInfoList>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QIcon>
#include <QTimer>
#include <QDebug>

BluetoothReceiverDialog::BluetoothReceiverDialog(QWidget *parent)
    : QWidget(parent), ui(new Ui::BluetoothReceiverDialog)
{
    ui->setupUi(this);
    ScreenUtils::placeOnSecondaryScreen(this);

    ui->logTextEdit->setVisible(false);
    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(0);
    ui->progressBar->setValue(-1);
    ui->statusIcon->setStyleSheet("border-radius:6px; background: orange;");
    ui->statusLabel->setText(QStringLiteral("🔄 Initialisation Bluetooth..."));

    m_targetDir = QDir::homePath() + "/BluetoothReceived";
    QDir().mkpath(m_targetDir);

    connect(ui->openFolderButton, &QPushButton::clicked, this, &BluetoothReceiverDialog::openFolder);
    connect(ui->backButton, &QPushButton::clicked, this, [this] {
        close();
        MainWindow::getInstance()->showFullScreen();
    });

    startBluetoothService();

    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &BluetoothReceiverDialog::refreshFileList);
    timer->start(2000);
    refreshFileList();
    // Timer de surveillance du service bluetoothd
    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, [this]() {
        QProcess checkProcess;
        checkProcess.start("bash", QStringList() << "-c" << "systemctl is-active bluetooth");
        checkProcess.waitForFinished();
        QString output = checkProcess.readAllStandardOutput().trimmed();

        if (output != "active") {
            m_statusTimer->stop();  // Arrête les vérifications
            if (m_btProcess && m_btProcess->state() != QProcess::NotRunning)
                m_btProcess->kill();  // Stoppe proprement

            QMessageBox::critical(this,
                tr("Erreur Bluetooth"),
                tr("❌ Le service Bluetooth est inactif.\n\nVeuillez redémarrer la machine."),
                QMessageBox::Ok
            );

            this->close();  // Ferme le dialogue actuel
            MainWindow::getInstance()->showFullScreen();  // Retourne à l'accueil
        }
    });
    m_statusTimer->start(5000); // Vérifie toutes les 5 secondes

}

BluetoothReceiverDialog::~BluetoothReceiverDialog()
{
    if (m_btProcess && m_btProcess->state() != QProcess::NotRunning)
        m_btProcess->kill();
    delete ui;
    if (m_statusTimer)
        m_statusTimer->stop();
    if (m_flashTimer)
        m_flashTimer->stop();

}

void BluetoothReceiverDialog::startBluetoothService()
{
    m_btProcess = new QProcess(this);

    connect(m_btProcess, &QProcess::readyReadStandardOutput, this, &BluetoothReceiverDialog::onProcessOutput);
    connect(m_btProcess, &QProcess::readyReadStandardError, this, &BluetoothReceiverDialog::onProcessOutput);
    connect(m_btProcess, &QProcess::errorOccurred, this, &BluetoothReceiverDialog::onProcessError);

    const QString script = R"(

    echo "[INFO] Déverrouillage de l'interface Bluetooth..."
    sudo /usr/sbin/rfkill unblock bluetooth

    echo "[INFO] Lancement du service bluetoothd si nécessaire..."
    sudo /usr/bin/systemctl start bluetooth
    sleep 1

    echo "[INFO] Démarrage de l'agent simplifié..."
    /home/AE/bluetooth_agent.py &

    echo "[INFO] Configuration du contrôleur Bluetooth..."
    echo "[INFO] Configuration du contrôleur Bluetooth..."
    (
    echo "power on"
    sleep 1
    echo "agent NoInputNoOutput"
    sleep 1
    echo "default-agent"
    sleep 1
    echo "discoverable on"
    echo "pairable on"
    ) | bluetoothctl

    echo "[INFO] Préparation du dossier de réception..."
    mkdir -p /home/AE/BluetoothReceived
    chmod 777 /home/AE/BluetoothReceived

    echo "[INFO] Fermeture des anciens obexd..."
    sudo /usr/bin/pkill obexd 2>/dev/null

    echo "[INFO] Lancement de obexd pour recevoir les fichiers..."
    /usr/libexec/bluetooth/obexd --root=/home/AE/BluetoothReceived -n -l -a

    )";


    m_btProcess->start("bash", QStringList() << "-c" << script);

    if (!m_btProcess->waitForStarted(3000)) {
        showError(tr("Impossible de démarrer le service Bluetooth."));
    }
}

void BluetoothReceiverDialog::onProcessOutput()
{
    const QByteArray out = m_btProcess->readAllStandardOutput() + m_btProcess->readAllStandardError();
    if (!out.isEmpty()) {
        const QString text = QString::fromLocal8Bit(out).trimmed();
        ui->logTextEdit->appendPlainText(text);

        if (text.contains("discoverable", Qt::CaseInsensitive) &&
            (text.contains("succeeded", Qt::CaseInsensitive) || text.contains("yes", Qt::CaseInsensitive))) {
            ui->progressBar->setRange(0, 100);
            ui->progressBar->setValue(100);
            ui->statusIcon->setStyleSheet("border-radius:6px; background: green;");
            ui->statusLabel->setText(QStringLiteral("✅ Bluetooth prêt à recevoir"));
        }
    }
}

void BluetoothReceiverDialog::onProcessError(QProcess::ProcessError)
{
    showError(tr("Erreur du service Bluetooth. Vérifiez les droits sudo."));
}

void BluetoothReceiverDialog::openFolder()
{
    QProcess::startDetached("xdg-open", {m_targetDir});
}

void BluetoothReceiverDialog::refreshFileList()
{
    ui->fileListWidget->clear();
    QDir dir(m_targetDir);

    // Liste des extensions autorisées (en minuscules, sans le point)
    QStringList allowedExtensions = {"png", "jpg", "jpeg", "bmp", "webp", "svg"};

    // Liste tous les fichiers (par date décroissante)
    QFileInfoList files = dir.entryInfoList(QDir::Files, QDir::Time);

    QStringList currentFiles;
    for (const QFileInfo &info : files) {
        QString ext = info.suffix().toLower();

        // Supprime les fichiers avec extension non autorisée
        if (!allowedExtensions.contains(ext)) {
            QFile::remove(info.absoluteFilePath());
            continue;
        }

        currentFiles << info.fileName();

        // Ajoute à l'interface uniquement les fichiers valides
        auto *item = new QListWidgetItem(QIcon(":/icons/file.svg"),
                                         info.fileName(),
                                         ui->fileListWidget);
        item->setToolTip(info.absoluteFilePath());
    }

    // Détection des nouveaux fichiers reçus
    QStringList newFiles;
    for (const QString &f : currentFiles) {
        if (!m_prevFiles.contains(f))
            newFiles << f;
    }

    if (!newFiles.isEmpty()) {
        const QString &name = newFiles.first();
        ui->statusLabel->setText(tr("📥 Fichier reçu : %1").arg(name));
        ui->statusIcon->setStyleSheet("border-radius:6px; background: blue;");

        if (!m_flashTimer) {
            m_flashTimer = new QTimer(this);
            m_flashTimer->setSingleShot(true);
            connect(m_flashTimer, &QTimer::timeout, this, [this]() {
                ui->statusLabel->setText(QStringLiteral("✅ Bluetooth prêt à recevoir"));
                ui->statusIcon->setStyleSheet("border-radius:6px; background: green;");
            });
        }
        m_flashTimer->start(3000);
    }

    m_prevFiles = currentFiles;
}

void BluetoothReceiverDialog::showError(const QString &msg)
{
    QMessageBox::warning(this, tr("Bluetooth"), msg);
}

void BluetoothReceiverDialog::closeEvent(QCloseEvent *event)
{
    // Arrête les services proprement
    QProcess::startDetached("bash", QStringList() << "-c" << R"(
        echo 'discoverable off' | bluetoothctl
        echo 'pairable off' | bluetoothctl
        echo 'power off' | bluetoothctl
        pkill obexd 2>/dev/null
    )");

    QWidget::closeEvent(event); // appel au parent
}

