#pragma once

#include <functional>
#include <QString>

class QMenu;
class QAction;
class QToolButton;
class QMenuBar;
class QObject;

namespace MainWindowMenuBuilder {

struct MenuHandles {
    QMenu *settingsMenu = nullptr;
    QMenu *languageMenu = nullptr;
    QAction *actionFrench = nullptr;
    QAction *actionEnglish = nullptr;
    QAction *actionWifiConfig = nullptr;
};

MenuHandles build(QMenuBar *menuBar,
                  QObject *parent,
                  const QString &settingsText,
                  const QString &languageText,
                  const QString &frenchText,
                  const QString &englishText,
                  const QString &wifiConfigText,
                  const std::function<void()> &onFrench,
                  const std::function<void()> &onEnglish,
                  const std::function<void()> &onWifiConfig);

} // namespace MainWindowMenuBuilder
