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
#include <QImageReader>
#include <QRegularExpression>

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

void WifiTransferWidget::handleClient(QTcpSocket *client)
{
    struct ClientContext {
        QByteArray buffer;
        QFile      file;
        bool       headerParsed = false;
        qint64     contentLength = -1;
        qint64     received      = 0;
    };
    auto ctx = new ClientContext;

    // Limite de taille (20 Mo)
    static const qint64 MAX_UPLOAD = 20 * 1024 * 1024;

    // Helpers locaux
    auto sanitize = [](QString name){
        name = QFileInfo(name).fileName();
        name.replace(QRegularExpression("[^A-Za-z0-9._-]"), "_");
        if (name.isEmpty()) name = "fichier";
        return name;
    };
    auto isAllowedExt = [](const QString &ext){
        static const QStringList ok = {"jpg","jpeg","png","gif","bmp","webp"};
        return ok.contains(ext.toLower());
    };

    connect(client, &QTcpSocket::readyRead, this, [=]() {
        ctx->buffer += client->readAll();

        if (!ctx->headerParsed) {
            const int headerEnd = ctx->buffer.indexOf("\r\n\r\n");
            if (headerEnd < 0) return; // attendre l'entête complet

            QByteArray headerBA = ctx->buffer.left(headerEnd);
            QByteArray body     = ctx->buffer.mid(headerEnd + 4);
            QString    header   = QString::fromUtf8(headerBA);

            // -------- GET /upload --------
            if (header.startsWith("GET /upload")) {
                QFile f(":/html/upload.html");
                QByteArray resp;
                if (f.open(QIODevice::ReadOnly)) {
                    QByteArray html = f.readAll();
                    resp = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: " +
                           QByteArray::number(html.size()) + "\r\n\r\n" + html;
                } else {
                    resp = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nupload.html introuvable";
                }
                client->write(resp);
                client->flush();
                client->disconnectFromHost();
                return;
            }

            // -------- POST /upload --------
            if (header.startsWith("POST /upload")) {
                // Content-Length
                QRegularExpression reLen("Content-Length:\\s*(\\d+)", QRegularExpression::CaseInsensitiveOption);
                auto mLen = reLen.match(header);
                if (mLen.hasMatch())
                    ctx->contentLength = mLen.captured(1).toLongLong();

                if (ctx->contentLength <= 0 || ctx->contentLength > MAX_UPLOAD) {
                    client->write("HTTP/1.1 413 Payload Too Large\r\nContent-Type: text/plain\r\n\r\nFichier trop volumineux (max 20 Mo)");
                    client->flush();
                    client->disconnectFromHost();
                    return;
                }

                // Nom original envoyé par le header X-Filename
                QRegularExpression reName("X-Filename:\\s*(.+)", QRegularExpression::CaseInsensitiveOption);
                auto mName = reName.match(header);
                QString origName;
                if (mName.hasMatch())
                    origName = QUrl::fromPercentEncoding(mName.captured(1).trimmed().toUtf8());

                QString safeName = sanitize(origName);
                QString ext = QFileInfo(safeName).suffix();
                if (!isAllowedExt(ext)) {
                    client->write("HTTP/1.1 415 Unsupported Media Type\r\nContent-Type: text/plain\r\n\r\nExtension non autorisée");
                    client->flush();
                    client->disconnectFromHost();
                    return;
                }

                // Prépare le fichier
                QString dir  = qApp->applicationDirPath() + "/images_generees";
                QDir().mkpath(dir);
                QString path = dir + '/'
                               + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_")
                               + safeName;

                ctx->file.setFileName(path);
                if (!ctx->file.open(QIODevice::WriteOnly)) {
                    client->write("HTTP/1.1 500 Internal Server Error\r\n\r\nErreur écriture fichier");
                    client->flush();
                    client->disconnectFromHost();
                    return;
                }

                // Écrit ce qu'on a déjà reçu
                ctx->file.write(body);
                ctx->received = body.size();
                if (m_statusLabel)
                    m_statusLabel->setText(tr("Réception : %1 / %2 octets").arg(ctx->received).arg(ctx->contentLength));

                ctx->headerParsed = true;
                ctx->buffer.clear();
            }
            else {
                client->write("HTTP/1.1 400 Bad Request\r\n\r\n");
                client->flush();
                client->disconnectFromHost();
                return;
            }
        }
        else {
            // Continuer à écrire les chunks
            if (!ctx->buffer.isEmpty()) {
                if (ctx->received + ctx->buffer.size() > MAX_UPLOAD) {
                    ctx->file.close();
                    client->write("HTTP/1.1 413 Payload Too Large\r\n\r\n");
                    client->flush();
                    client->disconnectFromHost();
                    return;
                }
                ctx->file.write(ctx->buffer);
                ctx->received += ctx->buffer.size();
                ctx->buffer.clear();

                if (m_statusLabel)
                    m_statusLabel->setText(tr("Réception : %1 / %2 octets").arg(ctx->received).arg(ctx->contentLength));
            }
        }

        // Fin du transfert
        if (ctx->headerParsed && ctx->received >= ctx->contentLength) {
            ctx->file.close();

            // Vérif que c'est bien une image
            QImageReader reader(ctx->file.fileName());
            if (!reader.canRead()) {
                QFile::remove(ctx->file.fileName());
                client->write("HTTP/1.1 415 Unsupported Media Type\r\nContent-Type: text/plain\r\n\r\nFichier non reconnu comme image");
                client->flush();
                client->disconnectFromHost();
                return;
            }

            if (m_statusLabel) {
                m_statusLabel->setStyleSheet("color:#2e7d32;");
                m_statusLabel->setText(tr("Réception terminée ✅"));
            }

            client->write("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nImage enregistrée !");
            client->flush();
            client->disconnectFromHost();
        }
    });

    connect(client, &QTcpSocket::disconnected, this, [=]() {
        client->deleteLater();
        delete ctx;
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

QString WifiTransferWidget::sanitizeFileName(const QString &name) const {
    QString base = QFileInfo(name).fileName();               // enlève chemin
    base.replace(QRegularExpression("[^A-Za-z0-9._-]"), "_"); // garde safe chars
    if (base.isEmpty()) base = "fichier";
    return base;
}

bool WifiTransferWidget::isAllowedExt(const QString &ext) const {
    static const QStringList ok = {"jpg","jpeg","png","gif","bmp","webp"};
    return ok.contains(ext.toLower());
}
