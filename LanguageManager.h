#ifndef LANGUAGEMANAGER_H
#define LANGUAGEMANAGER_H

#include <QObject>

class LanguageManager : public QObject
{
    Q_OBJECT
public:
    enum class Language { French, English };

    static LanguageManager* instance();

    Language currentLanguage() const;
    void setLanguage(Language lang);

signals:
    void languageChanged(Language lang);

private:
    explicit LanguageManager(QObject *parent = nullptr);
    Language m_language = Language::French;
};

#endif // LANGUAGEMANAGER_H
