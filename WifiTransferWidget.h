// WifiTransferWidget.h
#ifndef WIFITRANSFERWIDGET_H
#define WIFITRANSFERWIDGET_H

#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>
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
    void handleClient(QTcpSocket *client);

    QTcpServer *m_tcpServer = nullptr;
    QLabel *m_qrLabel = nullptr;
    QPushButton *m_backButton = nullptr;
    QLabel *m_statusLabel = nullptr;

};

#endif // WIFITRANSFERWIDGET_H
