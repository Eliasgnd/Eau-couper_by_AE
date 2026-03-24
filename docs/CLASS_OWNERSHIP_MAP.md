# Class Ownership Map

Ce document donne une **indication claire de rattachement** des classes pour tout le projet, afin de réduire les confusions entre modules (ex: `ShapeVisualization` vs `CustomDrawArea`).

## Convention de lecture

- Le **module** est défini par le dossier racine/fonctionnel.
- Chaque entrée liste les classes trouvées dans le header.

## ImageImportService.h

- `ImageImportService.h`: `ImageImportService`

## Language.h

- `Language.h`: `Language`

## MainWindow.h

- `MainWindow.h`: `MainWindow`, `QAction`, `QMenu`, `ShapeVisualization`, `NavigationController`, `AIServiceManager`, `ShapeController`, `MainWindow`

## ShapeController.h

- `ShapeController.h`: `ShapeVisualization`, `ShapeProjectModel`, `ShapeController`

## drawing/DrawModeManager.h

- `drawing/DrawModeManager.h`: `DrawModeManager`, `DrawMode`

## drawing/EraserTool.h

- `drawing/EraserTool.h`: `ShapeManager`, `EraserTool`

## drawing/HistoryManager.h

- `drawing/HistoryManager.h`: `HistoryManager`

## drawing/MouseInteractionHandler.h

- `drawing/MouseInteractionHandler.h`: `QMouseEvent`, `DrawModeManager`, `ShapeManager`, `ViewTransformer`, `MouseInteractionHandler`, `MouseInteractionHandler`

## drawing/PathGenerator.h

- `drawing/PathGenerator.h`: `PathGenerator`

## drawing/ShapeManager.h

- `drawing/ShapeManager.h`: `ShapeManager`, `ShapeManager`

## drawing/ShapeRenderer.h

- `drawing/ShapeRenderer.h`: `QPainter`, `ShapeManager`, `ShapeRenderer`

## drawing/TextTool.h

- `drawing/TextTool.h`: `QString`, `TextTool`

## drawing/ViewTransformer.h

- `drawing/ViewTransformer.h`: `ViewTransformer`

## drawing/shapes

- `drawing/shapes/CustomDrawArea.h`: `QMouseEvent`, `QPaintEvent`, `QWheelEvent`, `EraserTool`, `HistoryManager`, `MouseInteractionHandler`, `ShapeRenderer`, `TextTool`, `ViewTransformer`, `CustomDrawArea`, `CustomDrawArea`

## drawing/tools

- `drawing/tools/ImageEdgeImporter.h`: `ImageEdgeImporter`
- `drawing/tools/ImportedImageGeometryHelper.h`: `ImportedImageGeometryHelper`
- `drawing/tools/LogoImporter.h`: `LogoImporter`
- `drawing/tools/TouchGestureReader.h`: `TouchGestureReader`
- `drawing/tools/TrajetMotor.h`: `MainWindow`, `TrajetMotor`
- `drawing/tools/pathplanner.h`: `PathPlanner`
- `drawing/tools/skeletonizer.h`: `Skeletonizer`

## drawing/utils

- `drawing/utils/AspectRatioWrapper.h`: `AspectRatioWrapper`
- `drawing/utils/GeometryUtils.h`: `CutQueue`
- `drawing/utils/ImageExporter.h`: `QGraphicsScene`, `ImageExporter`
- `drawing/utils/PlacementOptimizer.h`: `PlacementOptimizer`
- `drawing/utils/shapevisualization/GeometryTransformHelper.h`: `GeometryTransformHelper`
- `drawing/utils/shapevisualization/GridPlacementService.h`: `GridPlacementService`
- `drawing/utils/shapevisualization/ShapeValidationService.h`: `ShapeValidationService`

## managers/ai

- `managers/ai/AIServiceManager.h`: `QWidget`, `AIServiceManager`
- `managers/ai/OpenAIService.h`: `QNetworkAccessManager`, `OpenAIService`

## managers/navigation

- `managers/navigation/AppController.h`: `MainWindow`, `ShapeVisualization`, `TrajetMotor`, `OpenAIService`, `AppController`
- `managers/navigation/NavigationController.h`: `QWidget`, `CustomEditor`, `WifiTransferWidget`, `WifiConfigDialog`, `BluetoothReceiverDialog`, `TestGpio`, `FolderWidget`, `LayoutsDialog`, `ShapeVisualization`, `NavigationController`

## managers/system

- `managers/system/GestureHandler.h`: `QGestureEvent`, `QPinchGesture`, `GestureHandler`
- `managers/system/MotorControl.h`: `MotorControl`
- `managers/system/Raspberry.h`: `Raspberry`

## models/Inventory.h

- `models/Inventory.h`: `Inventory`, `Inventory`

## models/ShapeModel.h

- `models/ShapeModel.h`: `ShapeModel`, `Type`

## tests

- `tests/inventory_safety_tests.h`: `InventorySafetyTests`
- `tests/placement_tests.h`: `PlacementTests`
- `tests/shape_manager_tests.h`: `ShapeManagerTests`

## ui/dialogs

- `ui/dialogs/AIImageProcessDialog.h`: `QLabel`, `QPushButton`, `AIImageProcessDialog`
- `ui/dialogs/AIImagePromptDialog.h`: `AIImagePromptDialog`
- `ui/dialogs/BluetoothReceiverDialog.h`: `BluetoothReceiverDialog`, `BluetoothReceiverDialog`
- `ui/dialogs/TestGpio.h`: `TestGpio`, `TestGpio`
- `ui/dialogs/WifiConfigDialog.h`: `WifiConfigDialog`, `WifiConfigDialog`

## ui/widgets

- `ui/widgets/CustomEditor.h`: `CustomEditor`, `CustomEditor`
- `ui/widgets/FolderWidget.h`: `QLineEdit`, `QComboBox`, `QSlider`, `QScrollBar`, `QLayout`, `QListWidgetItem`, `FolderWidget`, `FolderWidget`
- `ui/widgets/KeyboardDialog.h`: `KeyboardDialog`
- `ui/widgets/KeyboardEventFilter.h`: `ShapeVisualization`, `KeyboardEventFilter`
- `ui/widgets/LayoutsDialog.h`: `LayoutsDialog`, `LayoutsDialog`
- `ui/widgets/NumericKeyboardDialog.h`: `NumericKeyboardDialog`
- `ui/widgets/ShapeVisualization.h`: `ShapeVisualization`
- `ui/widgets/WifiTransferWidget.h`: `WifiTransferWidget`, `WifiTransferWidget`
- `ui/widgets/shapevisualization/LayoutManager.h`: `LayoutManager`
- `ui/widgets/shapevisualization/ShapeProjectModel.h`: `ShapeProjectModel`
