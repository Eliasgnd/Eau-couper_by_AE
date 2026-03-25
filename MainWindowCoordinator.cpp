#include "MainWindowCoordinator.h"

#include "MainWindow.h"
#include "NavigationController.h"
#include "ShapeController.h"
#include "ShapeVisualization.h"
#include "WorkspaceModel.h"
#include "Inventory.h"
#include "BaseShapeNamingService.h"
#include "ImageImportService.h"

MainWindowCoordinator::MainWindowCoordinator(NavigationController *navigationController,
                                             AIServiceManager *aiServiceManager,
                                             ShapeController *shapeController,
                                             WorkspaceModel *model,
                                             QObject *parent)
    : QObject(parent)
    , m_navigationController(navigationController)
    , m_aiServiceManager(aiServiceManager)
    , m_shapeController(shapeController)
    , m_model(model)
{
    // Signaux internes AI → Coordinator
    connect(m_aiServiceManager, &AIServiceManager::generationStatusChanged,
            this, &MainWindowCoordinator::generationStatusChanged);
    connect(m_aiServiceManager, &AIServiceManager::imageReadyForImport,
            this, &MainWindowCoordinator::imageReadyForImport);

    // Inventory → Coordinator (on connecte ici au lieu de MainWindow)
    connect(Inventory::getInstance(), &Inventory::shapeSelected,
            this, &MainWindowCoordinator::onShapeSelectedFromInventory);
    connect(Inventory::getInstance(), &Inventory::customShapeSelected,
            this, &MainWindowCoordinator::onCustomShapeSelected);
}

// =======================================================================
//  connectToView — Le Coordinator s'abonne aux signaux typés de la View.
//  Aucune référence à Ui::MainWindow ni aux widgets.
// =======================================================================
void MainWindowCoordinator::connectToView(MainWindow *view)
{
    m_view = view;

    // --- Dimensions ---
    connect(view, &MainWindow::dimensionsChangeRequested,
            this, &MainWindowCoordinator::onDimensionsChanged);

    // --- Nombre / espacement ---
    connect(view, &MainWindow::shapeCountChangeRequested,
            this, &MainWindowCoordinator::onShapeCountChanged);
    connect(view, &MainWindow::spacingChangeRequested,
            this, &MainWindowCoordinator::onSpacingChanged);

    // --- Formes prédéfinies ---
    connect(view, &MainWindow::circleRequested,    this, &MainWindowCoordinator::onCircleRequested);
    connect(view, &MainWindow::rectangleRequested,  this, &MainWindowCoordinator::onRectangleRequested);
    connect(view, &MainWindow::triangleRequested,   this, &MainWindowCoordinator::onTriangleRequested);
    connect(view, &MainWindow::starRequested,       this, &MainWindowCoordinator::onStarRequested);
    connect(view, &MainWindow::heartRequested,      this, &MainWindowCoordinator::onHeartRequested);

    // --- Optimisation ---
    connect(view, &MainWindow::optimizePlacement1Requested,
            this, &MainWindowCoordinator::onOptimize1Requested);
    connect(view, &MainWindow::optimizePlacement2Requested,
            this, &MainWindowCoordinator::onOptimize2Requested);

    // --- Déplacement / rotation / ajout / suppression ---
    connect(view, &MainWindow::moveUpRequested,     this, &MainWindowCoordinator::onMoveUpRequested);
    connect(view, &MainWindow::moveDownRequested,    this, &MainWindowCoordinator::onMoveDownRequested);
    connect(view, &MainWindow::moveLeftRequested,    this, &MainWindowCoordinator::onMoveLeftRequested);
    connect(view, &MainWindow::moveRightRequested,   this, &MainWindowCoordinator::onMoveRightRequested);
    connect(view, &MainWindow::rotateLeftRequested,  this, &MainWindowCoordinator::onRotateLeftRequested);
    connect(view, &MainWindow::rotateRightRequested, this, &MainWindowCoordinator::onRotateRightRequested);
    connect(view, &MainWindow::addShapeRequested,    this, &MainWindowCoordinator::onAddShapeRequested);
    connect(view, &MainWindow::deleteShapeRequested, this, &MainWindowCoordinator::onDeleteShapeRequested);

    // --- Navigation ---
    connect(view, &MainWindow::inventoryRequested,         this, &MainWindowCoordinator::onInventoryRequested);
    connect(view, &MainWindow::customEditorRequested,       this, &MainWindowCoordinator::onCustomEditorRequested);
    connect(view, &MainWindow::folderRequested,             this, &MainWindowCoordinator::onFolderRequested);
    connect(view, &MainWindow::testGpioRequested,           this, &MainWindowCoordinator::onTestGpioRequested);
    connect(view, &MainWindow::bluetoothReceiverRequested,  this, &MainWindowCoordinator::onBluetoothReceiverRequested);
    connect(view, &MainWindow::wifiTransferRequested,       this, &MainWindowCoordinator::onWifiTransferRequested);

    // --- Sauvegarde / AI ---
    connect(view, &MainWindow::saveLayoutRequested, this, &MainWindowCoordinator::onSaveLayoutRequested);
    connect(view, &MainWindow::generateAiRequested, this, &MainWindowCoordinator::onGenerateAiRequested);

    // --- NavigationController → View (layout, shapes, fullscreen) ---
    connect(m_navigationController, &NavigationController::customShapeApplied,
            view, [view](const QList<QPolygonF> &shapes) {
                view->displayCustomShapes(shapes, QString());
            });

    connect(m_navigationController, &NavigationController::layoutSelected,
            view, &MainWindow::applyLayout);

    connect(m_navigationController, &NavigationController::baseShapeLayoutSelected,
            view, &MainWindow::applyBaseShapeLayout);

    connect(m_navigationController, &NavigationController::baseShapeOnlySelected, this,
            [this](ShapeModel::Type type) {
                m_shapeController->setPredefinedShape(type);
            });

    connect(m_navigationController, &NavigationController::requestReturnToFullScreen, view,
            [view]() { view->showFullScreen(); });

    connect(m_navigationController, &NavigationController::requestOpenInventory, this,
            []() { Inventory::getInstance()->showFullScreen(); });

    // --- imageReadyForImport → ouvre dans l'éditeur custom ---
    connect(this, &MainWindowCoordinator::imageReadyForImport, this,
            [this](const QString &path, bool internalContours, bool colorEdges) {
                openImageInCustom(path, internalContours, colorEdges);
            });
}

// =======================================================================
//  Slots — réactions aux signaux de la View
// =======================================================================

void MainWindowCoordinator::onDimensionsChanged(int largeur, int longueur)
{
    m_model->setDimensions(largeur, longueur);
    m_shapeController->updateShape(largeur, longueur);
}

void MainWindowCoordinator::onShapeCountChanged(int count)
{
    m_shapeController->updateShapeCount(count, m_model->largeur(), m_model->longueur());
}

void MainWindowCoordinator::onSpacingChanged(int spacing)
{
    m_model->setSpacing(spacing);
    m_shapeController->updateSpacing(spacing);
}

void MainWindowCoordinator::onCircleRequested()    { m_shapeController->setPredefinedShape(ShapeModel::Type::Circle); }
void MainWindowCoordinator::onRectangleRequested()  { m_shapeController->setPredefinedShape(ShapeModel::Type::Rectangle); }
void MainWindowCoordinator::onTriangleRequested()   { m_shapeController->setPredefinedShape(ShapeModel::Type::Triangle); }
void MainWindowCoordinator::onStarRequested()       { m_shapeController->setPredefinedShape(ShapeModel::Type::Star); }
void MainWindowCoordinator::onHeartRequested()      { m_shapeController->setPredefinedShape(ShapeModel::Type::Heart); }

void MainWindowCoordinator::onOptimize1Requested(bool checked)
{
    m_shapeController->onOptimizePlacementClicked(checked, m_model->largeur(), m_model->longueur());
}

void MainWindowCoordinator::onOptimize2Requested(bool checked)
{
    m_shapeController->onOptimizePlacement2Clicked(checked, m_model->largeur(), m_model->longueur());
}

void MainWindowCoordinator::onMoveUpRequested()     { m_shapeController->onMoveUpClicked(); }
void MainWindowCoordinator::onMoveDownRequested()    { m_shapeController->onMoveDownClicked(); }
void MainWindowCoordinator::onMoveLeftRequested()    { m_shapeController->onMoveLeftClicked(); }
void MainWindowCoordinator::onMoveRightRequested()   { m_shapeController->onMoveRightClicked(); }
void MainWindowCoordinator::onRotateLeftRequested()  { m_shapeController->rotateLeft(); }
void MainWindowCoordinator::onRotateRightRequested() { m_shapeController->rotateRight(); }
void MainWindowCoordinator::onAddShapeRequested()    { m_shapeController->addShape(); }
void MainWindowCoordinator::onDeleteShapeRequested() { m_shapeController->deleteShape(); }

void MainWindowCoordinator::onInventoryRequested()
{
    openInventory(m_view, Inventory::getInstance());
}

void MainWindowCoordinator::onCustomEditorRequested()
{
    openCustomEditor(m_view, m_model->language());
}

void MainWindowCoordinator::onFolderRequested()
{
    openFolder(m_view, m_model->language());
}

void MainWindowCoordinator::onTestGpioRequested()
{
    openTestGpio(m_view);
}

void MainWindowCoordinator::onBluetoothReceiverRequested()
{
    openBluetoothReceiver(m_view);
}

void MainWindowCoordinator::onWifiTransferRequested()
{
    openWifiTransfer(m_view);
}

void MainWindowCoordinator::onSaveLayoutRequested()
{
    handleSaveLayoutRequest(m_view,
                            m_view->getShapeVisualization(),
                            m_shapeController->selectedShapeType());
}

void MainWindowCoordinator::onGenerateAiRequested()
{
    AiGenerationRequest request;
    if (!openAiGenerationPrompt(m_view, request))
        return;

    m_view->showAiProgressBar();
    emit m_view->requestAiGeneration(request.prompt,
                                     request.model,
                                     request.quality,
                                     request.size,
                                     request.colorPrompt);
}

// =======================================================================
//  Logique métier — identique à avant
// =======================================================================

void MainWindowCoordinator::openWifiSettings(QWidget *parent)
{
    m_navigationController->openWifiSettings(parent);
}

void MainWindowCoordinator::openInventory(QWidget *mainWindow, QWidget *inventoryWindow)
{
    m_navigationController->showInventory(mainWindow, inventoryWindow);
}

void MainWindowCoordinator::openCustomEditor(QWidget *mainWindow, Language language)
{
    m_navigationController->openCustomEditor(mainWindow, language);
}

void MainWindowCoordinator::openFolder(QWidget *mainWindow, Language language)
{
    m_navigationController->openFolder(mainWindow, language);
}

void MainWindowCoordinator::openTestGpio(QWidget *mainWindow)
{
    m_navigationController->openTestGpio(mainWindow);
}

void MainWindowCoordinator::openBluetoothReceiver(QWidget *mainWindow)
{
    m_navigationController->openBluetoothReceiver(mainWindow);
}

void MainWindowCoordinator::openWifiTransfer(QWidget *mainWindow)
{
    m_navigationController->openWifiTransfer(mainWindow);
}

void MainWindowCoordinator::handleSaveLayoutRequest(QWidget *mainWindow,
                                                    ShapeVisualization *visualization,
                                                    ShapeModel::Type selectedType)
{
    m_navigationController->handleSaveLayoutRequest(mainWindow, visualization, selectedType);
}

bool MainWindowCoordinator::openAiGenerationPrompt(QWidget *parent, AiGenerationRequest &request)
{
    return m_aiServiceManager->openGenerationPrompt(parent, request);
}

void MainWindowCoordinator::onCustomShapeSelected(const QList<QPolygonF> &polygons,
                                                  const QString &name)
{
    if (!m_shapeController->loadCustomShapes(polygons, name,
                                             m_model->largeur(), m_model->longueur()))
        return;

    QList<LayoutData> layouts = Inventory::getInstance()->getLayoutsForShape(name);
    if (!layouts.isEmpty()) {
        m_navigationController->openLayoutsDialog(m_dialogParent, name,
                                                  layouts, polygons,
                                                  m_model->language());
        return;
    }

    emit requestShowFullScreen();
}

void MainWindowCoordinator::onShapeSelectedFromInventory(ShapeModel::Type type)
{
    m_shapeController->setSelectedShapeType(type);
    QList<LayoutData> layouts = Inventory::getInstance()->getLayoutsForBaseShape(type);
    if (!layouts.isEmpty()) {
        QList<QPolygonF> polys = ShapeModel::shapePolygons(type, 100, 100);
        QString name = BaseShapeNamingService::baseShapeName(type, m_model->language());
        m_navigationController->openLayoutsDialog(m_dialogParent, name,
                                                  layouts, polys,
                                                  m_model->language(), true, type);
        return;
    }

    m_shapeController->setPredefinedShape(type);
}

void MainWindowCoordinator::openImageInCustom(const QString &filePath,
                                              bool internalContours,
                                              bool colorEdges)
{
    QPainterPath outline = ImageImportService::processImageToPath(filePath,
                                                                  internalContours,
                                                                  colorEdges);
    if (outline.isEmpty())
        return;

    m_navigationController->openCustomEditorWithImportedPath(m_dialogParent,
                                                             m_model->language(),
                                                             outline);
}
