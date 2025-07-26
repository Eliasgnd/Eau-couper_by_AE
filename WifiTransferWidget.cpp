#include "WifiTransferWidget.h"
#include "ui_WifiTransferWidget.h"
#include "MainWindow.h"
#include "qrcodegen.hpp"

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
#include <QProgressBar>
#include <QListWidget>
#include <QDesktopServices>
#include <QUrl>
#include <QImageReader>
#include <QRegularExpression>
#include <QFileInfo>
#include <QIcon>

// ---------------------- Helpers multipart ---------------------------
struct MultipartPart {
    QString     filename;
    QString     contentType;
    QByteArray  data;
};

static QList<MultipartPart> parseMultipartFormData(const QByteArray &body,
                                                   const QByteArray &boundaryRaw)
{
    QList<MultipartPart> parts;
    if (boundaryRaw.isEmpty())
        return parts;

    const QByteArray boundary = "--" + boundaryRaw;
    int pos = 0;
    while (true) {
        // Cherche prochain boundary
        int start = body.indexOf(boundary, pos);
        if (start < 0) break;
        start += boundary.size();
        // Fin ?
        if (body.mid(start, 2) == QByteArray("--"))
            break;

        // Skip CRLF après boundary
        if (body.mid(start, 2) == QByteArray("\r\n"))
            start += 2;

        // Headers de la part
        int headerEnd = body.indexOf("\r\n\r\n", start);
        if (headerEnd < 0) break;
        QByteArray partHeader = body.mid(start, headerEnd - start);
        int dataStart = headerEnd + 4;

        // Prochain boundary
        int next = body.indexOf(boundary, dataStart);
        if (next < 0) next = body.size();
        // Enlever les \r\n juste avant boundary
        int dataEnd = next;
        if (dataEnd >= 2 && body.mid(dataEnd - 2, 2) == "\r\n")
            dataEnd -= 2;

        QByteArray partData = body.mid(dataStart, dataEnd - dataStart);

        MultipartPart p;
        // Content-Disposition: form-data; name="files[]"; filename="xxx.jpg"
        QRegularExpression reFN("filename=\"([^\"]+)\"", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch m = reFN.match(QString::fromUtf8(partHeader));
        if (m.hasMatch())
            p.filename = m.captured(1);

        // Content-Type: image/jpeg
        QRegularExpression reCT("Content-Type:\\s*([^\\r\\n]+)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch m2 = reCT.match(QString::fromUtf8(partHeader));
        if (m2.hasMatch())
            p.contentType = m2.captured(1).trimmed();

        p.data = partData;
        if (!p.filename.isEmpty() && !p.data.isEmpty())
            parts.append(p);

        pos = next;
    }
    return parts;
}

// --------------------------------------------------------------------

WifiTransferWidget::WifiTransferWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::WifiTransferWidget), m_tcpServer(new QTcpServer(this))
{
    ui->setupUi(this);
    setWindowTitle(tr("Transfert Wi-Fi"));

    m_trayIcon = new QSystemTrayIcon(QIcon(":/icons/download.svg"), this);

    connect(ui->historyList, &QListWidget::itemDoubleClicked, this,
            [this](QListWidgetItem *item) {
                const QString path = item->data(Qt::UserRole).toString();
                QDesktopServices::openUrl(QUrl::fromLocalFile(path));
            });

    connect(ui->backButton, &QPushButton::clicked, this, [this]() {
        close();
        if (auto mw = MainWindow::getInstance())
            mw->showFullScreen();
    });

    const QString url = startServer();
    if (!url.isEmpty()) {
        QImage qr = generateQr(url);
        ui->qrLabel->setPixmap(QPixmap::fromImage(qr));
        ui->statusLabel->setText(tr("En attente de connexion..."));
    } else {
        ui->qrLabel->setText(tr("Impossible de démarrer le serveur"));
    }
}

WifiTransferWidget::~WifiTransferWidget() {
    stopServer();
    delete ui;
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
        QByteArray header;
        QByteArray body;
        bool       headerParsed   = false;
        qint64     contentLength  = -1;
        qint64     receivedTotal  = 0;   // header + body lu jusqu'ici
        QByteArray boundary;
        bool       finished       = false;
    };

    auto ctx = new ClientContext;

    // Nettoyage unique à la déconnexion
    connect(client, &QTcpSocket::disconnected, client, [client, ctx]() {
        client->deleteLater();   // objet Qt → deleteLater
        delete ctx;              // notre struct simple
    });

    connect(client, &QTcpSocket::readyRead, this, [=]() {
        const QByteArray chunk = client->readAll();
        ctx->receivedTotal += chunk.size();
        ctx->body += chunk;

        // 1) Tant qu’on n’a pas tout l’entête, on attend
        if (!ctx->headerParsed) {
            const int headerEnd = ctx->body.indexOf("\r\n\r\n");
            if (headerEnd < 0) {
                // Protection taille
                if (ctx->receivedTotal > MAX_UPLOAD_BYTES) {
                    client->write("HTTP/1.1 413 Payload Too Large\r\n\r\n");
                    client->disconnectFromHost();
                }
                return;
            }

            ctx->header = ctx->body.left(headerEnd);
            ctx->body   = ctx->body.mid(headerEnd + 4);
            ctx->headerParsed = true;

            const QString headerStr = QString::fromUtf8(ctx->header);

            // ----------- GET /upload -----------
            if (headerStr.startsWith("GET /upload")) {
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
                return; // PAS de delete ctx ici !
            }

            // ----------- POST /upload -----------
            if (headerStr.startsWith("POST /upload")) {

                // Content-Length
                QRegularExpression reLen("Content-Length:\\s*(\\d+)", QRegularExpression::CaseInsensitiveOption);
                auto mLen = reLen.match(headerStr);
                if (mLen.hasMatch())
                    ctx->contentLength = mLen.captured(1).toLongLong();

                if (ctx->contentLength <= 0 || ctx->contentLength > MAX_UPLOAD_BYTES) {
                    client->write("HTTP/1.1 413 Payload Too Large\r\nContent-Type: text/plain\r\n\r\nTaille invalide (>20Mo?)");
                    client->flush();
                    client->disconnectFromHost();
                    return;
                }

                // boundary multipart
                QRegularExpression reBound("boundary=([^;\\r\\n]+)");
                auto mBound = reBound.match(headerStr);
                if (mBound.hasMatch())
                    ctx->boundary = mBound.captured(1).toUtf8();
                else {
                    client->write("HTTP/1.1 400 Bad Request\r\n\r\nPas de boundary multipart");
                    client->flush();
                    client->disconnectFromHost();
                    return;
                }

                // Init barre de progression
                if (ui->progressBar) {
                    int maxVal = ctx->contentLength > INT_MAX ? INT_MAX : static_cast<int>(ctx->contentLength);
                    ui->progressBar->setRange(0, maxVal);
                    ui->progressBar->setValue(ctx->body.size());
                }
                if (ui->statusLabel) {
                    ui->statusLabel->setStyleSheet("");
                    ui->statusLabel->setText(tr("Réception : %1 / %2 octets")
                                            .arg(ctx->body.size()).arg(ctx->contentLength));
                }
            }
            else {
                // favicon.ico, OPTIONS, etc.
                client->write("HTTP/1.1 404 Not Found\r\n\r\n");
                client->flush();
                client->disconnectFromHost();
                return;
            }
        }
        else {
            // Mise à jour progression
            if (ui->progressBar) {
                int val = ctx->receivedTotal > INT_MAX ? INT_MAX : static_cast<int>(ctx->receivedTotal);
                ui->progressBar->setValue(val);
            }
            if (ui->statusLabel) {
                ui->statusLabel->setText(tr("Réception : %1 / %2 octets")
                                        .arg(ctx->receivedTotal).arg(ctx->contentLength + ctx->header.size() + 4));
            }
        }

        // 2) Si on a tout reçu (header + body)
        const qint64 expected = ctx->contentLength + ctx->header.size() + 4; // 4 = "\r\n\r\n"
        if (ctx->headerParsed && ctx->receivedTotal >= expected && !ctx->finished) {
            ctx->finished = true;  // empêche double passage

            // ---- Parse multipart ----
            QList<MultipartPart> parts = parseMultipartFormData(ctx->body, ctx->boundary);
            if (parts.isEmpty()) {
                client->write("HTTP/1.1 400 Bad Request\r\n\r\nAucune image valide");
                client->flush();
                client->disconnectFromHost();
                return;
            }

            const QString saveDir = qApp->applicationDirPath() + "/images_generees";
            QDir().mkpath(saveDir);

            QStringList savedFiles;
            for (const MultipartPart &p : parts) {
                QString safe = sanitizeFileName(p.filename);
                QString ext  = QFileInfo(safe).suffix();
                if (!isAllowedExt(ext))
                    continue;

                QString path = saveDir + '/' +
                               QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_") + safe;

                QFile out(path);
                if (!out.open(QIODevice::WriteOnly)) continue;
                out.write(p.data);
                out.close();

                QImageReader reader(path);
                if (!reader.canRead()) { QFile::remove(path); continue; }

                savedFiles << path;

                // UI: dernière miniature
                if (!p.data.isEmpty()) {
                    QPixmap px(path);
                    if (!px.isNull())
                        ui->imageLabel->setPixmap(px.scaled(ui->imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                }

                QListWidgetItem *it = new QListWidgetItem(QFileInfo(path).fileName() +
                                                          " - " + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
                it->setData(Qt::UserRole, path);
                ui->historyList->addItem(it);
            }

            if (savedFiles.isEmpty()) {
                client->write("HTTP/1.1 415 Unsupported Media Type\r\n\r\nAucun fichier image accepté");
                client->flush();
                client->disconnectFromHost();
                return;
            }

            // Succès UI
            if (ui->statusLabel) {
                ui->statusLabel->setStyleSheet("color:#2e7d32;");
                ui->statusLabel->setText(tr("Réception terminée ✅ (%1 fichier(s))").arg(savedFiles.size()));
            }
            if (ui->progressBar)
                ui->progressBar->setValue(ui->progressBar->maximum());

            QMessageBox::information(this, tr("Transfert Wi-Fi"),
                                     tr("%1 image(s) reçue(s)").arg(savedFiles.size()));

            if (QSystemTrayIcon::isSystemTrayAvailable() && !isActiveWindow()) {
                m_trayIcon->show();
                m_trayIcon->showMessage(tr("Transfert Wi-Fi"),
                                        tr("%1 image(s) reçue(s)").arg(savedFiles.size()));
            }
            client->write("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nImages enregistrées !");
            client->flush();
            client->disconnectFromHost();
        }
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
    QString base = QFileInfo(name).fileName();
    base.replace(QRegularExpression("[^A-Za-z0-9._-]"), "_");
    if (base.isEmpty()) base = "fichier";
    return base;
}

bool WifiTransferWidget::isAllowedExt(const QString &ext) const {
    static const QStringList ok = {"jpg","jpeg","png","gif","bmp","webp"};
    return ok.contains(ext.toLower());
}
