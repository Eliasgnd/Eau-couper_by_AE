#ifndef OPENAISERVICE_H
#define OPENAISERVICE_H

#include <QObject>

class QNetworkAccessManager;

class OpenAIService : public QObject
{
    Q_OBJECT

public:
    explicit OpenAIService(QObject *parent = nullptr);

    void requestImageGeneration(const QString &userPrompt,
                                const QString &model,
                                const QString &quality,
                                const QString &size,
                                bool colorPrompt);

signals:
    void statusUpdate(const QString &message);
    void generationFinished(bool success, const QString &filePathOrError);

private:
    QNetworkAccessManager *m_netManager = nullptr;
};

#endif // OPENAISERVICE_H
