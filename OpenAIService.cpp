#include "OpenAIService.h"

#include "ImagePaths.h"

#include <QDateTime>
#include <QDir>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>

OpenAIService::OpenAIService(QObject *parent)
    : QObject(parent)
{
}

void OpenAIService::requestImageGeneration(const QString &userPrompt,
                                           const QString &model,
                                           const QString &quality,
                                           const QString &size,
                                           bool colorPrompt)
{
    if (userPrompt.isEmpty()) {
        emit generationFinished(false, tr("Le prompt est vide."));
        return;
    }

    QString finalPrompt;
    if (colorPrompt) {
        finalPrompt =
            "A minimal flat icon of a " + userPrompt +
            ", centered, on a white background. Clean shapes, no outlines, no text, no decorations, no shadow, no background. Suitable for icon design or sticker. Simple, geometric, and immediately recognizable.";
    } else {
        const QString startPrompt = "A single black outline drawing of a ";
        const QString styleSuffix =
            ", only the outer edge, no internal lines, no doors, no windows, no shading, no textures, white background, vector style, icon-like, extremely minimal";
        finalPrompt = startPrompt + userPrompt + styleSuffix;
    }

    if (!m_netManager)
        m_netManager = new QNetworkAccessManager(this);

    emit statusUpdate(tr("🧠 Génération en cours..."));

    const QString modelStr = model.isEmpty() ? QStringLiteral("dall-e-3") : model;
    const QString qualityStr = quality.isEmpty() ? QStringLiteral("standard") : quality;
    const QString sizeStr = size.isEmpty() ? QStringLiteral("1024x1024") : size;

    QNetworkRequest req(QUrl("https://api.openai.com/v1/images/generations"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    const QByteArray apiKey = qEnvironmentVariable("OPENAI_API_KEY").toUtf8();
    if (apiKey.isEmpty()) {
        emit statusUpdate(tr("❌ Clé API manquante"));
        emit generationFinished(false, tr("La clé API OpenAI est absente. Définissez OPENAI_API_KEY."));
        return;
    }
    req.setRawHeader("Authorization", "Bearer " + apiKey);

    const QJsonObject body{
        {"model", modelStr},
        {"prompt", finalPrompt},
        {"n", 1},
        {"size", sizeStr},
        {"quality", qualityStr}
    };

    QNetworkReply *reply = m_netManager->post(req, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply, userPrompt]() {
        if (reply->error() != QNetworkReply::NoError) {
            const QString error = reply->errorString();
            reply->deleteLater();
            emit statusUpdate(tr("❌ Erreur API"));
            emit generationFinished(false, error);
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        reply->deleteLater();

        const QJsonArray dataArray = doc.object().value("data").toArray();
        if (dataArray.isEmpty()) {
            emit statusUpdate(tr("❌ Réponse API invalide"));
            emit generationFinished(false, tr("Réponse API invalide: image introuvable."));
            return;
        }

        const QString url = dataArray.first().toObject().value("url").toString();
        if (url.isEmpty()) {
            emit statusUpdate(tr("❌ URL image manquante"));
            emit generationFinished(false, tr("Réponse API invalide: URL image manquante."));
            return;
        }

        QNetworkReply *imgReply = m_netManager->get(QNetworkRequest(QUrl(url)));
        connect(imgReply, &QNetworkReply::finished, this, [this, imgReply, userPrompt]() {
            if (imgReply->error() != QNetworkReply::NoError) {
                const QString error = imgReply->errorString();
                imgReply->deleteLater();
                emit statusUpdate(tr("❌ Erreur image"));
                emit generationFinished(false, error);
                return;
            }

            QImage img;
            img.loadFromData(imgReply->readAll());
            imgReply->deleteLater();

            if (img.isNull()) {
                emit statusUpdate(tr("❌ Image invalide"));
                emit generationFinished(false, tr("L'image générée est invalide."));
                return;
            }

            const QString imagesDirPath = ImagePaths::iaDir();
            QDir imagesDir(imagesDirPath);
            QString sanitized = userPrompt.normalized(QString::NormalizationForm_D);
            sanitized.remove(QRegularExpression("[\\p{Mn}]"));
            sanitized.replace(QRegularExpression("[^A-Za-z0-9]+"), "_");
            sanitized = sanitized.trimmed();
            if (sanitized.isEmpty())
                sanitized = "image";

            const QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm");
            const QString archiveFile = imagesDir.filePath(timestamp + '_' + sanitized + ".png");
            if (!img.save(archiveFile)) {
                emit generationFinished(false, tr("Impossible d'enregistrer l'image générée."));
                return;
            }

            emit statusUpdate(QString());
            emit generationFinished(true, archiveFile);
        });
    });
}
