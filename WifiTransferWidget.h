// WifiTransferWidget.h
#ifndef WIFITRANSFERWIDGET_H
#define WIFITRANSFERWIDGET_H

#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QListWidget>
#include <QSystemTrayIcon>

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
    QProgressBar *m_progressBar = nullptr;
    QLabel *m_imageLabel = nullptr;
    QListWidget *m_historyList = nullptr;
    QSystemTrayIcon *m_trayIcon = nullptr;

};

#endif // WIFITRANSFERWIDGET_H
