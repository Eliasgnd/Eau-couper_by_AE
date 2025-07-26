#include "WifiTransferWidget.h"
#include "MainWindow.h"
#include "qrcodegen.hpp"

#include <QVBoxLayout>
#include <QNetworkInterface>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QMessageBox>
#include <QCloseEvent>
#include <QApplication>
#include <QPainter>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include <QLabel>

WifiTransferWidget::WifiTransferWidget(QWidget *parent)
    : QWidget(parent), m_tcpServer(new QTcpServer(this))
{
    setWindowTitle(tr("Transfert Wi-Fi"));
    QVBoxLayout *layout = new QVBoxLayout(this);
    m_qrLabel = new QLabel(this);
    m_qrLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_backButton = new QPushButton(tr("Retour"), this);
    layout->addWidget(m_qrLabel);
    layout->addWidget(m_statusLabel);
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
        m_statusLabel->setText(tr("En attente de connexion..."));
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
    for (const QHostAddress &addr : QNetworkInterface::allAddresses()) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol && !addr.isLoopback()) {
            ip = addr.toString();
            break;
        }
    }
    if (ip.isEmpty())
        ip = QStringLiteral("127.0.0.1");

    const quint16 port = 8080;
    if (!m_tcpServer->listen(QHostAddress::Any, port)) {
        qDebug() << "[ERROR] Impossible d’écouter le port" << port;
        return {};
    }

    connect(m_tcpServer, &QTcpServer::newConnection, this, [this]() {
        while (m_tcpServer->hasPendingConnections()) {
            QTcpSocket *client = m_tcpServer->nextPendingConnection();
            handleClient(client);
        }
    });

    return QString("http://%1:%2/upload").arg(ip).arg(port);
}

void WifiTransferWidget::stopServer() {
    if (m_tcpServer)
        m_tcpServer->close();
}

void WifiTransferWidget::handleClient(QTcpSocket *client) {
    struct ClientContext {
        QByteArray buffer;
        QFile file;
        bool headerParsed = false;
        qint64 contentLength = -1;
        qint64 received = 0;
    };
    ClientContext *context = new ClientContext;

    connect(client, &QTcpSocket::readyRead, this, [=]() {
        QByteArray chunk = client->readAll();
        context->buffer += chunk;

        if (!context->headerParsed) {
            int headerEnd = context->buffer.indexOf("\r\n\r\n");
            if (headerEnd < 0) return;

            QByteArray header = context->buffer.left(headerEnd);
            QByteArray body = context->buffer.mid(headerEnd + 4);

            if (header.startsWith("GET /upload")) {
                QString filePath = QDir::currentPath() + "/html/upload.html";
                QFile file(filePath);

                QByteArray resp;
                if (file.open(QIODevice::ReadOnly))
                    resp = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " +
                           QByteArray::number(file.size()) + "\r\n\r\n" + file.readAll();
                else
                    resp = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nupload.html introuvable";

                client->write(resp);
                client->flush();
                client->disconnectFromHost();
                return;
            }

            if (header.startsWith("POST /upload")) {
                // extraire longueur
                QRegularExpression re("Content-Length: (\\d+)", QRegularExpression::CaseInsensitiveOption);
                auto match = re.match(QString::fromUtf8(header));
                if (match.hasMatch()) context->contentLength = match.captured(1).toLongLong();

                QString dir = qApp->applicationDirPath() + "/images_generees";
                QDir().mkpath(dir);
                QString path = dir + "/" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".jpg";
                context->file.setFileName(path);
                if (!context->file.open(QIODevice::WriteOnly)) {
                    client->write("HTTP/1.1 500\r\n\r\nErreur écriture fichier");
                    client->flush();
                    client->disconnectFromHost();
                    return;
                }

                context->file.write(body);
                context->received = body.size();
                context->headerParsed = true;
                context->buffer.clear();

            } else {
                client->write("HTTP/1.1 400\r\n\r\nBad Request");
                client->flush();
                client->disconnectFromHost();
                return;
            }
        } else {
            context->file.write(context->buffer);
            context->received += context->buffer.size();
            context->buffer.clear();
        }

        if (context->headerParsed && context->received >= context->contentLength) {
            context->file.close();
            qDebug() << "[OK] Image reçue :" << context->file.fileName();

            client->write("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nImage enregistrée !");
            client->flush();
            client->disconnectFromHost();
        }
    });

    connect(client, &QTcpSocket::disconnected, this, [=]() {
        client->deleteLater();
        delete context;
    });
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
