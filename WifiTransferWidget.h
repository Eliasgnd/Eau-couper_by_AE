#ifndef WIFITRANSFERWIDGET_H
#define WIFITRANSFERWIDGET_H

#include <QWidget>
#include <QHttpServer>
#include <QLabel>
#include <QPushButton>

class WifiTransferWidget : public QWidget {
    Q_OBJECT
public:
    explicit WifiTransferWidget(QWidget *parent = nullptr);
    ~WifiTransferWidget();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QString startServer();
    void stopServer();
    QImage generateQr(const QString &url);

    QHttpServer m_server;
    QLabel *m_qrLabel = nullptr;
    QPushButton *m_backButton = nullptr;
};

#endif // WIFITRANSFERWIDGET_H
