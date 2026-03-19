#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include <QTranslator>
#include "Language.h"

class MainWindow;
class FormeVisualization;
class TrajetMotor;
class OpenAIService;

class AppController : public QObject
{
    Q_OBJECT
public:
    explicit AppController(QObject *parent = nullptr);

    void setMainWindow(MainWindow *window);
    void setFormeVisualization(FormeVisualization *visualization);

public slots:
    void startCutting();
    void stopCutting();
    void pauseCutting();

    void requestAiGeneration(const QString &prompt,
                             const QString &model,
                             const QString &quality,
                             const QString &size,
                             bool colorPrompt);

    void changeLanguage(Language lang);
    void openWifiSettings();
    void openBluetoothReceiver();
    void openTestGpio();
    void openDossier();

signals:
    void cutProgressUpdated(int percent, const QString &timeTxt);
    void cutFinished(bool success);
    void cutControlsEnabled(bool enabled);

    void aiGenerationStatus(const QString &msg);
    void aiImageReady(const QString &path);

    void languageApplied(Language lang, bool ok);

private:
    bool ensureServicesInitialized();
    bool loadLanguage(Language lang);

    MainWindow *m_mainWindow = nullptr;
    FormeVisualization *m_formeVisualization = nullptr;

    TrajetMotor *m_trajetMotor = nullptr;
    OpenAIService *m_aiService = nullptr;
    QTranslator m_translator;
    Language m_currentLanguage = Language::French;
    bool m_pauseRequested = false;
};

#endif // APPCONTROLLER_H
