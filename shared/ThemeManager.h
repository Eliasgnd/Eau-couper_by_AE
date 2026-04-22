#pragma once
#include <QObject>

// Gestionnaire de thème global — singleton.
// Toutes les pages se connectent à themeChanged(bool) pour rester en sync.
class ThemeManager : public QObject
{
    Q_OBJECT
public:
    static ThemeManager* instance();

    bool  isDark() const { return m_isDark; }
    void  setDark(bool dark);
    void  toggle() { setDark(!m_isDark); }
    void  applyToApp();   // applique le QSS sur qApp

signals:
    void themeChanged(bool isDark);

private:
    explicit ThemeManager(QObject *parent = nullptr);
    bool m_isDark = false;
};
