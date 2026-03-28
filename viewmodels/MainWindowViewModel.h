#pragma once

#include <QObject>
#include <QString>
#include "Language.h"

// ViewModel de la fenêtre principale.
// Stocke l'état UI (découpe, progression, langue, IA, contrôles).
// MainWindowCoordinator met à jour ce ViewModel via les setters.
// MainWindow lit l'état initial via les getters, puis observe les signaux.
class MainWindowViewModel : public QObject
{
    Q_OBJECT

public:
    explicit MainWindowViewModel(QObject *parent = nullptr);

    // --- Getters (état initial pour MainWindow) ---
    bool    isCuttingActive()       const { return m_cuttingActive; }
    int     cuttingProgress()       const { return m_cuttingProgress; }
    QString cuttingTimeRemaining()  const { return m_cuttingTimeRemaining; }
    bool    controlsEnabled()       const { return m_controlsEnabled; }
    Language currentLanguage()      const { return m_language; }
    bool    isAiGenerating()        const { return m_aiGenerating; }

    // --- Setters appelés par MainWindowCoordinator ---
    void setCuttingActive(bool active);
    void setCutProgress(int percent, const QString &timeRemaining);
    void setCutFinished(bool success);
    void setControlsEnabled(bool enabled);
    void setLanguage(Language lang, bool ok);
    void setAiGenerating(bool generating);
    void setAiStatus(const QString &status);
    void setShapeProgress(int current, int total);
    void setAiImageReceived();

signals:
    // --- Signaux écoutés par MainWindow ---
    void cuttingActiveChanged(bool active);
    void cutProgressChanged(int percent, const QString &timeRemaining);
    void cutFinished(bool success);
    void controlsEnabledChanged(bool enabled);
    void languageChanged(Language lang, bool ok);
    void aiGeneratingChanged(bool generating);
    void aiStatusChanged(const QString &status);
    void shapeProgressChanged(int current, int total);
    void aiImageReceived();

private:
    bool     m_cuttingActive        = false;
    int      m_cuttingProgress      = 0;
    QString  m_cuttingTimeRemaining;
    bool     m_controlsEnabled      = true;
    Language m_language             = Language::French;
    bool     m_aiGenerating         = false;
};
