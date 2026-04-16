#ifndef AIDIALOGCOORDINATOR_H
#define AIDIALOGCOORDINATOR_H
//test
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

class AIDialogCoordinator : public QObject
{
    Q_OBJECT
public:
    explicit AIDialogCoordinator(QObject *parent = nullptr);
    void setDialogParent(QWidget *parent);

    bool openGenerationPrompt(QWidget *parent, AiGenerationRequest &request) const;

public slots:
    void onGenerationStatus(const QString &msg);
    void onAiImageReady(const QString &imagePath);

signals:
    void generationStatusChanged(const QString &msg);
    void imageReadyForImport(const QString &path,
                             bool internalContours,
                             bool colorEdges);

private:
    QWidget *m_dialogParent = nullptr;
};

#endif // AIDIALOGCOORDINATOR_H .
