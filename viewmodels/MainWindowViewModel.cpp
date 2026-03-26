#include "MainWindowViewModel.h"

MainWindowViewModel::MainWindowViewModel(QObject *parent)
    : QObject(parent)
{
}

void MainWindowViewModel::setCuttingActive(bool active)
{
    if (m_cuttingActive == active)
        return;
    m_cuttingActive = active;
    emit cuttingActiveChanged(active);
}

void MainWindowViewModel::setCutProgress(int percent, const QString &timeRemaining)
{
    m_cuttingProgress = percent;
    m_cuttingTimeRemaining = timeRemaining;
    emit cutProgressChanged(percent, timeRemaining);
}

void MainWindowViewModel::setCutFinished(bool success)
{
    m_cuttingActive = false;
    emit cutFinished(success);
}

void MainWindowViewModel::setControlsEnabled(bool enabled)
{
    if (m_controlsEnabled == enabled)
        return;
    m_controlsEnabled = enabled;
    emit controlsEnabledChanged(enabled);
}

void MainWindowViewModel::setLanguage(Language lang, bool ok)
{
    m_language = lang;
    emit languageChanged(lang, ok);
}

void MainWindowViewModel::setAiGenerating(bool generating)
{
    if (m_aiGenerating == generating)
        return;
    m_aiGenerating = generating;
    emit aiGeneratingChanged(generating);
}
