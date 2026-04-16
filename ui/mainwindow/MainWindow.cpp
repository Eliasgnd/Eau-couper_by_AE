#include "MainWindow.h"
#include "ui_mainwindow.h"
#include "Inventory.h"
#include "ShapeVisualization.h"
#include "ImageImportService.h"
#include "AspectRatioWrapper.h"
#include "DialogManager.h"
#include "AIDialogCoordinator.h"
#include "ShapeCoordinator.h"
#include "MainWindowMenuBuilder.h"
#include "MainWindowCoordinator.h"
#include "BaseShapeNamingService.h"
#include "WorkspaceViewModel.h"
#include "MainWindowViewModel.h"
#include "ShapeVisualizationViewModel.h"

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

MainWindow::MainWindow(QWidget *parent,
                       MainWindowCoordinator *coordinator,
                       WorkspaceViewModel *model,
                       Inventory *inventory)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // --- Modèle ---
    if (model) {
        m_model = model;
    } else {
        m_model = new WorkspaceViewModel(this);
    }

    // --- Inventory (injecté depuis l'extérieur via AppFactory) ---
    m_inventory = inventory;

    // --- Coordinator ---
    if (coordinator) {
        m_coordinator = coordinator;
        m_ownsCoordinator = false;
    } else {
        auto *navigation = new DialogManager(m_inventory->viewModel(), this);
        auto *ai         = new AIDialogCoordinator(this);
        auto *shape      = new ShapeCoordinator(ui->shapeVisualizationWidget, this);
        m_coordinator    = new MainWindowCoordinator(navigation, ai, shape, m_model, this);
        m_ownsCoordinator = true;
    }

    m_coordinator->setDialogParent(this);
    m_coordinator->setInventory(m_inventory);

    // Créer et injecter le ViewModel — il sert de pont entre Coordinator et View
    m_viewModel = new MainWindowViewModel(this);
    m_coordinator->setViewModel(m_viewModel);

    setupUI();
    setupModels();
    setupViewConnections();

    m_coordinator->setMainWindow(this);
    m_coordinator->setShapeVisualization(shapeVisualization);

    // Le Coordinator connecte ses propres réactions aux signaux de la View
    m_coordinator->connectToView(this);

    // Fix Cause A : SpinBox_vitesse::valueChanged(10) a été émis pendant setupUi()
    // AVANT que setupViewConnections() établisse la connexion → valeur initiale jamais
    // propagée vers CuttingService. On la force ici, après toutes les connexions.
    emit requestSpeedChange(ui->SpinBox_vitesse->value());
}

// --- UI Setup ---

void MainWindow::setupUI()
{
    setupWorkspaceLayout();
    setupMenus();
    applyStyleSheets();

    ui->Slider_longueur->setValue(ui->Longueur->value());
    ui->Slider_largeur->setValue(ui->Largeur->value());

    ui->optimizePlacementButton->setCheckable(true);
    ui->optimizePlacementButton2->setCheckable(true);

    // Configuration de la barre de progression
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(0);
    ui->progressBar->setFormat("%p%");
    ui->progressBar->setAlignment(Qt::AlignCenter);

    // --- MODIFICATIONS ICI ---
    // 1. On force la barre de progression à garder sa hauteur même invisible
    QSizePolicy spProgress = ui->progressBar->sizePolicy();
    spProgress.setRetainSizeWhenHidden(true);
    ui->progressBar->setSizePolicy(spProgress);
    ui->progressBar->setVisible(false);

    // Dans MainWindow::setupUI()
    if (ui->timeRemainingLabel) {
        ui->timeRemainingLabel->setText(tr("Temps restant estimé : 0s"));
        ui->timeRemainingLabel->setMinimumWidth(280);
        ui->timeRemainingLabel->setAlignment(Qt::AlignCenter);
    }

    // --- CORRECTIONS SAUTS INTERFACE ---
    // Fixe la taille du label de compte pour qu'il ne pousse pas le reste
    if (ui->shapeCountLabel) {
        ui->shapeCountLabel->setMinimumWidth(200);
    }

    // Contraint la largeur du menu de gauche via le layout (car widgetControls n'existe pas)
    if (ui->verticalLayout) {
        for(int i = 0; i < ui->verticalLayout->count(); ++i) {
            if (QWidget* w = ui->verticalLayout->itemAt(i)->widget()) {
                w->setMaximumWidth(350);
            }
        }
    }
    // --- CORRECTION DU SAUT D'INTERFACE LORS DES ERREURS ---
    if (ui->labelAIGenerationStatus) {
        ui->labelAIGenerationStatus->setWordWrap(true); // Force le texte long à passer à la ligne
        ui->labelAIGenerationStatus->setMinimumHeight(50); // Laisse de la place pour 2 lignes
        // Optionnel : on le met en rouge pour que l'erreur soit bien visible
        ui->labelAIGenerationStatus->setStyleSheet("color: #D32F2F; font-weight: bold;");
    }
}

void MainWindow::setupWorkspaceLayout()
{
    ui->centerVBox->setStretch(0, 0);
    ui->centerVBox->setStretch(1, 1);
    ui->centerVBox->setStretch(2, 0);
    ui->centerVBox->setStretch(3, 0);

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
        menuBar(), this,
        tr("Paramètres"), tr("Langue"), tr("Français"), tr("Anglais"),
        tr("Configurer le Wi-Fi"),
        [this]() { setLanguageFrench(); },
        [this]() { setLanguageEnglish(); },
        [this]() { m_coordinator->openWifiSettings(this); });

    settingsMenu    = handles.settingsMenu;
    languageMenu    = handles.languageMenu;
    actionFrench    = handles.actionFrench;
    actionEnglish   = handles.actionEnglish;
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
    shapeVisualization = qobject_cast<ShapeVisualization*>(ui->shapeVisualizationWidget);
    auto *svVm = new ShapeVisualizationViewModel(this);
    shapeVisualization->setProjectModel(svVm);
}

// =======================================================================
//  setupViewConnections — La View ne fait que relayer les actions UI
//  vers des signaux typés. Aucune logique métier ici.
// =======================================================================
void MainWindow::setupViewConnections()
{
    // ---- Synchronisation bidirectionnelle Slider ↔ SpinBox (pure UI) ----
    connect(ui->Longueur,        &QSpinBox::valueChanged,  ui->Slider_longueur, &QSlider::setValue);
    connect(ui->Slider_longueur, &QSlider::valueChanged,   ui->Longueur,        &QSpinBox::setValue);
    connect(ui->Largeur,         &QSpinBox::valueChanged,  ui->Slider_largeur,  &QSlider::setValue);
    connect(ui->Slider_largeur,  &QSlider::valueChanged,   ui->Largeur,         &QSpinBox::setValue);

    // ---- Dimensions → signal typé ----
    connect(ui->Largeur, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this]() { emit dimensionsChangeRequested(ui->Largeur->value(), ui->Longueur->value()); });
    connect(ui->Longueur, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this]() { emit dimensionsChangeRequested(ui->Largeur->value(), ui->Longueur->value()); });

    // ---- Nombre de formes / espacement → signal typé ----
    connect(ui->shapeCountSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::shapeCountChangeRequested);
    connect(ui->spaceSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::spacingChangeRequested);

    // ---- Formes prédéfinies ----
    connect(ui->Cercle,    &QPushButton::clicked, this, &MainWindow::circleRequested);
    connect(ui->Rectangle, &QPushButton::clicked, this, &MainWindow::rectangleRequested);
    connect(ui->Triangle,  &QPushButton::clicked, this, &MainWindow::triangleRequested);
    connect(ui->Etoile,    &QPushButton::clicked, this, &MainWindow::starRequested);
    connect(ui->Coeur,     &QPushButton::clicked, this, &MainWindow::heartRequested);

    // ---- Optimisation (la View gère seulement l'aspect toggle mutuel) ----
    connect(ui->optimizePlacementButton, &QPushButton::clicked, this, [this](bool checked) {
        ui->optimizePlacementButton2->setChecked(false);
        emit optimizePlacement1Requested(checked);
    });
    connect(ui->optimizePlacementButton2, &QPushButton::clicked, this, [this](bool checked) {
        ui->optimizePlacementButton->setChecked(false);
        emit optimizePlacement2Requested(checked);
    });

    // ---- Déplacement / rotation / ajout / suppression ----
    connect(ui->ButtonUp,            &QPushButton::clicked, this, &MainWindow::moveUpRequested);
    connect(ui->ButtonDown,          &QPushButton::clicked, this, &MainWindow::moveDownRequested);
    connect(ui->ButtonLeft,          &QPushButton::clicked, this, &MainWindow::moveLeftRequested);
    connect(ui->ButtonRight,         &QPushButton::clicked, this, &MainWindow::moveRightRequested);
    connect(ui->ButtonRotationLeft,  &QPushButton::clicked, this, &MainWindow::rotateLeftRequested);
    connect(ui->ButtonRotationRight, &QPushButton::clicked, this, &MainWindow::rotateRightRequested);
    connect(ui->ButtonAddShape,      &QPushButton::clicked, this, &MainWindow::addShapeRequested);
    connect(ui->ButtonDeleteShape,   &QPushButton::clicked, this, &MainWindow::deleteShapeRequested);

    // ---- Navigation ----
    connect(ui->buttonInventory,           &QPushButton::clicked, this, &MainWindow::inventoryRequested);
    connect(ui->buttonCustom,              &QPushButton::clicked, this, &MainWindow::customEditorRequested);
    connect(ui->buttonTestGpio,            &QPushButton::clicked, this, &MainWindow::folderRequested);
    connect(ui->buttonViewGeneratedImages, &QPushButton::clicked, this, &MainWindow::testGpioRequested);
    connect(ui->buttonFileReceiver,        &QPushButton::clicked, this, &MainWindow::bluetoothReceiverRequested);
    connect(ui->buttonWifiTransfer,        &QPushButton::clicked, this, &MainWindow::wifiTransferRequested);
    connect(ui->buttonTestMoteurs,         &QPushButton::clicked, this, &MainWindow::stmTestRequested);

    // ---- Sauvegarde ----
    connect(ui->ButtonSaveLayout, &QPushButton::clicked, this, &MainWindow::saveLayoutRequested);

    // ---- Coupe ----
    connect(ui->Play,  &QPushButton::clicked, this, &MainWindow::requestStartCut);
    connect(ui->Pause, &QPushButton::clicked, this, &MainWindow::requestPauseCut);
    connect(ui->Stop,  &QPushButton::clicked, this, &MainWindow::requestStopCut);
    connect(ui->SpinBox_vitesse, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::requestSpeedChange);

    // ---- AI ----
    connect(ui->buttonGenerateAI, &QPushButton::clicked, this, &MainWindow::generateAiRequested);

    // ---- Réactions de la View au ViewModel (découpe, langue) ----
    connect(m_viewModel, &MainWindowViewModel::cutProgressChanged, this, &MainWindow::updateProgressBar);
    connect(m_viewModel, &MainWindowViewModel::cutFinished,        this, &MainWindow::onCutFinished);
    connect(m_viewModel, &MainWindowViewModel::controlsEnabledChanged, this, &MainWindow::setSpinboxSliderEnabled);
    connect(m_viewModel, &MainWindowViewModel::languageChanged,    this, &MainWindow::onLanguageApplied);

    // ---- Réactions de la View au ViewModel (IA, shape progress) ----
    connect(m_viewModel, &MainWindowViewModel::aiStatusChanged,
            ui->labelAIGenerationStatus, &QLabel::setText);
    connect(m_viewModel, &MainWindowViewModel::aiImageReceived, this, &MainWindow::hideAiProgressBar);
    connect(m_viewModel, &MainWindowViewModel::shapeProgressChanged, this,
            [this](int current, int total) {
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

    // ---- Réactions de la View au ShapeVisualization ----
    connect(shapeVisualization, &ShapeVisualization::shapesPlacedCount, this,
            [this](int count) {
                ui->shapeCountLabel->setText(tr("Formes placées: %1").arg(count));
            });

    connect(shapeVisualization, &ShapeVisualization::actionRefused, this,
            [this](const QString &reason) {
                QMessageBox::warning(this, tr("Action refusée"), reason);
            });

    connect(shapeVisualization, &ShapeVisualization::optimizationStateChanged, this,
            [this](bool optimized) {
                if (!optimized) {
                    ui->optimizePlacementButton->setChecked(false);
                    ui->optimizePlacementButton2->setChecked(false);
                }
            });

    // ---- Synchroniser la View quand le Model change ----
    connect(m_model, &WorkspaceViewModel::largeurChanged, ui->Largeur, [this](int v) {
        ui->Largeur->blockSignals(true);
        ui->Largeur->setValue(v);
        ui->Largeur->blockSignals(false);
    });
    connect(m_model, &WorkspaceViewModel::longueurChanged, ui->Longueur, [this](int v) {
        ui->Longueur->blockSignals(true);
        ui->Longueur->setValue(v);
        ui->Longueur->blockSignals(false);
    });

    // ---- Connexion des signaux de découpe et de langue ----
    connect(this, &MainWindow::requestStartCut,    m_coordinator, &MainWindowCoordinator::startCutting);
    connect(this, &MainWindow::requestPauseCut,    m_coordinator, &MainWindowCoordinator::pauseCutting);
    connect(this, &MainWindow::requestStopCut,     m_coordinator, &MainWindowCoordinator::stopCutting);
    connect(this, &MainWindow::requestSpeedChange, m_coordinator, &MainWindowCoordinator::setCuttingSpeed);
    connect(this, &MainWindow::requestLanguageChange, m_coordinator, &MainWindowCoordinator::changeLanguage);
}

// =======================================================================
//  Slots publics — la View expose des méthodes pour que le Controller
//  puisse demander un changement d'affichage
// =======================================================================

void MainWindow::showAiProgressBar()
{
    ui->progressBarAI->setVisible(true);
    ui->progressBarAI->setMinimum(0);
    ui->progressBarAI->setMaximum(0);
}

void MainWindow::hideAiProgressBar()
{
    ui->progressBarAI->setVisible(false);
}

void MainWindow::displayCustomShapes(const QList<QPolygonF> &shapes, const QString &name)
{
    if (!shapeVisualization) return;
    shapeVisualization->displayCustomShapes(shapes);
    shapeVisualization->setCurrentCustomShapeName(name);
    showFullScreen();
}

void MainWindow::applyLayout(const LayoutData &layout)
{
    if (!shapeVisualization) return;
    shapeVisualization->applyLayout(layout);
    applySelectedLayoutToControls(layout);
}

void MainWindow::displayBaseShapeLayout(const QList<QPolygonF> &polys,
                                        const QString &name,
                                        const LayoutData &layout)
{
    if (!shapeVisualization) return;
    shapeVisualization->setCustomMode();
    shapeVisualization->displayCustomShapes(polys);
    shapeVisualization->setCurrentCustomShapeName(name);
    shapeVisualization->applyLayout(layout);
    applySelectedLayoutToControls(layout);
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

// --- Event Handlers ---

void MainWindow::updateProgressBar(int percentage, const QString &remainingTimeText)
{
    ui->progressBar->setVisible(true);
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(percentage);
    if (ui->timeRemainingLabel)
        ui->timeRemainingLabel->setText(remainingTimeText);
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        retranslateDynamicUi();
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
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
    if (settingsMenu)    settingsMenu->setTitle(tr("Paramètres"));
    if (languageMenu)    languageMenu->setTitle(tr("Langue"));
    if (actionFrench)    actionFrench->setText(tr("Français"));
    if (actionEnglish)   actionEnglish->setText(tr("Anglais"));
    if (actionWifiConfig) actionWifiConfig->setText(tr("Configurer le Wi-Fi"));

    if (ui->shapeCountLabel) {
        int count = ui->shapeCountSpinBox->value();
        ui->shapeCountLabel->setText(tr("Formes placées: %1").arg(count));
    }
    if (ui->timeRemainingLabel)
        ui->timeRemainingLabel->setText(tr("Temps restant estimé : 0s"));
}

void MainWindow::setSpinboxSliderEnabled(bool enabled)
{
    ui->Largeur->setEnabled(enabled);
    ui->Longueur->setEnabled(enabled);
    ui->Slider_largeur->setEnabled(enabled);
    ui->Slider_longueur->setEnabled(enabled);
    ui->shapeCountSpinBox->setEnabled(enabled);
    ui->spaceSpinBox->setEnabled(enabled);
}

void MainWindow::onCutFinished(bool /*success*/)
{
    ui->progressBar->setValue(0);
    ui->progressBar->setVisible(false);
}

void MainWindow::onLanguageApplied(Language lang, bool ok)
{
    Q_UNUSED(lang)
    if (!ok)
        QMessageBox::warning(this, tr("Langue"), tr("Impossible de charger la langue demandée."));
    retranslateDynamicUi();
}

MainWindow::~MainWindow()
{
    if (!m_ownsCoordinator)
        m_coordinator = nullptr;
    delete ui;
}

ShapeVisualization* MainWindow::getShapeVisualization() const
{
    return shapeVisualization;
}

Language MainWindow::displayLanguage() const
{
    return m_viewModel->currentLanguage();
}
