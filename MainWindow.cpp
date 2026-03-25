#include "MainWindow.h"
#include "ui_mainwindow.h"
#include "Inventory.h"
#include "ShapeVisualization.h"
#include "ImageImportService.h"
#include "AspectRatioWrapper.h"
#include "NavigationController.h"
#include "AIServiceManager.h"
#include "ShapeController.h"
#include "MainWindowMenuBuilder.h"
#include "MainWindowCoordinator.h"
#include "BaseShapeNamingService.h"

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

MainWindow::MainWindow(QWidget *parent, MainWindowCoordinator *coordinator)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    if (coordinator) {
        m_coordinator = coordinator;
        m_ownsCoordinator = false;
    } else {
        auto *navigation = new NavigationController(this);
        auto *ai = new AIServiceManager(this);
        auto *shape = new ShapeController(ui->shapeVisualizationWidget, this);
        m_coordinator = new MainWindowCoordinator(navigation, ai, shape, this);
        m_ownsCoordinator = true;
    }

    m_navigationController = m_coordinator->navigationController();
    m_aiServiceManager = m_coordinator->aiServiceManager();
    m_shapeController = m_coordinator->shapeController();
    m_coordinator->setDialogParent(this);
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
    const auto handles = MainWindowMenuBuilder::build(
        menuBar(),
        this,
        tr("Paramètres"),
        tr("Langue"),
        tr("Français"),
        tr("Anglais"),
        tr("Configurer le Wi-Fi"),
        [this]() { setLanguageFrench(); },
        [this]() { setLanguageEnglish(); },
        [this]() { m_coordinator->openWifiSettings(this); });

    settingsMenu = handles.settingsMenu;
    languageMenu = handles.languageMenu;
    actionFrench = handles.actionFrench;
    actionEnglish = handles.actionEnglish;
    actionWifiConfig = handles.actionWifiConfig;
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
    m_coordinator->bindTo(ui, this, [this]() { return m_displayLanguage; });
    setupShapeConnections();
    setupNavigationConnections();
    setupSystemConnections();
    connect(m_coordinator, &MainWindowCoordinator::generationStatusChanged,
            ui->labelAIGenerationStatus, &QLabel::setText);
    connect(m_coordinator, &MainWindowCoordinator::imageReadyForImport, this,
            [this](const QString &path, bool internalContours, bool colorEdges) {
                ui->progressBarAI->setVisible(false);
                m_coordinator->openImageInCustom(path, internalContours, colorEdges); // ← appel sur Coordinator
            });
}

void MainWindow::setupShapeConnections()
{
    // Connection de l'inventory à la forme
    QObject::connect(Inventory::getInstance(), &Inventory::shapeSelected,
                     m_coordinator, &MainWindowCoordinator::onShapeSelectedFromInventory);

    // Connecter le signal du nombre de formes placées pour mettre à jour le label
    connect(shapeVisualization, &ShapeVisualization::shapesPlacedCount,
            this, [this](int count) {
                ui->shapeCountLabel->setText(QString("Formes placées: %1").arg(count));
            });

    connect(shapeVisualization, &ShapeVisualization::actionRefused,
            this, [this](const QString &reason) {
                QMessageBox::warning(this, tr("Action refusée"), reason);
            });

    connect(m_shapeController, &ShapeController::progressUpdated,
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
    connect(ui->Largeur, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this]() { m_coordinator->onDimensionsChanged(ui->Largeur->value(), ui->Longueur->value()); });
    connect(ui->Longueur, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this]() { m_coordinator->onDimensionsChanged(ui->Largeur->value(), ui->Longueur->value()); });

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
            m_coordinator, &MainWindowCoordinator::onCustomShapeSelected);
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
                shapeVisualization->setCurrentCustomShapeName(BaseShapeNamingService::baseShapeName(type, Language::French));
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



    connect(ui->buttonGenerateAI, &QPushButton::clicked, this, [this]() {
        AiGenerationRequest request;
        if (!m_coordinator->openAiGenerationPrompt(this, request))
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
}

void MainWindow::setupSystemConnections()
{


    connect(ui->ButtonSaveLayout, &QPushButton::clicked, this, [this]() {
        m_coordinator->handleSaveLayoutRequest(this,
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
    if (!m_ownsCoordinator)
        m_coordinator = nullptr;
    delete ui;
}

ShapeVisualization* MainWindow::getShapeVisualization() const
{
    return shapeVisualization;
}

// --- Event Handlers ---
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
