#include "MainWindow.h"
#include "ui_mainwindow.h"
#include "MachineViewModel.h"
#include "StmProtocol.h"
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
#include "InventoryViewModel.h"

#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QPainterPath>
#include <QDebug>
#include <QScreen>
#include <QGuiApplication>
#include <QApplication>
#include "ThemeManager.h"
#include <QShowEvent>
#include <QToolButton>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QMenu>
#include <QMenuBar>
#include <QSettings>
#include <QGridLayout>
#include <QSignalBlocker>
#include <QPointF>
#include <QSizeF>
#include <QTabWidget>
#include <QVBoxLayout>

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

    // Thème global — synchronisé via ThemeManager
    m_isDarkTheme = ThemeManager::instance()->isDark();
    connect(ThemeManager::instance(), &ThemeManager::themeChanged,
            this, [this](bool dark){ m_isDarkTheme = dark; updateThemeButton(); });

    setupUI();
    setupModels();
    setupViewConnections();

    m_coordinator->setMainWindow(this);
    m_coordinator->setShapeVisualization(shapeVisualization);

    // Le Coordinator connecte ses propres réactions aux signaux de la View
    m_coordinator->connectToView(this);

    // Tentative unique de connexion automatique au STM au démarrage.
    // Si la carte n'est pas branchée à cet instant, on laisse ensuite
    // le bouton manuel comme solution de secours.
    QTimer::singleShot(0, this, [this]() {
        auto* machineVm = m_coordinator ? m_coordinator->machineViewModel() : nullptr;
        if (!machineVm || machineVm->isConnected())
            return;

        machineVm->connectToStm(QString());
    });

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

    // On cache la barre de menu Qt et la status bar Qt natives
    menuBar()->hide();
    statusBar()->hide();

    ui->Slider_longueur->setValue(ui->Longueur->value());
    ui->Slider_largeur->setValue(ui->Largeur->value());

    ui->optimizePlacementButton->setCheckable(true);

    // Configuration de la barre de progression
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(0);
    ui->progressBar->setFormat(tr("Decoupe estimee : %p%"));
    ui->progressBar->setAlignment(Qt::AlignCenter);
    ui->progressBar->setTextVisible(true);

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
        ui->timeRemainingLabel->setText(tr("Temps restant estime : --"));
    }

    // --- CORRECTIONS SAUTS INTERFACE ---
    // Fixe la taille du label de compte pour qu'il ne pousse pas le reste
    if (ui->shapeCountLabel) {
        ui->shapeCountLabel->setMinimumWidth(200);
    }
    ui->spinBoxPlateauLargeur->setRange(5, 600);
    ui->spinBoxPlateauLongueur->setRange(5, 400);
    ui->spinBoxPlateauX->setRange(0, 600);
    ui->spinBoxPlateauY->setRange(0, 400);
    ui->Largeur->setRange(5, 600);
    ui->Longueur->setRange(5, 400);
    ui->Slider_largeur->setRange(5, 600);
    ui->Slider_longueur->setRange(5, 400);
    if (ui->Largeur->value() < 5)
        ui->Largeur->setValue(5);
    if (ui->Longueur->value() < 5)
        ui->Longueur->setValue(5);
    setupSurfaceEditTabs();
    setSurfaceEditMode(false);

    // --- CORRECTION DU SAUT D'INTERFACE LORS DES ERREURS ---
    if (ui->labelAIGenerationStatus) {
        ui->labelAIGenerationStatus->setWordWrap(true); // Force le texte long à passer à la ligne
        ui->labelAIGenerationStatus->setMinimumHeight(50); // Laisse de la place pour 2 lignes
        // Optionnel : on le met en rouge pour que l'erreur soit bien visible
        ui->labelAIGenerationStatus->setStyleSheet("color: #D32F2F; font-weight: bold;");
    }
    if (ui->gridShapes && ui->tabSystemLayout) {
        ui->gridShapes->removeWidget(ui->buttonCustom);
        ui->gridShapes->removeWidget(ui->buttonInventory);

        const int insertIndex = qMax(0, ui->tabSystemLayout->count() - 1);
        ui->tabSystemLayout->insertWidget(insertIndex, ui->buttonCustom);
        ui->tabSystemLayout->insertWidget(insertIndex + 1, ui->buttonInventory);
    }

    connect(ui->leftTabWidget, &QTabWidget::currentChanged, this, [this](int) {
        QTimer::singleShot(0, this, [this]() { refreshQuickShapeButtons(); });
    });

    ui->progressBar->setFormat(tr("Decoupe estimee : %p%"));
    refreshQuickShapeButtons();
}

void MainWindow::setupWorkspaceLayout()
{
    auto *visualizationWidget = ui->shapeVisualizationWidget;
    visualizationWidget->setSheetSizeMm(QSizeF(600, 400));
    visualizationWidget->setToolTip(tr("Le grand rectangle represente la machine 600 x 400 mm. Glissez le rectangle bleu pour positionner le support reel."));

    auto *wrapper = new AspectRatioWrapper(nullptr, 600.0 / 400.0, ui->centralwidget);
    wrapper->setObjectName("formeRatioWrapper");
    wrapper->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // CORRECTION ICI : On utilise bodyLayout, qui est le grand layout central
    ui->bodyLayout->replaceWidget(visualizationWidget, wrapper);
    wrapper->setChild(visualizationWidget);

    wrapper->setAspect(600.0 / 400.0);
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
    ThemeManager::instance()->applyToApp();
    updateThemeButton();
}

void MainWindow::updateThemeButton()
{
    if (!ui->buttonTheme) return;
    QString iconPath = m_isDarkTheme ? ":/icons/moon.svg" : ":/icons/sun.svg";
    ui->buttonTheme->setIcon(QIcon(iconPath));
}

void MainWindow::toggleTheme()
{
    QWidget *cw = centralWidget();
    auto *effect = new QGraphicsOpacityEffect(cw);
    cw->setGraphicsEffect(effect);

    auto *fadeOut = new QPropertyAnimation(effect, "opacity", this);
    fadeOut->setDuration(120);
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);

    connect(fadeOut, &QPropertyAnimation::finished, this, [this, cw, effect]() {
        ThemeManager::instance()->toggle();
        if (shapeVisualization) {
            shapeVisualization->updateGeometry();
            if (QWidget *wrapper = shapeVisualization->parentWidget()) {
                wrapper->updateGeometry();
                wrapper->update();
            }
        }
        if (ui && ui->bodyLayout) {
            ui->bodyLayout->invalidate();
            ui->bodyLayout->activate();
        }

        auto *fadeIn = new QPropertyAnimation(effect, "opacity", this);
        fadeIn->setDuration(120);
        fadeIn->setStartValue(0.0);
        fadeIn->setEndValue(1.0);
        connect(fadeIn, &QPropertyAnimation::finished, cw, [cw]() {
            cw->setGraphicsEffect(nullptr);
        });
        fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
    });
    fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
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

    // ---- Plateau → signal typé ----
    connect(ui->buttonEditCutSurface, &QPushButton::clicked, this, [this]() {
        setSurfaceEditMode(true);
    });
    connect(ui->buttonApplyCutSurface, &QPushButton::clicked, this, [this]() {
        emitSurfaceChangeFromControls();
        setSurfaceEditMode(false);
    });
    connect(ui->spinBoxPlateauLargeur, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this]() { updateSurfaceControlLimits(); emitSurfaceChangeFromControls(); });
    connect(ui->spinBoxPlateauLongueur, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this]() { updateSurfaceControlLimits(); emitSurfaceChangeFromControls(); });
    connect(ui->spinBoxPlateauX, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this]() { emitSurfaceChangeFromControls(); });
    connect(ui->spinBoxPlateauY, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this]() { emitSurfaceChangeFromControls(); });

    // ---- Nombre de formes / espacement → signal typé ----
    connect(ui->shapeCountSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::shapeCountChangeRequested);
    connect(ui->spaceSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::spacingChangeRequested);

    // ---- Thème ----
    connect(ui->buttonTheme, &QPushButton::clicked, this, &MainWindow::toggleTheme);

    // ---- Déplacement / rotation / ajout / suppression ----
    connect(ui->ButtonUp,            &QPushButton::clicked, this, &MainWindow::moveUpRequested);
    connect(ui->ButtonDown,          &QPushButton::clicked, this, &MainWindow::moveDownRequested);
    connect(ui->ButtonLeft,          &QPushButton::clicked, this, &MainWindow::moveLeftRequested);
    connect(ui->ButtonRight,         &QPushButton::clicked, this, &MainWindow::moveRightRequested);
    connect(ui->ButtonRotationLeft,  &QPushButton::clicked, this, &MainWindow::rotateLeftRequested);
    connect(ui->ButtonRotationRight, &QPushButton::clicked, this, &MainWindow::rotateRightRequested);
    connect(ui->ButtonAddShape,      &QPushButton::clicked, this, &MainWindow::addShapeRequested);
    connect(ui->ButtonDeleteShape,   &QPushButton::clicked, this, &MainWindow::deleteShapeRequested);

    // ---- Optimisation ----
    connect(ui->optimizePlacementButton, &QPushButton::toggled,
            this, &MainWindow::optimizePlacement1Requested);

    // ---- Navigation ----
    connect(ui->buttonInventory,           &QPushButton::clicked, this, &MainWindow::inventoryRequested);
    connect(ui->buttonCustom,              &QPushButton::clicked, this, &MainWindow::customEditorRequested);
    connect(ui->buttonViewGeneratedImages, &QPushButton::clicked, this, &MainWindow::folderRequested);
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

    // ---- Réactions de la View au ShapeVisualization ----
    connect(shapeVisualization, &ShapeVisualization::shapesPlacedCount, this,
            [this](int count) {
                m_lastShapeCount = count;
                updatePositionLabelDisplay();
                if (ui->shapeCountLabel) {
                    ui->shapeCountLabel->setText(tr("Formes placées: %1").arg(count));
                }
            });

    connect(shapeVisualization, &ShapeVisualization::actionRefused, this,
            [this](const QString &reason) {
                QMessageBox::warning(this, tr("Action refusée"), reason);
            });

    connect(shapeVisualization, &ShapeVisualization::sheetSizeMmChanged, this,
            [this](const QSizeF &) {
                syncSurfaceControlsFromVisualization();
            });
    connect(shapeVisualization, &ShapeVisualization::sheetOriginMmChanged, this,
            [this](const QPointF &) {
                syncSurfaceControlsFromVisualization();
            });

    connect(shapeVisualization, &ShapeVisualization::optimizationStateChanged, this,
            [this](bool optimized) {
                if (!optimized) {
                    ui->optimizePlacementButton->setChecked(false);
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

    // ---- Etat machine → status bar ----
    auto *mvm = m_coordinator->machineViewModel();
    connect(mvm, &MachineViewModel::stateChanged,
            this, &MainWindow::onMachineStateChanged);
    connect(shapeVisualization, &ShapeVisualization::headLogicalPositionChanged,
            this, [this](double x, double y) {
                m_lastHeadX = x;
                m_lastHeadY = y;
                updatePositionLabelDisplay();
            });
    onMachineStateChanged(mvm->state());

    // ---- Boutons machine secondaires (status bar) ----
    connect(ui->buttonHome,  &QPushButton::clicked, this, &MainWindow::homeRequested);
    connect(ui->buttonReset, &QPushButton::clicked, this, &MainWindow::positionResetRequested);
    connect(ui->buttonRearm, &QPushButton::clicked, this, &MainWindow::rearmRequested);

    // ---- Bouton parametres (popup du menu existant) ----
    connect(ui->buttonSettings, &QPushButton::clicked, this, [this]() {
        if (settingsMenu) {
            settingsMenu->popup(ui->buttonSettings->mapToGlobal(
                QPoint(0, ui->buttonSettings->height())));
        }
    });

    // ---- Bouton langue (toggle FR/EN) ----
    connect(ui->buttonLanguage, &QPushButton::clicked, this, [this]() {
        const Language next = (displayLanguage() == Language::French)
                                  ? Language::English
                                  : Language::French;
        ui->buttonLanguage->setText(next == Language::French ? "EN" : "FR");
        emit requestLanguageChange(next);
    });
    ui->buttonLanguage->setText(displayLanguage() == Language::French ? "FR" : "EN");
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
    refreshQuickShapeButtons();
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
    refreshQuickShapeButtons();
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
    percentage = qBound(0, percentage, 100);
    ui->progressBar->setVisible(true);
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(percentage);
    ui->progressBar->setFormat(tr("Decoupe estimee : %1%").arg(percentage));
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
    QTimer::singleShot(0, this, [this]() {
        if (ui && ui->bodyLayout) {
            ui->bodyLayout->invalidate();
            ui->bodyLayout->activate();
        }
        if (ui && ui->rightLayout) {
            ui->rightLayout->invalidate();
            ui->rightLayout->activate();
        }
        if (shapeVisualization) {
            shapeVisualization->updateGeometry();
            shapeVisualization->update();
            if (QWidget *wrapper = shapeVisualization->parentWidget()) {
                wrapper->updateGeometry();
                wrapper->update();
            }
        }
        refreshQuickShapeButtons();
    });
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (ui && ui->rightLayout) {
        ui->rightLayout->invalidate();
        ui->rightLayout->activate();
    }
    if (shapeVisualization) {
        shapeVisualization->updateGeometry();
    }
    refreshQuickShapeButtons();
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

    if (ui->timeRemainingLabel)
        ui->timeRemainingLabel->setText(tr("Temps restant estime : --"));
    ui->progressBar->setFormat(tr("Decoupe estimee : %p%"));
    refreshQuickShapeButtons();
}

void MainWindow::setSpinboxSliderEnabled(bool enabled)
{
    ui->Play->setEnabled(enabled);
    ui->Largeur->setEnabled(enabled);
    ui->Longueur->setEnabled(enabled);
    ui->Slider_largeur->setEnabled(enabled);
    ui->Slider_longueur->setEnabled(enabled);
    ui->shapeCountSpinBox->setEnabled(enabled);
    ui->spaceSpinBox->setEnabled(enabled);
}

void MainWindow::onCutFinished(bool success)
{
    ui->progressBar->setValue(0);
    ui->progressBar->setVisible(false);
    ui->progressBar->setFormat(tr("Decoupe estimee : %p%"));
    if (ui->timeRemainingLabel) {
        const QString message = success
            ? tr("Decoupe terminee")
            : tr("Decoupe interrompue");
        ui->timeRemainingLabel->setText(message);
        QTimer::singleShot(3500, this, [this, message]() {
            if (ui->timeRemainingLabel && ui->timeRemainingLabel->text() == message)
                ui->timeRemainingLabel->setText(tr("Temps restant estime : --"));
        });
    }
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

void MainWindow::refreshQuickShapeButtons()
{
    if (!m_inventory || !m_inventory->viewModel())
        return;

    rebuildQuickShapeButtons();
}

int MainWindow::quickShapeCapacity() const
{
    if (!ui || !ui->leftTabWidget || !ui->gridShapes)
        return 0;

    const int availableWidth = qMax(180, ui->leftTabWidget->width() - 32);
    const int availableHeight = qMax(180, ui->leftTabWidget->height() - 72);
    const int minButtonWidth = 110;
    const int buttonHeight = 56;
    const int spacing = ui->gridShapes->spacing() > 0 ? ui->gridShapes->spacing() : 8;

    const int columns = qMax(1, (availableWidth + spacing) / (minButtonWidth + spacing));
    const int rows = qMax(1, (availableHeight + spacing) / (buttonHeight + spacing));
    return qMax(1, columns * rows);
}

void MainWindow::clearQuickShapeButtons()
{
    if (!ui || !ui->gridShapes)
        return;

    while (QLayoutItem *item = ui->gridShapes->takeAt(0)) {
        if (QWidget *widget = item->widget()) {
            const bool isStaticShapeButton =
                widget == ui->Cercle ||
                widget == ui->Rectangle ||
                widget == ui->Triangle ||
                widget == ui->Etoile ||
                widget == ui->Coeur;

            if (isStaticShapeButton) {
                widget->hide();
            } else {
                widget->deleteLater();
            }
        }
        delete item;
    }
    m_quickShapeButtons.clear();
}

void MainWindow::rebuildQuickShapeButtons()
{
    if (!ui || !ui->gridShapes || !m_inventory || !m_inventory->viewModel())
        return;

    const int capacity = quickShapeCapacity();
    if (capacity <= 0)
        return;

    const QList<QuickShapeEntry> shapes = m_inventory->viewModel()->getQuickAccessShapes(capacity);
    const int availableWidth = qMax(180, ui->leftTabWidget->width() - 32);
    const int spacing = ui->gridShapes->spacing() > 0 ? ui->gridShapes->spacing() : 8;
    const int minButtonWidth = 110;
    const int columns = qMax(1, (availableWidth + spacing) / (minButtonWidth + spacing));

    clearQuickShapeButtons();

    for (int index = 0; index < shapes.size(); ++index) {
        const QuickShapeEntry &entry = shapes.at(index);
        auto *button = new QPushButton(entry.name, this);
        button->setMinimumHeight(56);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        button->setIcon(QIcon());

        if (entry.isBaseShape) {
            connect(button, &QPushButton::clicked, this, [this, entry]() {
                emit baseShapeRequested(static_cast<int>(entry.baseType));
            });
        } else {
            connect(button, &QPushButton::clicked, this, [this, entry]() {
                emit customShapeRequested(entry.name);
            });
        }

        const int row = index / columns;
        const int column = index % columns;
        ui->gridShapes->addWidget(button, row, column);
        m_quickShapeButtons.append(button);
    }
}

void MainWindow::setupSurfaceEditTabs()
{
    if (m_surfaceEditTabs || !ui || !ui->rightLayout)
        return;

    m_surfaceEditTabs = new QTabWidget(ui->rightPanel);
    m_surfaceEditTabs->setObjectName(QStringLiteral("surfaceEditTabs"));
    m_surfaceEditTabs->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_surfaceEditTabs->setMaximumHeight(360);
    connect(m_surfaceEditTabs, &QTabWidget::currentChanged, this, [this](int index) {
        if (!m_surfaceEditTabs)
            return;
        m_surfaceEditTabs->setMaximumHeight(index == 0 ? 360 : 760);
        ui->rightLayout->invalidate();
        ui->rightLayout->activate();
    });

    m_surfaceEditPage = new QWidget(m_surfaceEditTabs);
    auto *surfaceLayout = new QVBoxLayout(m_surfaceEditPage);
    surfaceLayout->setContentsMargins(0, 0, 0, 0);
    surfaceLayout->setSpacing(8);
    surfaceLayout->setAlignment(Qt::AlignTop);

    m_shapeEditPage = new QWidget(m_surfaceEditTabs);
    auto *shapeLayout = new QVBoxLayout(m_shapeEditPage);
    shapeLayout->setContentsMargins(0, 0, 0, 0);
    shapeLayout->setSpacing(8);
    shapeLayout->setAlignment(Qt::AlignTop);

    m_surfaceEditTabs->addTab(m_surfaceEditPage, tr("Surface"));
    m_surfaceEditTabs->addTab(m_shapeEditPage, tr("Formes"));
    m_surfaceEditTabs->hide();

    const int insertIndex = ui->rightLayout->indexOf(ui->buttonEditCutSurface) + 1;
    ui->rightLayout->insertWidget(qMax(0, insertIndex), m_surfaceEditTabs);
}

void MainWindow::setSurfaceEditMode(bool editing)
{
    m_surfaceEditMode = editing;
    setupSurfaceEditTabs();

    if (ui->buttonEditCutSurface)
        ui->buttonEditCutSurface->setVisible(!editing);

    if (editing) {
        if (auto *layout = qobject_cast<QVBoxLayout*>(m_surfaceEditPage->layout()))
            layout->addWidget(ui->groupPlateau);
        if (auto *layout = qobject_cast<QVBoxLayout*>(m_shapeEditPage->layout())) {
            layout->addWidget(ui->groupDimensions);
            layout->addWidget(ui->groupProprietes);
            layout->addWidget(ui->groupOptimisation);
        }
        if (m_surfaceEditTabs) {
            m_surfaceEditTabs->setCurrentIndex(0);
            m_surfaceEditTabs->setMaximumHeight(360);
            m_surfaceEditTabs->show();
        }
        ui->groupPlateau->show();
        ui->groupDimensions->show();
        ui->groupProprietes->show();
        syncSurfaceControlsFromVisualization();
    } else {
        if (m_surfaceEditTabs)
            m_surfaceEditTabs->hide();
        ui->groupPlateau->hide();
        const int insertIndex = qMax(0, ui->rightLayout->indexOf(ui->buttonEditCutSurface) + 1);
        ui->rightLayout->insertWidget(insertIndex, ui->groupDimensions);
        ui->rightLayout->insertWidget(insertIndex + 1, ui->groupProprietes);
        ui->rightLayout->insertWidget(insertIndex + 2, ui->groupOptimisation);
        ui->groupDimensions->show();
        ui->groupProprietes->show();
        ui->groupOptimisation->show();
    }

    ui->rightLayout->invalidate();
    ui->rightLayout->activate();
    updateGeometry();
    emit cutSurfaceEditModeChanged(editing);
}

void MainWindow::syncSurfaceControlsFromVisualization()
{
    if (!shapeVisualization)
        return;

    const QSizeF size = shapeVisualization->sheetSizeMm();
    const QPointF origin = shapeVisualization->sheetOriginMm();

    const QSignalBlocker blockWidth(ui->spinBoxPlateauLargeur);
    const QSignalBlocker blockHeight(ui->spinBoxPlateauLongueur);
    const QSignalBlocker blockX(ui->spinBoxPlateauX);
    const QSignalBlocker blockY(ui->spinBoxPlateauY);

    ui->spinBoxPlateauLargeur->setValue(qRound(size.width()));
    ui->spinBoxPlateauLongueur->setValue(qRound(size.height()));
    updateSurfaceControlLimits();
    ui->spinBoxPlateauX->setValue(qRound(origin.x()));
    ui->spinBoxPlateauY->setValue(qRound(origin.y()));
}

void MainWindow::updateSurfaceControlLimits()
{
    const int width = qBound(5, ui->spinBoxPlateauLargeur->value(), 600);
    const int height = qBound(5, ui->spinBoxPlateauLongueur->value(), 400);

    const QSignalBlocker blockX(ui->spinBoxPlateauX);
    const QSignalBlocker blockY(ui->spinBoxPlateauY);
    ui->spinBoxPlateauLargeur->setMinimum(5);
    ui->spinBoxPlateauLongueur->setMinimum(5);
    ui->spinBoxPlateauLargeur->setMaximum(600);
    ui->spinBoxPlateauLongueur->setMaximum(400);
    ui->spinBoxPlateauX->setMaximum(qMax(0, 600 - width));
    ui->spinBoxPlateauY->setMaximum(qMax(0, 400 - height));
}

void MainWindow::emitSurfaceChangeFromControls()
{
    if (!m_surfaceEditMode)
        return;

    updateSurfaceControlLimits();
    emit cutSurfaceChangeRequested(ui->spinBoxPlateauLargeur->value(),
                                   ui->spinBoxPlateauLongueur->value(),
                                   ui->spinBoxPlateauX->value(),
                                   ui->spinBoxPlateauY->value());
}

void MainWindow::onMachineStateChanged(MachineState state)
{
    struct StateStyle { QString label; QString bg; QString fg; };
    static const QMap<MachineState, StateStyle> styles = {
        { MachineState::DISCONNECTED,  { tr("DECONNECTE"),   "#272A30", "#94A3B8" } },
        { MachineState::READY,         { tr("PRET"),         "#15803D", "#FFFFFF" } },
        { MachineState::MOVING,        { tr("EN MOUVEMENT"), "#0284C7", "#FFFFFF" } },
        { MachineState::HOMING,        { tr("HOMING"),       "#7E22CE", "#FFFFFF" } },
        { MachineState::RECOVERY_WAIT, { tr("RECOVERY"),     "#B45309", "#FFFFFF" } },
        { MachineState::EMERGENCY,     { tr("URGENCE"),      "#991B1B", "#FFFFFF" } },
        { MachineState::ALARM,         { tr("ALARME"),       "#C2410C", "#FFFFFF" } },
    };

    const StateStyle &s = styles.value(state, styles[MachineState::DISCONNECTED]);

    // Déterminer si la machine est en découpe / mouvement
    m_isCutting = (state == MachineState::MOVING || state == MachineState::HOMING);
    updatePositionLabelDisplay();

    ui->labelMachineState->setText(s.label);
    ui->labelMachineState->setStyleSheet(
        QString("background-color: %1;"
                "color: %2;"
                "border-radius: 20px;"
                "padding: 6px 20px;"
                "font-size: 14px;"
                "font-weight: 700;"
                "letter-spacing: 1px;")
            .arg(s.bg, s.fg));

    if (state == MachineState::EMERGENCY) {
        if (!m_emergencyFlashTimer) {
            m_emergencyFlashTimer = new QTimer(this);
            m_emergencyFlashTimer->setInterval(500);
            bool flashState = false;
            connect(m_emergencyFlashTimer, &QTimer::timeout, this, [this, flashState]() mutable {
                flashState = !flashState;
                const QString border = flashState ? "border: 2px solid #EF4444;" : "border: 2px solid transparent;";
                ui->labelMachineState->setStyleSheet(
                    ui->labelMachineState->styleSheet() + border);
            });
        }
        m_emergencyFlashTimer->start();
    } else if (m_emergencyFlashTimer) {
        m_emergencyFlashTimer->stop();
    }
}

void MainWindow::updatePositionLabelDisplay()
{
    if (!ui->labelPositionXY) return;

    if (m_isCutting) {
        ui->labelPositionXY->setText(
            QString("X: %1 mm     Y: %2 mm")
                .arg(m_lastHeadX, 7, 'f', 2)
                .arg(m_lastHeadY, 7, 'f', 2));
    } else {
        ui->labelPositionXY->setText(tr("Formes placées sur le support : %1").arg(m_lastShapeCount));
    }
}
