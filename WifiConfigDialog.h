#ifndef WIFICONFIGDIALOG_H
#define WIFICONFIGDIALOG_H

#include <QWidget>
#include <QProcess>

QT_BEGIN_NAMESPACE
namespace Ui { class WifiConfigDialog; }
QT_END_NAMESPACE

class WifiConfigDialog : public QWidget
{
    Q_OBJECT
public:
    explicit WifiConfigDialog(QWidget *parent = nullptr);
    ~WifiConfigDialog() override;

private slots:
    void scanNetworks();
    void connectSelected();

private:
    void closeEvent(QCloseEvent *event) override;

    Ui::WifiConfigDialog *ui {nullptr};
};

#endif // WIFICONFIGDIALOG_H
