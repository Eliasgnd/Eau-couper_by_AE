#pragma once

#include <functional>

#include "Language.h"

namespace Ui {
class MainWindow;
}

class MainWindowCoordinator;
class QWidget;

namespace MainWindowNavigationBinder {

void bindNavigationButtons(Ui::MainWindow *ui,
                           MainWindowCoordinator *coordinator,
                           QWidget *mainWindow,
                           const std::function<Language()> &languageProvider);

} // namespace MainWindowNavigationBinder
