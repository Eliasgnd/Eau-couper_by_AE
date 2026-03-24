#ifndef AISERVICEMANAGER_H
#define AISERVICEMANAGER_H

#include <QObject>
#include <QString>

class QWidget;

struct AiGenerationRequest {
    QString prompt;
    QString model;
    QString quality;
    QString size;
    bool colorPrompt = false;
};

struct AiImageProcessingOptions {
    bool internalContours = false;
    bool colorEdges = false;
};

class AIServiceManager : public QObject
{
    Q_OBJECT
public:
    explicit AIServiceManager(QObject *parent = nullptr);

    bool openGenerationPrompt(QWidget *parent, AiGenerationRequest &request) const;
    bool resolveImageProcessingOptions(QWidget *parent,
                                       const QString &imagePath,
                                       AiImageProcessingOptions &options,
                                       QString &errorMessage) const;
};

#endif // AISERVICEMANAGER_H
