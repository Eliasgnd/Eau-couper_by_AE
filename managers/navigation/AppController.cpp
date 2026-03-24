#include "AppController.h"

#include "MainWindow.h"
#include "FormeVisualization.h"
#include "trajetmotor.h"
#include "OpenAIService.h"

#include <QApplication>

AppController::AppController(QObject *parent)
    : QObject(parent)
{
}

void AppController::setMainWindow(MainWindow *window)
{
    m_mainWindow = window;
}

void AppController::setFormeVisualization(FormeVisualization *visualization)
{
    m_formeVisualization = visualization;
    ensureServicesInitialized();
}

bool AppController::ensureServicesInitialized()
{
    if (!m_formeVisualization || !m_mainWindow) {
        return false;
    }

    if (!m_trajetMotor) {
        m_trajetMotor = new TrajetMotor(m_formeVisualization, m_mainWindow);

        connect(m_trajetMotor, &TrajetMotor::decoupeProgress, this,
                [this](int remaining, int total) {
                    const int percent = (total > 0) ? ((total - remaining) * 100) / total : 0;
                    const int timeSec = (remaining * 15) / 1000;
                    emit cutProgressUpdated(percent, tr("Temps restant estimé : %1s").arg(timeSec));
                });
    }

    if (!m_aiService) {
        m_aiService = new OpenAIService(this);
        connect(m_aiService, &OpenAIService::statusUpdate,
                this, &AppController::aiGenerationStatus);
        connect(m_aiService, &OpenAIService::generationFinished,
                this, [this](bool success, const QString &result) {
                    if (!success) {
                        emit aiGenerationStatus(result);
                        return;
                    }
                    emit aiImageReady(result);
                });
    }

    return true;
}

void AppController::startCutting()
{
    if (!ensureServicesInitialized()) return;

    if (m_trajetMotor->isPaused()) {
        m_trajetMotor->resume();
        m_pauseRequested = false;
        return;
    }

    m_formeVisualization->resetAllShapeColors();
    if (!m_formeVisualization->validateShapes()) {
        emit aiGenerationStatus(tr("Certaines formes dépassent la zone ou se chevauchent."));
        emit cutFinished(false);
        return;
    }

    emit cutControlsEnabled(false);
    m_formeVisualization->setDecoupeEnCours(true);
    m_formeVisualization->resetAllShapeColors();
    m_trajetMotor->executeTrajet();
}

void AppController::stopCutting()
{
    if (!ensureServicesInitialized()) return;
    m_trajetMotor->stopCut();
    m_formeVisualization->setDecoupeEnCours(false);
    emit cutProgressUpdated(0, tr("Temps restant estimé : 0s"));
    emit cutControlsEnabled(true);
    emit cutFinished(true);
}

void AppController::pauseCutting()
{
    if (!ensureServicesInitialized()) return;

    if (!m_pauseRequested) {
        m_trajetMotor->pause();
        m_pauseRequested = true;
    } else {
        m_trajetMotor->resume();
        m_pauseRequested = false;
    }
}

void AppController::requestAiGeneration(const QString &prompt,
                                        const QString &model,
                                        const QString &quality,
                                        const QString &size,
                                        bool colorPrompt)
{
    if (!ensureServicesInitialized()) return;
    m_aiService->requestImageGeneration(prompt, model, quality, size, colorPrompt);
}

bool AppController::loadLanguage(Language lang)
{
    qApp->removeTranslator(&m_translator);
    m_currentLanguage = lang;

    const QString locale = (lang == Language::French) ? "fr" : "en";
    const QString path = qApp->applicationDirPath()
                         + "/translations/machineDecoupeIHM_" + locale + ".qm";

    const bool ok = m_translator.load(path);
    if (ok) {
        qApp->installTranslator(&m_translator);
    }
    return ok;
}

void AppController::changeLanguage(Language lang)
{
    const bool ok = loadLanguage(lang);
    emit languageApplied(lang, ok);
}
