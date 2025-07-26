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

    QString sanitizeFileName(const QString &name) const;
    bool   isAllowedExt(const QString &ext) const;

    static constexpr qint64 MAX_UPLOAD_BYTES = 20 * 1024 * 1024; // 20 Mo
};

#endif // WIFITRANSFERWIDGET_H
