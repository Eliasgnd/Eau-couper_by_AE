#ifndef BLUETOOTHRECEIVERDIALOG_H
#define BLUETOOTHRECEIVERDIALOG_H

#include <QWidget>
#include <QProcess>
#include <QStringList>
#include <QTimer>

namespace Ui {
class BluetoothReceiverDialog;
}

class BluetoothReceiverDialog : public QWidget
{
    Q_OBJECT

public:
    explicit BluetoothReceiverDialog(QWidget *parent = nullptr);
    ~BluetoothReceiverDialog() override;

private slots:
    void openFolder();
    void refreshFileList();
    void onProcessOutput();
    void onProcessError(QProcess::ProcessError);

private:
    void startBluetoothService();
    void showError(const QString &message);
    void closeEvent(QCloseEvent *event);

    Ui::BluetoothReceiverDialog *ui;
    QProcess *m_btProcess = nullptr;
    QString m_targetDir;
    QTimer *m_statusTimer = nullptr;
    QStringList m_prevFiles;
    QTimer *m_flashTimer = nullptr;

};

#endif // BLUETOOTHRECEIVERDIALOG_H
