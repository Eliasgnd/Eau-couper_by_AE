#include "WifiTransferWidget.h"
#include "MainWindow.h"
#include "qrcodegen.hpp"

#include <QVBoxLayout>
#include <QNetworkInterface>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QRegularExpression>
#include <QMessageBox>
#include <QCloseEvent>
#include <QApplication>
#include <QPainter>

WifiTransferWidget::WifiTransferWidget(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(tr("Transfert Wi-Fi"));
    QVBoxLayout *layout = new QVBoxLayout(this);
    m_qrLabel = new QLabel(this);
    m_qrLabel->setAlignment(Qt::AlignCenter);
    m_backButton = new QPushButton(tr("Retour"), this);
    layout->addWidget(m_qrLabel);
    layout->addWidget(m_backButton);

    connect(m_backButton, &QPushButton::clicked, this, [this]() {
        close();
        if (auto mw = MainWindow::getInstance())
            mw->showFullScreen();
    });

    QString url = startServer();
    if (!url.isEmpty()) {
        QImage qr = generateQr(url);
        m_qrLabel->setPixmap(QPixmap::fromImage(qr));
    } else {
        m_qrLabel->setText(tr("Impossible de démarrer le serveur"));
    }
}

WifiTransferWidget::~WifiTransferWidget() {
    stopServer();
}

void WifiTransferWidget::closeEvent(QCloseEvent *event) {
    stopServer();
    QWidget::closeEvent(event);
}

QString WifiTransferWidget::startServer() {
    QString ip;
    const auto addrs = QNetworkInterface::allAddresses();
    for (const QHostAddress &addr : addrs) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol && !addr.isLoopback()) {
            ip = addr.toString();
            break;
        }
    }
    if (ip.isEmpty())
        ip = QStringLiteral("127.0.0.1");

    const quint16 port = 8080;

    m_server.route("/upload", QHttpServerRequest::Method::Get,
                   []() {
                       QFile file("html/upload.html");
                       if (file.open(QIODevice::ReadOnly))
                           return QHttpServerResponse("text/html", file.readAll());
                       return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
                   });

    m_server.route("/upload", QHttpServerRequest::Method::Post,
                   [](const QHttpServerRequest &req) {
                       if (!req.hasBody())
                           return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);
                       auto multi = req.multipartFormData();
                       if (!multi)
                           return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);

                       QString dir = qApp->applicationDirPath() + QDir::separator() + "images_generees";
                       QDir().mkpath(dir);

                       for (const auto &part : multi->parts()) {
                           QString fname = QFileInfo(part.filename()).fileName();
                           fname.replace(QRegularExpression("[^A-Za-z0-9._-]"), "_");
                           if (!fname.endsWith(".png", Qt::CaseInsensitive) &&
                               !fname.endsWith(".jpg", Qt::CaseInsensitive) &&
                               !fname.endsWith(".jpeg", Qt::CaseInsensitive))
                               continue;
                           QString path = dir + QDir::separator() + fname;
                           if (QFile::exists(path)) {
                               QString ts = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
                               path = dir + QDir::separator() + ts + '_' + fname;
                           }
                           QFile f(path);
                           if (f.open(QIODevice::WriteOnly)) {
                               f.write(part.body());
                               f.close();
                           }
                       }
                       return QHttpServerResponse("text/plain", "OK");
                   });

    if (!m_server.listen(QHostAddress::Any, port))
        return QString();

    return QStringLiteral("http://%1:%2/upload").arg(ip).arg(port);
}

void WifiTransferWidget::stopServer() {
    if (m_server.isListening())
        m_server.close();
}

QImage WifiTransferWidget::generateQr(const QString &url) {
    using qrcodegen::QrCode;
    const auto qr = QrCode::encodeText(url.toUtf8().constData(), QrCode::Ecc::LOW);
    const int size = qr.getSize();
    const int scale = 5;
    QImage img(size * scale, size * scale, QImage::Format_RGB32);
    img.fill(Qt::white);
    QPainter p(&img);
    p.setBrush(Qt::black);
    p.setPen(Qt::NoPen);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            if (qr.getModule(x, y))
                p.drawRect(x * scale, y * scale, scale, scale);
        }
    }
    return img;
}


