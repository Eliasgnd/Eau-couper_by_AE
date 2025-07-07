#include "LanguageManager.h"

LanguageManager* LanguageManager::instance()
{
    static LanguageManager inst;
    return &inst;
}

LanguageManager::LanguageManager(QObject *parent)
    : QObject(parent)
{
}

LanguageManager::Language LanguageManager::currentLanguage() const
{
    return m_language;
}

void LanguageManager::setLanguage(Language lang)
{
    if (m_language == lang)
        return;
    m_language = lang;
    emit languageChanged(lang);
}
