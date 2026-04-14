#include "MainWindowCoordinator.h"
#include "MainWindow.h"
#include "DialogManager.h"
#include "ShapeCoordinator.h"
#include "ShapeVisualization.h"
#include "WorkspaceViewModel.h"
#include "Inventory.h"
#include "InventoryViewModel.h"
#include "BaseShapeNamingService.h"
#include "ImageImportService.h"
#include "OpenAIService.h"
#include "StmTestDialog.h"

#include <QApplication>

MainWindowCoordinator::MainWindowCoordinator(DialogManager *navigationController,
                                             AIDialogCoordinator *aiServiceManager,
                                             ShapeCoordinator *shapeController,
                                             WorkspaceViewModel *model,
                                             QObject *parent)
    : QObject(parent)
    , m_navigationController(navigationController)
    , m_aiServiceManager(aiServiceManager)
    , m_shapeController(shapeController)
    , m_model(model)
    , m_cuttingService(new CuttingService(this))
    , m_machineViewModel(new MachineViewModel(this))
    , m_aiService(nullptr)
{
    // Signaux internes AI → Coordinator
    connect(m_aiServiceManager, &AIDialogCoordinator::generationStatusChanged,
            this, &MainWindowCoordinator::generationStatusChanged);
    connect(m_aiServiceManager, &AIDialogCoordinator::imageReadyForImport,
            this, &MainWindowCoordinator::imageReadyForImport);

    // CuttingService → ViewModel (connexion différée car m_viewModel est null ici)
    // La connexion est établie dans setViewModel() dès que le ViewModel est disponible.
}

void MainWindowCoordinator::setViewModel(MainWindowViewModel *viewModel)
{
    m_viewModel = viewModel;
    // CuttingService → ViewModel
    connect(m_cuttingService, &CuttingService::progressUpdated,
            m_viewModel,      &MainWindowViewModel::setCutProgress);
    connect(m_cuttingService, &CuttingService::finished,
            m_viewModel,      &MainWindowViewModel::setCutFinished);
    connect(m_cuttingService, &CuttingService::controlsEnabledChanged,
            m_viewModel,      &MainWindowViewModel::setControlsEnabled);
    connect(m_cuttingService, &CuttingService::statusMessage,
            this,             &MainWindowCoordinator::aiGenerationStatus);
    // ShapeCoordinator progress → ViewModel
    connect(m_shapeController, &ShapeCoordinator::progressUpdated,
            m_viewModel,       &MainWindowViewModel::setShapeProgress);
    // AI status → ViewModel
    connect(this, &MainWindowCoordinator::aiGenerationStatus,
            m_viewModel, &MainWindowViewModel::setAiStatus);
    // AI image ready → ViewModel
    connect(this, &MainWindowCoordinator::imageReadyForImport, m_viewModel,
            [this]() { m_viewModel->setAiImageReceived(); });
    // DialogManager::imageReuseRequested → openImageInCustom
    connect(m_navigationController, &DialogManager::imageReuseRequested,
            this, &MainWindowCoordinator::openImageInCustom);
}

void MainWindowCoordinator::setInventory(Inventory *inventory)
{
    m_inventory = inventory;
    if (!m_inventory)
        return;
    // La sélection de forme prédéfinie passe par le ViewModel (plus par la View directement)
    connect(m_inventory->viewModel(), &InventoryViewModel::baseShapeSelected,
            this, &MainWindowCoordinator::onShapeSelectedFromInventory);
    connect(m_inventory, &Inventory::customShapeSelected,
            this, &MainWindowCoordinator::onCustomShapeSelected);
}

void MainWindowCoordinator::setMainWindow(MainWindow *window) {
    m_view = window;
}

void MainWindowCoordinator::setShapeVisualization(ShapeVisualization *visualization) {
    m_shapeVisualization = visualization;
    m_cuttingService->initialize(m_shapeVisualization, m_view);
    m_cuttingService->setMachineViewModel(m_machineViewModel);
    ensureServicesInitialized();
}

bool MainWindowCoordinator::ensureServicesInitialized() {
    if (!m_shapeVisualization || !m_view) {
        return false;
    }

    if (!m_aiService) {
        m_aiService = new OpenAIService(this);
        connect(m_aiService, &OpenAIService::statusUpdate,
                this, &MainWindowCoordinator::aiGenerationStatus);
        connect(m_aiService, &OpenAIService::generationFinished,
                this, [this](bool success, const QString &result) {
                    if (!success) {
                        emit aiGenerationStatus(result);
                        return;
                    }
                    emit aiImageReady(result);
                });
    }

    return true;
}

void MainWindowCoordinator::startCutting()  { m_cuttingService->startCutting();  }
void MainWindowCoordinator::stopCutting()   { m_cuttingService->stopCutting();   }
void MainWindowCoordinator::pauseCutting()  { m_cuttingService->pauseCutting();  }
void MainWindowCoordinator::setCuttingSpeed(int s) { m_cuttingService->setCuttingSpeed(s); }

void MainWindowCoordinator::onStmTestRequested()
{
    auto *dlg = new StmTestDialog(m_machineViewModel, m_view);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

void MainWindowCoordinator::requestAiGeneration(const QString &prompt,
                                                const QString &model,
                                                const QString &quality,
                                                const QString &size,
                                                bool colorPrompt) {
    if (!ensureServicesInitialized()) return;
    m_aiService->requestImageGeneration(prompt, model, quality, size, colorPrompt);
}

bool MainWindowCoordinator::loadLanguage(Language lang) {
    qApp->removeTranslator(&m_translator);
    m_currentLanguage = lang;

    const QString locale = (lang == Language::French) ? "fr" : "en";
    const QString path = qApp->applicationDirPath()
                         + "/translations/machineDecoupeIHM_" + locale + ".qm";

    const bool ok = m_translator.load(path);
    if (ok) {
        qApp->installTranslator(&m_translator);
    }
    return ok;
}

void MainWindowCoordinator::changeLanguage(Language lang) {
    const bool ok = loadLanguage(lang);
    m_model->setLanguage(lang);
    if (m_viewModel) m_viewModel->setLanguage(lang, ok);
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

    // --- DialogManager → View (layout, shapes, fullscreen) ---
    connect(m_navigationController, &DialogManager::customShapeApplied,
            view, [view](const QList<QPolygonF> &shapes) {
                view->displayCustomShapes(shapes, QString());
            });

    connect(m_navigationController, &DialogManager::layoutSelected,
            view, &MainWindow::applyLayout);

    // Le Coordinator calcule les polygones et le nom (domain) avant de les passer à la View.
    // La View ne connaît plus ShapeModel::Type.
    connect(m_navigationController, &DialogManager::baseShapeLayoutSelected,
            view, [this, view](ShapeModel::Type type, const LayoutData &layout) {
                const QList<QPolygonF> polys = ShapeModel::shapePolygons(type, 100, 100);
                const QString name = BaseShapeNamingService::baseShapeName(type, m_model->language());
                view->displayBaseShapeLayout(polys, name, layout);
            });

    connect(m_navigationController, &DialogManager::baseShapeOnlySelected, this,
            [this](ShapeModel::Type type) {
                m_shapeController->setPredefinedShape(type);
            });

    connect(m_navigationController, &DialogManager::requestReturnToFullScreen, view,
            [view]() { view->showFullScreen(); });

    connect(m_navigationController, &DialogManager::requestOpenInventory, this,
            [this]() { if (m_inventory) m_inventory->showFullScreen(); });

    connect(m_inventory, &Inventory::navigationBackRequested, view,
            [view]() { view->showFullScreen(); });

    // --- imageReadyForImport → ouvre dans l'éditeur custom ---
    connect(this, &MainWindowCoordinator::imageReadyForImport, this,
            [this](const QString &path, bool internalContours, bool colorEdges) {
                openImageInCustom(path, internalContours, colorEdges);
            });

    // --- Découpe (View → Coordinator) ---
    connect(view, &MainWindow::requestStartCut,    this, &MainWindowCoordinator::startCutting);
    connect(view, &MainWindow::requestPauseCut,    this, &MainWindowCoordinator::pauseCutting);
    connect(view, &MainWindow::requestStopCut,     this, &MainWindowCoordinator::stopCutting);
    connect(view, &MainWindow::requestSpeedChange, this, &MainWindowCoordinator::setCuttingSpeed);

    // --- Test Moteurs STM ---
    connect(view, &MainWindow::stmTestRequested,   this, &MainWindowCoordinator::onStmTestRequested);

    // --- AI (View → Coordinator) ---
    connect(view, &MainWindow::requestAiGeneration, this, &MainWindowCoordinator::requestAiGeneration);

    // --- Langue (View → Coordinator) ---
    connect(view, &MainWindow::requestLanguageChange, this, &MainWindowCoordinator::changeLanguage);

    // --- Feedback AI → AIDialogCoordinator ---
    connect(this, &MainWindowCoordinator::aiGenerationStatus,
            m_aiServiceManager, &AIDialogCoordinator::onGenerationStatus);
    connect(this, &MainWindowCoordinator::aiImageReady,
            m_aiServiceManager, &AIDialogCoordinator::onAiImageReady);
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
    openInventory(m_view, m_inventory);
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

    QList<LayoutData> layouts = m_inventory->viewModel()->getLayoutsForShape(name);
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
    QList<LayoutData> layouts = m_inventory->viewModel()->getLayoutsForBaseShape(type);
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

// Dans MainWindowCoordinator.cpp
void MainWindowCoordinator::openImageInCustom(const QString &filePath, bool internalContours, bool colorEdges)
{
    QPainterPath outline = ImageImportService::processImageToPath(filePath, internalContours, colorEdges);
    if (outline.isEmpty())
        return;

    m_navigationController->openCustomEditorWithImportedPath(m_dialogParent, m_model->language(), outline);
}
