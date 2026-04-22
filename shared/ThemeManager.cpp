#include "ThemeManager.h"
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QSettings>

ThemeManager* ThemeManager::instance()
{
    static ThemeManager inst;
    return &inst;
}

ThemeManager::ThemeManager(QObject *parent) : QObject(parent)
{
    QSettings s("EauCouper", "IHM");
    m_isDark = s.value("theme/dark", false).toBool();
}

void ThemeManager::setDark(bool dark)
{
    m_isDark = dark;
    QSettings("EauCouper", "IHM").setValue("theme/dark", m_isDark);
    applyToApp();
    emit themeChanged(m_isDark);
}

void ThemeManager::applyToApp()
{
    QString path = m_isDark ? ":/styles/style.qss" : ":/styles/style_light.qss";
    QFile f(path);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text))
        qApp->setStyleSheet(QTextStream(&f).readAll());
}
