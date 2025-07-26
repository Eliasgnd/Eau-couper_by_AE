#ifndef WIFITRANSFERWIDGET_H
#define WIFITRANSFERWIDGET_H

#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSystemTrayIcon>
#include <QImage>

QT_BEGIN_NAMESPACE
namespace Ui { class WifiTransferWidget; }
QT_END_NAMESPACE

class WifiTransferWidget : public QWidget {
    Q_OBJECT
public:
    explicit WifiTransferWidget(QWidget *parent = nullptr);
    ~WifiTransferWidget() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QString startServer();
    void    stopServer();
    QImage  generateQr(const QString &url);
    void    handleClient(QTcpSocket *client);

    QString sanitizeFileName(const QString &name) const;
    bool    isAllowedExt(const QString &ext) const;

    // UI
    Ui::WifiTransferWidget *ui {nullptr};
    QTcpServer      *m_tcpServer     = nullptr;
    QSystemTrayIcon *m_trayIcon      = nullptr;

    static constexpr qint64 MAX_UPLOAD_BYTES = 20 * 1024 * 1024; // 20 Mo
};

#endif // WIFITRANSFERWIDGET_H
