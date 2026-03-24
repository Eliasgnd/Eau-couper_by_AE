#include "MainWindowMenuBuilder.h"

#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QToolButton>
#include <QIcon>

namespace MainWindowMenuBuilder {

MenuHandles build(QMenuBar *menuBar,
                  QObject *parent,
                  const QString &settingsText,
                  const QString &languageText,
                  const QString &frenchText,
                  const QString &englishText,
                  const QString &wifiConfigText,
                  const std::function<void()> &onFrench,
                  const std::function<void()> &onEnglish,
                  const std::function<void()> &onWifiConfig)
{
    MenuHandles handles;

    handles.languageMenu = new QMenu(languageText, parent);
    handles.actionFrench = handles.languageMenu->addAction(frenchText);
    handles.actionEnglish = handles.languageMenu->addAction(englishText);

    QObject::connect(handles.actionFrench, &QAction::triggered, parent, [onFrench]() {
        onFrench();
    });
    QObject::connect(handles.actionEnglish, &QAction::triggered, parent, [onEnglish]() {
        onEnglish();
    });

    handles.settingsMenu = new QMenu(settingsText, parent);
    handles.actionWifiConfig = handles.settingsMenu->addAction(wifiConfigText);
    handles.settingsMenu->addMenu(handles.languageMenu);

    QObject::connect(handles.actionWifiConfig, &QAction::triggered, parent, [onWifiConfig]() {
        onWifiConfig();
    });

    auto *settingsButton = new QToolButton(qobject_cast<QWidget*>(parent));
    settingsButton->setObjectName("settingsToolButton");
    settingsButton->setText(settingsText);
    settingsButton->setIcon(QIcon(":/icons/settings.svg"));
    settingsButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    settingsButton->setPopupMode(QToolButton::InstantPopup);
    settingsButton->setMenu(handles.settingsMenu);

    menuBar->setCornerWidget(settingsButton, Qt::TopRightCorner);

    return handles;
}

} // namespace MainWindowMenuBuilder
