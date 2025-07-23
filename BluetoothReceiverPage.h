#ifndef BLUETOOTHRECEIVERPAGE_H
#define BLUETOOTHRECEIVERPAGE_H

#include <QWidget>
#include <QProcess>

namespace Ui {
class BluetoothReceiverPage;
}

class BluetoothReceiverPage : public QWidget
{
    Q_OBJECT
public:
    explicit BluetoothReceiverPage(QWidget *parent = nullptr);
    ~BluetoothReceiverPage() override;

private slots:
    void readStdOutput();
    void readStdError();
    void processFinished(int exitCode, QProcess::ExitStatus status);
    void on_backButton_clicked();

private:
    void startReceiver();
    void appendLog(const QString &text);
    void updateStatus(const QString &text, const QString &color);
    void parseOutput(const QString &text);

    Ui::BluetoothReceiverPage *ui {nullptr};
    QProcess *m_process {nullptr};
};

#endif // BLUETOOTHRECEIVERPAGE_H
