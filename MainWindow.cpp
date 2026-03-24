#include "MainWindow.h"
#include "ui_mainwindow.h"
#include "Inventory.h"
#include "ShapeVisualization.h"
#include "ImageImportService.h"
#include "AspectRatioWrapper.h"
#include "NavigationController.h"
#include "AIServiceManager.h"
#include "ShapeController.h"

#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QPainterPath>
#include <QDebug>
#include <QScreen>
#include <QGuiApplication>
#include <QShowEvent>
#include <QToolButton>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>


// --- Construction / Destruction ---

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_navigationController = new NavigationController(this);
    m_aiServiceManager = new AIServiceManager(this);
    m_aiServiceManager->setDialogParent(this);
    m_shapeController = new ShapeController(ui->shapeVisualizationWidget, this);
    setupUI();
    setupModels();
    setupConnections();
}

// --- UI Setup ---

void MainWindow::setupUI()
{
    setupWorkspaceLayout();
    setupMenus();
    applyStyleSheets();


    // place la fenêtre sur le 2ᵉ écran
    // ScreenUtils::placeOnSecondaryScreen(this);

    // Synchroniser l'état initial des sliders avec les spinboxes
    ui->Slider_longueur->setValue(ui->Longueur->value());
    ui->Slider_largeur->setValue(ui->Largeur->value());
    // activation et desactivation de l'optimisation

    ui->optimizePlacementButton->setCheckable(true);
    ui->optimizePlacementButton2->setCheckable(true);

    // Configuration de la barre de progression
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(0);
    ui->progressBar->setFormat("%p%");
    ui->progressBar->setAlignment(Qt::AlignCenter);
    ui->progressBar->setVisible(false);


    if (ui->timeRemainingLabel)
        ui->timeRemainingLabel->setText(tr("Temps restant estimé : 0s"));
}

void MainWindow::setupWorkspaceLayout()
{
    // Stretches de la colonne centrale
    ui->centerVBox->setStretch(0, 0);
    ui->centerVBox->setStretch(1, 1);
    ui->centerVBox->setStretch(2, 0);
    ui->centerVBox->setStretch(3, 0);

    // Stretches du layout principal
    ui->mainHorizontalLayout->setStretch(0, 0);
    ui->mainHorizontalLayout->setStretch(1, 1);

    auto *visualizationWidget = ui->shapeVisualizationWidget;
    visualizationWidget->setSheetSizeMm(QSizeF(600, 400));

    auto *wrapper = new AspectRatioWrapper(nullptr, 600.0 / 400.0, ui->centralwidget);
    wrapper->setObjectName("formeRatioWrapper");
    wrapper->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->horizontalLayout->replaceWidget(visualizationWidget, wrapper);
    wrapper->setChild(visualizationWidget);

    connect(visualizationWidget, &ShapeVisualization::sheetSizeMmChanged,
            wrapper, [wrapper](const QSizeF &mm) {
                if (mm.width() > 0.0 && mm.height() > 0.0)
                    wrapper->setAspect(mm.width() / mm.height());
            });
}

void MainWindow::setupMenus()
{
    languageMenu  = new QMenu(tr("Langue"), this);
    actionFrench  = languageMenu->addAction(tr("Français"));
    actionEnglish = languageMenu->addAction(tr("Anglais"));
    connect(actionFrench, &QAction::triggered, this, &MainWindow::setLanguageFrench);
    connect(actionEnglish, &QAction::triggered, this, &MainWindow::setLanguageEnglish);

    settingsMenu = new QMenu(tr("Paramètres"), this);
    actionWifiConfig = settingsMenu->addAction(tr("Configurer le Wi-Fi"));
    settingsMenu->addMenu(languageMenu);
    connect(actionWifiConfig, &QAction::triggered, this,
            [this]() { m_navigationController->openWifiSettings(this); });

    auto *settingsBtn = new QToolButton(this);
    settingsBtn->setObjectName("settingsToolButton");
    settingsBtn->setText(tr("Paramètres"));
    settingsBtn->setIcon(QIcon(":/icons/settings.svg"));
    settingsBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    settingsBtn->setPopupMode(QToolButton::InstantPopup);
    settingsBtn->setMenu(settingsMenu);
    menuBar()->setCornerWidget(settingsBtn, Qt::TopRightCorner);
}

void MainWindow::applyStyleSheets()
{
    if (auto *settingsBtn = menuBar()->cornerWidget(Qt::TopRightCorner)) {
        QFile styleFile(":/styles/style.qss");
        if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&styleFile);
            settingsBtn->setStyleSheet(stream.readAll());
        }
    }
}

void MainWindow::setupModels()
{
    // Initialiser la classe ShapeVisualization à partir du widget de l'UI
    shapeVisualization = qobject_cast<ShapeVisualization*>(ui->shapeVisualizationWidget);
}

void MainWindow::setupConnections()
{
    setupShapeConnections();
    setupNavigationConnections();
    setupSystemConnections();
    connect(m_aiServiceManager, &AIServiceManager::generationStatusChanged,
            ui->labelAIGenerationStatus, &QLabel::setText);
    connect(m_aiServiceManager, &AIServiceManager::imageReadyForImport, this,
            [this](const QString &path, bool internalContours, bool colorEdges) {
                ui->progressBarAI->setVisible(false);
                openImageInCustom(path, internalContours, colorEdges);
            });
}

void MainWindow::setupShapeConnections()
{
    // Connection de l'inventory à la forme
    QObject::connect(Inventory::getInstance(), &Inventory::shapeSelected,
                     this, &MainWindow::onShapeSelectedFromInventory);

    // Connecter le signal du nombre de formes placées pour mettre à jour le label
    connect(shapeVisualization, &ShapeVisualization::shapesPlacedCount,
            this, [this](int count) {
                ui->shapeCountLabel->setText(QString("Formes placées: %1").arg(count));
            });

    connect(shapeVisualization, &ShapeVisualization::actionRefused,
            this, [this](const QString &reason) {
                QMessageBox::warning(this, tr("Action refusée"), reason);
            });

    connect(shapeVisualization, &ShapeVisualization::progressUpdated,
            this, [this](int current, int total) {
                if (total <= 0) {
                    ui->progressBar->setRange(0, 100);
                    ui->progressBar->setValue(0);
                    ui->progressBar->setVisible(false);
                    return;
                }

                ui->progressBar->setVisible(true);
                ui->progressBar->setRange(0, total);
                ui->progressBar->setValue(current);
            });

    // Connecter les spinbox pour les dimensions
    connect(ui->Largeur, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this]() { m_shapeController->updateShape(ui->Largeur->value(), ui->Longueur->value()); });
    connect(ui->Longueur, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this]() { m_shapeController->updateShape(ui->Largeur->value(), ui->Longueur->value()); });

    // Connecter les boutons pour changer les modèles
    connect(ui->Cercle, &QPushButton::clicked, this, [this]() {
        m_shapeController->setPredefinedShape(ShapeModel::Type::Circle);
    });
    connect(ui->Rectangle, &QPushButton::clicked, this, [this]() {
        m_shapeController->setPredefinedShape(ShapeModel::Type::Rectangle);
    });
    connect(ui->Triangle, &QPushButton::clicked, this, [this]() {
        m_shapeController->setPredefinedShape(ShapeModel::Type::Triangle);
    });
    connect(ui->Etoile, &QPushButton::clicked, this, [this]() {
        m_shapeController->setPredefinedShape(ShapeModel::Type::Star);
    });
    connect(ui->Coeur, &QPushButton::clicked, this, [this]() {
        m_shapeController->setPredefinedShape(ShapeModel::Type::Heart);
    });

    // connexion pour les formes de l'inventory
    connect(Inventory::getInstance(), &Inventory::customShapeSelected,
            this,                       &MainWindow::onCustomShapeSelected);
}

void MainWindow::setupNavigationConnections()
{
    connect(m_navigationController, &NavigationController::customShapeApplied,
            this, [this](const QList<QPolygonF> &shapes) {
                if (shapeVisualization) {
                    shapeVisualization->displayCustomShapes(shapes);
                    shapeVisualization->setCurrentCustomShapeName("");
                }
                showFullScreen();
            });
    connect(m_navigationController, &NavigationController::layoutSelected, this,
            [this](const LayoutData &layout) {
                shapeVisualization->applyLayout(layout);
                applySelectedLayoutToControls(layout);
            });
    connect(m_navigationController, &NavigationController::baseShapeLayoutSelected, this,
            [this](ShapeModel::Type type, const LayoutData &layout) {
                m_shapeController->setSelectedShapeType(type);
                QList<QPolygonF> shapePolys = ShapeModel::shapePolygons(type, 100, 100);
                shapeVisualization->setCustomMode();
                shapeVisualization->displayCustomShapes(shapePolys);
                shapeVisualization->setCurrentCustomShapeName(Inventory::baseShapeName(type, Language::French));
                shapeVisualization->applyLayout(layout);
                applySelectedLayoutToControls(layout);
            });
    connect(m_navigationController, &NavigationController::baseShapeOnlySelected, this,
            [this](ShapeModel::Type type) {
                m_shapeController->setPredefinedShape(type);
            });
    connect(m_navigationController, &NavigationController::requestReturnToFullScreen, this,
            [this]() {
                showFullScreen();
            });
    connect(m_navigationController, &NavigationController::requestOpenInventory, this,
            []() {
                Inventory::getInstance()->showFullScreen();
            });

    // Naviguer entre les pages
    connect(ui->buttonInventory, &QPushButton::clicked, this,
            [this]() { m_navigationController->showInventory(this, Inventory::getInstance()); });
    connect(ui->buttonCustom, &QPushButton::clicked, this,
            [this]() { m_navigationController->openCustomEditor(this, m_displayLanguage); });
    // "Images générées" should open the gallery of generated images
    connect(ui->buttonTestGpio, &QPushButton::clicked, this,
            [this]() { m_navigationController->openFolder(this, m_displayLanguage); });

    connect(ui->buttonGenerateAI, &QPushButton::clicked, this, [this]() {
        AiGenerationRequest request;
        if (!m_aiServiceManager->openGenerationPrompt(this, request))
            return;

        ui->progressBarAI->setVisible(true);
        ui->progressBarAI->setMinimum(0);
        ui->progressBarAI->setMaximum(0);
        emit requestAiGeneration(request.prompt,
                                 request.model,
                                 request.quality,
                                 request.size,
                                 request.colorPrompt);
    });
    // Small button with the download icon opens the GPIO test page
    connect(ui->buttonViewGeneratedImages, &QPushButton::clicked, this,
            [this]() { m_navigationController->openTestGpio(this); });
    connect(ui->buttonFileReceiver, &QPushButton::clicked, this,
            [this]() { m_navigationController->openBluetoothReceiver(this); });
    connect(ui->buttonWifiTransfer, &QPushButton::clicked, this,
            [this]() { m_navigationController->openWifiTransfer(this); });
}

void MainWindow::setupSystemConnections()
{
    // Connecter les spinboxes aux sliders
    connect(ui->Longueur, &QSpinBox::valueChanged, ui->Slider_longueur, &QSlider::setValue);
    connect(ui->Slider_longueur, &QSlider::valueChanged, ui->Longueur, &QSpinBox::setValue);
    connect(ui->Largeur, &QSpinBox::valueChanged, ui->Slider_largeur, &QSlider::setValue);
    connect(ui->Slider_largeur, &QSlider::valueChanged, ui->Largeur, &QSpinBox::setValue);

    // Connection entre spinbox nombre de formes et ShapeController
    connect(ui->shapeCountSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int count) {
                m_shapeController->updateShapeCount(count,
                                                    ui->Largeur->value(),
                                                    ui->Longueur->value());
            });

    connect(ui->optimizePlacementButton, &QPushButton::clicked, this,
            [this](bool checked) {
                ui->optimizePlacementButton2->setChecked(false);
                m_shapeController->onOptimizePlacementClicked(checked,
                                                              ui->Largeur->value(),
                                                              ui->Longueur->value());
            });

    connect(ui->optimizePlacementButton2, &QPushButton::clicked, this,
            [this](bool checked) {
                ui->optimizePlacementButton->setChecked(false);
                m_shapeController->onOptimizePlacement2Clicked(checked,
                                                               ui->Largeur->value(),
                                                               ui->Longueur->value());
            });

    // spinBox pour l'espacement
    connect(ui->spaceSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            m_shapeController, &ShapeController::updateSpacing);

    connect(shapeVisualization, &ShapeVisualization::spacingChanged,
            this, [this](int newSpacing) {
                ui->spaceSpinBox->setValue(newSpacing);
            });

    // Déplacement : 1 pixel par clic
    connect(ui->ButtonUp, &QPushButton::clicked, m_shapeController, &ShapeController::onMoveUpClicked);
    connect(ui->ButtonDown, &QPushButton::clicked, m_shapeController, &ShapeController::onMoveDownClicked);
    connect(ui->ButtonLeft, &QPushButton::clicked, m_shapeController, &ShapeController::onMoveLeftClicked);
    connect(ui->ButtonRight, &QPushButton::clicked, m_shapeController, &ShapeController::onMoveRightClicked);

    connect(ui->ButtonRotationLeft, &QPushButton::clicked,
            m_shapeController, &ShapeController::rotateLeft);
    connect(ui->ButtonRotationRight, &QPushButton::clicked,
            m_shapeController, &ShapeController::rotateRight);
    connect(ui->ButtonAddShape, &QPushButton::clicked,
            m_shapeController, &ShapeController::addShape);
    connect(ui->ButtonDeleteShape, &QPushButton::clicked,
            m_shapeController, &ShapeController::deleteShape);

    connect(ui->ButtonSaveLayout, &QPushButton::clicked, this, [this]() {
        m_navigationController->handleSaveLayoutRequest(this,
                                                        shapeVisualization,
                                                        m_shapeController->selectedShapeType());
    });

    connect(ui->Play, &QPushButton::clicked, this, [this]() { emit requestStartCut(); });
    connect(ui->Pause, &QPushButton::clicked, this, [this]() { emit requestPauseCut(); });
    connect(ui->Stop, &QPushButton::clicked, this, [this]() { emit requestStopCut(); });

    connect(shapeVisualization, &ShapeVisualization::optimizationStateChanged, this,
            [this](bool optimized) {
                if (!optimized) {
                    ui->optimizePlacementButton->setChecked(false);
                    ui->optimizePlacementButton2->setChecked(false);
                }
            });
}

MainWindow::~MainWindow() {
    delete ui;
}

ShapeVisualization* MainWindow::getShapeVisualization() const
{
    return shapeVisualization;
}

// --- Public API ---

void MainWindow::openImageInCustom(const QString &filePath,
                                   bool internalContours,
                                   bool colorEdges)
{
    QPainterPath outline = ImageImportService::processImageToPath(filePath,
                                                                  internalContours,
                                                                  colorEdges);
    if (outline.isEmpty())
        return;
    m_navigationController->openCustomEditorWithImportedPath(this, m_displayLanguage, outline);
}

// --- Event Handlers ---

void MainWindow::onCustomShapeSelected(const QList<QPolygonF> &polygons,
                                       const QString &name)
{
    if (!m_shapeController->loadCustomShapes(polygons,
                                             name,
                                             ui->Largeur->value(),
                                             ui->Longueur->value())) {
        return;
    }

    /* 4) Si des dispositions existent, ouvrir la fenêtre LayoutsDialog */
    QList<LayoutData> layouts = Inventory::getInstance()->getLayoutsForShape(name);
    if (!layouts.isEmpty()) {
        m_navigationController->openLayoutsDialog(this,
                                                  name,
                                                  layouts,
                                                  polygons,
                                                  m_displayLanguage);
        return;
    }

    /* 5) Pas de dispositions : on se contente d’afficher la forme ----- */
    this->showFullScreen();
}

void MainWindow::updateProgressBar(int percentage, const QString &remainingTimeText) {
    ui->progressBar->setVisible(true);
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(percentage);
    if (ui->timeRemainingLabel) {
        ui->timeRemainingLabel->setText(remainingTimeText);
    }
}

// --- Qt Events ---

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);       // Traduction automatique des widgets de l'UI
        retranslateDynamicUi();        // Traduction manuelle de ce que tu as créé en C++
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);


    const auto screens = QGuiApplication::screens();
    for (int i = 0; i < screens.size(); ++i) {
        QScreen *s = screens.at(i);
        //qDebug() << "Écran" << i
        //         << "nom =" << s->name()
        //         << "géométrie =" << s->geometry()
        //         << "disponible =" << s->availableGeometry();
    }
    /*if (screens.size() > 1) {
        QScreen* second = screens.at(0);
        // Attribuer le QWindow natif à l'écran secondaire
        if (auto win = this->windowHandle()) {
            win->setScreen(second);
        }
        // Passer en plein écran
        this->showFullScreen();
    }*/
    this->showFullScreen();

}

void MainWindow::setLanguageFrench()
{
    emit requestLanguageChange(Language::French);
}

void MainWindow::setLanguageEnglish()
{
    emit requestLanguageChange(Language::English);
}

void MainWindow::retranslateDynamicUi()
{
    if (settingsMenu) settingsMenu->setTitle(tr("Paramètres"));
    if (languageMenu) languageMenu->setTitle(tr("Langue"));
    if (actionFrench)  actionFrench ->setText(tr("Français"));
    if (actionEnglish) actionEnglish->setText(tr("Anglais"));
    if (actionWifiConfig) actionWifiConfig->setText(tr("Configurer le Wi-Fi"));

    if (ui->shapeCountLabel) {
        // Tu peux sauvegarder l'ancien nombre s'il est dynamique :
        int count = ui->shapeCountSpinBox->value();
        ui->shapeCountLabel->setText(tr("Formes placées: %1").arg(count));
    }
    if (ui->timeRemainingLabel) {
        ui->timeRemainingLabel->setText(tr("Temps restant estim\u00e9 : 0s"));
    }
}

void MainWindow::onShapeSelectedFromInventory(ShapeModel::Type type)
{
    m_shapeController->setSelectedShapeType(type);
    QList<LayoutData> layouts = Inventory::getInstance()->getLayoutsForBaseShape(type);
    if (!layouts.isEmpty()) {
        QList<QPolygonF> polys = ShapeModel::shapePolygons(type, 100, 100);
        QString name = Inventory::baseShapeName(type, m_displayLanguage);
        m_navigationController->openLayoutsDialog(this,
                                                  name,
                                                  layouts,
                                                  polys,
                                                  m_displayLanguage,
                                                  true,
                                                  type);

        return;
    }

    m_shapeController->setPredefinedShape(type);
}

void MainWindow::setSpinboxSliderEnabled(bool enabled)
{
    ui->Largeur->setEnabled(enabled);
    ui->Longueur->setEnabled(enabled);
    ui->Slider_largeur->setEnabled(enabled);
    ui->Slider_longueur->setEnabled(enabled);
    ui->shapeCountSpinBox->setEnabled(enabled);
    ui->spaceSpinBox->setEnabled(enabled);
    qDebug() << "[DEBUG] Appel de setSpinboxSliderEnabled(" << enabled << ")";
}

void MainWindow::onCutFinished(bool /*success*/)
{
    ui->progressBar->setValue(0);
    ui->progressBar->setVisible(false);
}

void MainWindow::onLanguageApplied(Language lang, bool ok)
{
    m_displayLanguage = lang;
    if (!ok) {
        QMessageBox::warning(this, tr("Langue"), tr("Impossible de charger la langue demandée."));
    }
    retranslateDynamicUi();
}

void MainWindow::applySelectedLayoutToControls(const LayoutData &layout)
{
    ui->Largeur->blockSignals(true);
    ui->Longueur->blockSignals(true);
    ui->shapeCountSpinBox->blockSignals(true);
    ui->spaceSpinBox->blockSignals(true);
    ui->Slider_largeur->blockSignals(true);
    ui->Slider_longueur->blockSignals(true);

    ui->Largeur->setValue(layout.largeur);
    ui->Longueur->setValue(layout.longueur);
    ui->Slider_largeur->setValue(layout.largeur);
    ui->Slider_longueur->setValue(layout.longueur);
    ui->shapeCountSpinBox->setValue(layout.items.size());
    ui->spaceSpinBox->setValue(layout.spacing);

    ui->Largeur->blockSignals(false);
    ui->Longueur->blockSignals(false);
    ui->shapeCountSpinBox->blockSignals(false);
    ui->spaceSpinBox->blockSignals(false);
    ui->Slider_largeur->blockSignals(false);
    ui->Slider_longueur->blockSignals(false);
}
