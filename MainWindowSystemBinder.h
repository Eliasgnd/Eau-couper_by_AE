#pragma once

class QObject;

namespace Ui {
class MainWindow;
}

class ShapeController;
class ShapeVisualization;

namespace MainWindowSystemBinder {

void bind(Ui::MainWindow *ui,
          ShapeController *shapeController,
          ShapeVisualization *shapeVisualization,
          QObject *context);

} // namespace MainWindowSystemBinder
