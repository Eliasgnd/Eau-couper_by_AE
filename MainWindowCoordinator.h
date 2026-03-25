#pragma once

#include <QObject>
#include <QList>
#include <QPolygonF>
#include <QString>

#include "Language.h"
#include "AIServiceManager.h"
#include "ShapeModel.h"

class MainWindow;
class NavigationController;
class ShapeController;
class WorkspaceModel;
class QWidget;
class ShapeVisualization;
struct LayoutData;

class MainWindowCoordinator : public QObject
{
    Q_OBJECT

public:
    MainWindowCoordinator(NavigationController *navigationController,
                          AIServiceManager *aiServiceManager,
                          ShapeController *shapeController,
                          WorkspaceModel *model,
                          QObject *parent = nullptr);

    NavigationController *navigationController() const { return m_navigationController; }
    AIServiceManager     *aiServiceManager()     const { return m_aiServiceManager; }
    ShapeController      *shapeController()      const { return m_shapeController; }
    WorkspaceModel       *workspaceModel()       const { return m_model; }

    void setDialogParent(QWidget *parent) { m_dialogParent = parent; }

    // Connecte le Coordinator aux signaux de la View
    void connectToView(MainWindow *view);

    // --- Actions déléguées (navigation) ---
    void openWifiSettings(QWidget *parent);
    void openInventory(QWidget *mainWindow, QWidget *inventoryWindow);
    void openCustomEditor(QWidget *mainWindow, Language language);
    void openFolder(QWidget *mainWindow, Language language);
    void openTestGpio(QWidget *mainWindow);
    void openBluetoothReceiver(QWidget *mainWindow);
    void openWifiTransfer(QWidget *mainWindow);

    void handleSaveLayoutRequest(QWidget *mainWindow,
                                 ShapeVisualization *visualization,
                                 ShapeModel::Type selectedType);

    bool openAiGenerationPrompt(QWidget *parent, AiGenerationRequest &request);

    void openImageInCustom(const QString &filePath,
                           bool internalContours,
                           bool colorEdges);

public slots:
    // Réactions aux signaux de la View
    void onDimensionsChanged(int largeur, int longueur);
    void onShapeCountChanged(int count);
    void onSpacingChanged(int spacing);

    void onCircleRequested();
    void onRectangleRequested();
    void onTriangleRequested();
    void onStarRequested();
    void onHeartRequested();

    void onOptimize1Requested(bool checked);
    void onOptimize2Requested(bool checked);

    void onMoveUpRequested();
    void onMoveDownRequested();
    void onMoveLeftRequested();
    void onMoveRightRequested();
    void onRotateLeftRequested();
    void onRotateRightRequested();
    void onAddShapeRequested();
    void onDeleteShapeRequested();

    void onInventoryRequested();
    void onCustomEditorRequested();
    void onFolderRequested();
    void onTestGpioRequested();
    void onBluetoothReceiverRequested();
    void onWifiTransferRequested();

    void onSaveLayoutRequested();
    void onGenerateAiRequested();

    // Réactions aux événements de l'Inventory
    void onCustomShapeSelected(const QList<QPolygonF> &polygons, const QString &name);
    void onShapeSelectedFromInventory(ShapeModel::Type type);

signals:
    void generationStatusChanged(const QString &status);
    void imageReadyForImport(const QString &path, bool internalContours, bool colorEdges);
    void requestShowFullScreen();

private:
    NavigationController *m_navigationController = nullptr;
    AIServiceManager     *m_aiServiceManager     = nullptr;
    ShapeController      *m_shapeController      = nullptr;
    WorkspaceModel       *m_model                = nullptr;

    QWidget  *m_dialogParent = nullptr;
    MainWindow *m_view       = nullptr;   // référence vers la View (pour slots uniquement)
};
