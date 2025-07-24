#ifndef BLUETOOTHRECEIVERDIALOG_H
#define BLUETOOTHRECEIVERDIALOG_H

#include <QWidget>
#include <QProcess>

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

private:
    void startBlueman();
    void showError(const QString &message);

    Ui::BluetoothReceiverDialog *ui {nullptr};
    QProcess *m_bluemanProcess {nullptr};
    QString m_targetDir;
};

#endif // BLUETOOTHRECEIVERDIALOG_H
