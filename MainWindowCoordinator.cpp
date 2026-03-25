#include "MainWindowCoordinator.h"

#include "NavigationController.h"
#include "ShapeController.h"
#include "ShapeVisualization.h"

MainWindowCoordinator::MainWindowCoordinator(NavigationController *navigationController,
                                             AIServiceManager *aiServiceManager,
                                             ShapeController *shapeController,
                                             QObject *parent)
    : QObject(parent)
    , m_navigationController(navigationController)
    , m_aiServiceManager(aiServiceManager)
    , m_shapeController(shapeController)
{
    QObject::connect(m_aiServiceManager, &AIServiceManager::generationStatusChanged,
                     this, &MainWindowCoordinator::generationStatusChanged);
    QObject::connect(m_aiServiceManager, &AIServiceManager::imageReadyForImport,
                     this, &MainWindowCoordinator::imageReadyForImport);
}

void MainWindowCoordinator::setDialogParent(QWidget *parent)
{
    m_aiServiceManager->setDialogParent(parent);
}

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

void MainWindowCoordinator::bindTo(Ui::MainWindow *ui,
                                   QWidget *mainWindow,
                                   const std::function<Language()> &languageProvider)
{
    // ---- Anciennement MainWindowNavigationBinder ----
    QObject::connect(ui->buttonInventory, &QPushButton::clicked, mainWindow,
                     [this, mainWindow]() {
                         openInventory(mainWindow, Inventory::getInstance());
                     });

    QObject::connect(ui->buttonCustom, &QPushButton::clicked, mainWindow,
                     [this, mainWindow, languageProvider]() {
                         openCustomEditor(mainWindow, languageProvider());
                     });

    QObject::connect(ui->buttonTestGpio, &QPushButton::clicked, mainWindow,
                     [this, mainWindow, languageProvider]() {
                         openFolder(mainWindow, languageProvider());
                     });

    QObject::connect(ui->buttonViewGeneratedImages, &QPushButton::clicked, mainWindow,
                     [this, mainWindow]() { openTestGpio(mainWindow); });

    QObject::connect(ui->buttonFileReceiver, &QPushButton::clicked, mainWindow,
                     [this, mainWindow]() { openBluetoothReceiver(mainWindow); });

    QObject::connect(ui->buttonWifiTransfer, &QPushButton::clicked, mainWindow,
                     [this, mainWindow]() { openWifiTransfer(mainWindow); });

    // ---- Anciennement MainWindowSystemBinder ----
    QObject::connect(ui->Longueur, &QSpinBox::valueChanged,
                     ui->Slider_longueur, &QSlider::setValue);
    QObject::connect(ui->Slider_longueur, &QSlider::valueChanged,
                     ui->Longueur, &QSpinBox::setValue);
    QObject::connect(ui->Largeur, &QSpinBox::valueChanged,
                     ui->Slider_largeur, &QSlider::setValue);
    QObject::connect(ui->Slider_largeur, &QSlider::valueChanged,
                     ui->Largeur, &QSpinBox::setValue);

    QObject::connect(ui->shapeCountSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), mainWindow,
                     [this, ui](int count) {
                         m_shapeController->updateShapeCount(count,
                                                             ui->Largeur->value(),
                                                             ui->Longueur->value());
                     });

    QObject::connect(ui->optimizePlacementButton, &QPushButton::clicked, mainWindow,
                     [this, ui](bool checked) {
                         ui->optimizePlacementButton2->setChecked(false);
                         m_shapeController->onOptimizePlacementClicked(checked,
                                                                       ui->Largeur->value(),
                                                                       ui->Longueur->value());
                     });

    QObject::connect(ui->optimizePlacementButton2, &QPushButton::clicked, mainWindow,
                     [this, ui](bool checked) {
                         ui->optimizePlacementButton->setChecked(false);
                         m_shapeController->onOptimizePlacement2Clicked(checked,
                                                                        ui->Largeur->value(),
                                                                        ui->Longueur->value());
                     });

    QObject::connect(ui->spaceSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                     m_shapeController, &ShapeController::updateSpacing);

    QObject::connect(ui->ButtonUp,    &QPushButton::clicked, m_shapeController, &ShapeController::onMoveUpClicked);
    QObject::connect(ui->ButtonDown,  &QPushButton::clicked, m_shapeController, &ShapeController::onMoveDownClicked);
    QObject::connect(ui->ButtonLeft,  &QPushButton::clicked, m_shapeController, &ShapeController::onMoveLeftClicked);
    QObject::connect(ui->ButtonRight, &QPushButton::clicked, m_shapeController, &ShapeController::onMoveRightClicked);

    QObject::connect(ui->ButtonRotationLeft,  &QPushButton::clicked, m_shapeController, &ShapeController::rotateLeft);
    QObject::connect(ui->ButtonRotationRight, &QPushButton::clicked, m_shapeController, &ShapeController::rotateRight);
    QObject::connect(ui->ButtonAddShape,      &QPushButton::clicked, m_shapeController, &ShapeController::addShape);
    QObject::connect(ui->ButtonDeleteShape,   &QPushButton::clicked, m_shapeController, &ShapeController::deleteShape);
}

void MainWindowCoordinator::onCustomShapeSelected(const QList<QPolygonF> &polygons,
                                                  const QString &name)
{
    if (!m_shapeController->loadCustomShapes(polygons, name,
                                             m_lastLargeur, m_lastLongueur))
        return;

    QList<LayoutData> layouts = Inventory::getInstance()->getLayoutsForShape(name);
    if (!layouts.isEmpty()) {
        m_navigationController->openLayoutsDialog(m_dialogParent, name,
                                                  layouts, polygons,
                                                  m_language);
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
        QString name = BaseShapeNamingService::baseShapeName(type, m_language);
        m_navigationController->openLayoutsDialog(m_dialogParent, name,
                                                  layouts, polys,
                                                  m_language, true, type);
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
                                                             m_language,
                                                             outline);
}

void MainWindowCoordinator::onDimensionsChanged(int largeur, int longueur)
{
    m_lastLargeur = largeur;
    m_lastLongueur = longueur;
}

void MainWindowCoordinator::onLanguageChanged(Language lang)
{
    m_language = lang;
}
