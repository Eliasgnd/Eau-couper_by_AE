#include "MainWindow.h"
#include "ScreenUtils.h"
#include "ShapeModel.h"
#include "qmessagebox.h"
#include "ui_mainwindow.h"
#include "Inventory.h"
#include "ShapeVisualization.h"
#include "Language.h"
#include "ImageImportService.h"
#include "AIImagePromptDialog.h"
#include "AIImageProcessDialog.h"
#include "WifiTransferWidget.h"
#include "AspectRatioWrapper.h"
#include "NavigationController.h"
#include "AIServiceManager.h"
#include "GeometryUtils.h"

#include <QSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QImage>
#include <QPainterPath>
#include <QDebug>
#include <QScreen>
#include <QGuiApplication>
#include <QShowEvent>
#include <QTimer>
#include <QWindow>
#include <QPoint>
#include <QWidgetAction>
#include <QToolButton>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>


//Ceci est le Mainwindow,bienvenue dans le mainwindow

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_navigationController = new NavigationController(this);
    m_aiServiceManager = new AIServiceManager(this);
    setupUI();
    setupModels();
    setupConnections();
}

void MainWindow::setupUI()
{
    // La barre de titre (index 0) ne prend pas de stretch, la vue (index 1) prend tout, le bas (index 2) reste compact.
    ui->centerVBox->setStretch(0, 0);  // topBarLayout
    ui->centerVBox->setStretch(1, 1);  // horizontalLayout contenant shapeVisualizationWidget
    ui->centerVBox->setStretch(2, 0);  // horizontalLayout_2 (commandes + Réglages)
    ui->centerVBox->setStretch(3, 0);  // verticalLayout_2 (vide chez toi)

    // --- AJOUT : facteurs de stretch des 3 colonnes du layout principal ---
    // Étirements des colonnes (pas dans le .ui pour éviter l'erreur uic)
    ui->mainHorizontalLayout->setStretch(0, 0); // colonne gauche (fixe)
    ui->mainHorizontalLayout->setStretch(1, 1); // colonne droite (expansive)

    auto fv = ui->shapeVisualizationWidget;   // ShapeVisualization* issu du .ui
    fv->setSheetSizeMm(QSizeF(600, 400));     // OK

    // 1) Crée le wrapper SANS re‑parenter fv
    auto wrapper = new AspectRatioWrapper(nullptr, 600.0/400.0, ui->centralwidget);
    wrapper->setObjectName("formeRatioWrapper");
    wrapper->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 2) Remplace fv par wrapper TANT QUE fv est encore dans le layout
    ui->horizontalLayout->replaceWidget(fv, wrapper);

    // 3) Maintenant, on adopte fv à l’intérieur du wrapper
    wrapper->setChild(fv);

    // 4) Mets à jour le ratio du wrapper si la taille de feuille change
    connect(fv, &ShapeVisualization::sheetSizeMmChanged,
            wrapper, [wrapper](const QSizeF& mm) {
                if (mm.width() > 0.0 && mm.height() > 0.0)
                    wrapper->setAspect(mm.width() / mm.height());
            });
    setupMenus();
    applyStyleSheets();


    // place la fenêtre sur le 2ᵉ écran
    // ScreenUtils::placeOnSecondaryScreen(this);

    // Connection de l'inventory à la forme
    // Bon : on récupère directement les valeurs actuelles des spinboxes longueur/largeur
    updateSliderLongueur(ui->Longueur->value());
    updateSliderLargeur (ui->Largeur ->value());
    // activation et desactivation de l'optimisation

    ui->optimizePlacementButton->setCheckable(true);
    ui->optimizePlacementButton2->setCheckable(true);

    // Configuration de la barre de progression
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(0);
    ui->progressBar->setFormat("%p%");
    ui->progressBar->setAlignment(Qt::AlignCenter);


    if (ui->timeRemainingLabel)
        ui->timeRemainingLabel->setText(tr("Temps restant estimé : 0s"));
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
    connect(actionWifiConfig, &QAction::triggered, this, &MainWindow::openWifiConfig);

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
}

void MainWindow::setupShapeConnections()
{
    // Connection de l'inventory à la forme
    QObject::connect(Inventory::getInstance(), &Inventory::shapeSelected,
                     this, &MainWindow::onShapeSelectedFromInventory);

    // Connecter le signal du nombre de formes placées pour mettre à jour le label
    connect(shapeVisualization, &ShapeVisualization::shapesPlacedCount,
            this, &MainWindow::updateShapeCountLabel);

    // Connecter les spinbox pour les dimensions
    connect(ui->Largeur, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updateShape);
    connect(ui->Longueur, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updateShape);

    // Connecter les boutons pour changer les modèles
    connect(ui->Cercle, &QPushButton::clicked, this, [this]() {
        setPredefinedShape(ShapeModel::Type::Circle);
    });
    connect(ui->Rectangle, &QPushButton::clicked, this, [this]() {
        setPredefinedShape(ShapeModel::Type::Rectangle);
    });
    connect(ui->Triangle, &QPushButton::clicked, this, [this]() {
        setPredefinedShape(ShapeModel::Type::Triangle);
    });
    connect(ui->Etoile, &QPushButton::clicked, this, [this]() {
        setPredefinedShape(ShapeModel::Type::Star);
    });
    connect(ui->Coeur, &QPushButton::clicked, this, [this]() {
        setPredefinedShape(ShapeModel::Type::Heart);
    });

    // connexion pour les formes de l'inventory
    connect(Inventory::getInstance(), &Inventory::customShapeSelected,
            this,                       &MainWindow::onCustomShapeSelected);
}

void MainWindow::setupNavigationConnections()
{
    connect(m_navigationController, &NavigationController::customShapeApplied,
            this, &MainWindow::applyCustomShape);
    connect(m_navigationController, &NavigationController::customDrawingReset,
            this, &MainWindow::resetDrawing);
    connect(m_navigationController, &NavigationController::layoutSelected, this,
            [this](const LayoutData &layout) {
                shapeVisualization->applyLayout(layout);
                applySelectedLayoutToControls(layout);
            });
    connect(m_navigationController, &NavigationController::baseShapeLayoutSelected, this,
            [this](ShapeModel::Type type, const LayoutData &layout) {
                selectedShapeType = type;
                QList<QPolygonF> shapePolys = ShapeModel::shapePolygons(type, 100, 100);
                shapeVisualization->setCustomMode();
                shapeVisualization->displayCustomShapes(shapePolys);
                shapeVisualization->setCurrentCustomShapeName(Inventory::baseShapeName(type, Language::French));
                shapeVisualization->applyLayout(layout);
                applySelectedLayoutToControls(layout);
            });
    connect(m_navigationController, &NavigationController::baseShapeOnlySelected, this,
            [this](ShapeModel::Type type) {
                setPredefinedShape(type);
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
    connect(ui->buttonInventory, &QPushButton::clicked, this, &MainWindow::showInventory);
    connect(ui->buttonCustom, &QPushButton::clicked, this, &MainWindow::showCustom);
    // "Images générées" should open the gallery of generated images
    connect(ui->buttonTestGpio, &QPushButton::clicked, this, &MainWindow::showFolder);

    connect(ui->buttonGenerateAI, &QPushButton::clicked, this, &MainWindow::openAIImagePromptDialog);
    // Small button with the download icon opens the GPIO test page
    connect(ui->buttonViewGeneratedImages, &QPushButton::clicked, this, &MainWindow::openTestGpio);
    connect(ui->buttonFileReceiver, &QPushButton::clicked, this, &MainWindow::on_receptionFichierButton_clicked);
    connect(ui->buttonWifiTransfer, &QPushButton::clicked, this, &MainWindow::openWifiTransfer);
}

void MainWindow::setupSystemConnections()
{
    // Connecter les spinboxes aux sliders
    connect(ui->Longueur, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updateSliderLongueur);
    connect(ui->Largeur, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updateSliderLargeur);
    connect(ui->Slider_longueur, &QSlider::valueChanged, this, &MainWindow::updateSpinBoxLongueur);
    connect(ui->Slider_largeur, &QSlider::valueChanged, this, &MainWindow::updateSpinBoxLargeur);

    // Connection entre spinbox nombre de formes et ShapeVisualization
    connect(ui->shapeCountSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updateShapeCount);

    connect(ui->optimizePlacementButton, &QPushButton::clicked,
            this, &MainWindow::onOptimizePlacementClicked);

    connect(ui->optimizePlacementButton2, &QPushButton::clicked,
            this, &MainWindow::onOptimizePlacement2Clicked);

    // spinBox pour l'espacement
    connect(ui->spaceSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::updateSpacing);

    connect(shapeVisualization, &ShapeVisualization::spacingChanged,
            this, [this](int newSpacing) {
                ui->spaceSpinBox->setValue(newSpacing);
            });

    // Déplacement : 1 pixel par clic
    connect(ui->ButtonUp, &QPushButton::clicked, this, &MainWindow::onMoveUpClicked);
    connect(ui->ButtonDown, &QPushButton::clicked, this, &MainWindow::onMoveDownClicked);
    connect(ui->ButtonLeft, &QPushButton::clicked, this, &MainWindow::onMoveLeftClicked);
    connect(ui->ButtonRight, &QPushButton::clicked, this, &MainWindow::onMoveRightClicked);

    connect(ui->ButtonRotationLeft, &QPushButton::clicked, this, [this]() {
        shapeVisualization->rotateSelectedShapes(-90); // rotation vers la gauche
    });
    connect(ui->ButtonRotationRight, &QPushButton::clicked, this, [this]() {
        shapeVisualization->rotateSelectedShapes(90);  // rotation vers la droite
    });

    connect(ui->ButtonAddShape, &QPushButton::clicked, this, [this]() {
        shapeVisualization->addShapeBottomRight();
    });

    connect(ui->ButtonDeleteShape, &QPushButton::clicked, this, [this]() {
        shapeVisualization->deleteSelectedShapes();
    });

    connect(ui->ButtonSaveLayout, &QPushButton::clicked, this, &MainWindow::onSaveLayoutClicked);

    connect(ui->Play, &QPushButton::clicked, this, &MainWindow::StartPixel);
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

void MainWindow::updateShape() {
    int largeur = ui->Largeur->value();
    int longueur = ui->Longueur->value();
    shapeVisualization->updateDimensions(largeur, longueur);
}

void MainWindow::setPredefinedShape(ShapeModel::Type type)
{
    selectedShapeType = type;
    shapeVisualization->setPredefinedMode();
    shapeVisualization->setModel(type);
}

void MainWindow::moveSelectedShape(int dx, int dy)
{
    shapeVisualization->moveSelectedShapes(dx, dy);
}

void MainWindow::onMoveUpClicked()
{
    moveSelectedShape(0, -1);
}

void MainWindow::onMoveDownClicked()
{
    moveSelectedShape(0, 1);
}

void MainWindow::onMoveLeftClicked()
{
    moveSelectedShape(-1, 0);
}

void MainWindow::onMoveRightClicked()
{
    moveSelectedShape(1, 0);
}

void MainWindow::onOptimizePlacementClicked()
{
    if (ui->optimizePlacementButton->isChecked()) {
        ui->optimizePlacementButton2->setChecked(false);
        shapeVisualization->optimizePlacement();
    } else {
        int largeur = ui->Largeur->value();
        int longueur = ui->Longueur->value();
        shapeVisualization->updateDimensions(largeur, longueur);
    }
}

void MainWindow::onOptimizePlacement2Clicked()
{
    if (ui->optimizePlacementButton2->isChecked()) {
        ui->optimizePlacementButton->setChecked(false);
        shapeVisualization->optimizePlacement2();
    } else {
        int largeur = ui->Largeur->value();
        int longueur = ui->Longueur->value();
        shapeVisualization->updateDimensions(largeur, longueur);
    }
}

void MainWindow::onSaveLayoutClicked()
{
    auto saveLayout = [this]() {
        bool ok = false;
        QString name = m_navigationController->promptForName(this,
                                                             tr("Nom de la disposition"),
                                                             tr("Entrez un nom"),
                                                             &ok);
        if (!ok || name.isEmpty())
            return;
        LayoutData layout = shapeVisualization->captureCurrentLayout(name);
        if (shapeVisualization->isCustomMode())
            Inventory::getInstance()->addLayoutToShape(shapeVisualization->currentCustomShapeName(), layout);
        else
            Inventory::getInstance()->addLayoutToBaseShape(selectedShapeType, layout);
    };

    if (shapeVisualization->isCustomMode() && shapeVisualization->currentCustomShapeName().isEmpty()) {
        QMessageBox::warning(this,
                             tr("Disposition"),
                             tr("Forme non sauvegardée — veuillez la sauvegarder d'abord."));
        if (promptAndSaveCurrentCustomShape())
            saveLayout();
        return;
    }

    saveLayout();
}

void MainWindow::showInventory() {
    m_navigationController->showInventory(this, Inventory::getInstance());
}

void MainWindow::showCustom() {
    m_navigationController->openCustomEditor(this, m_displayLanguage);
}

void MainWindow::openTestGpio() {
    m_navigationController->openTestGpio(this);
}

void MainWindow::openWifiTransfer() {
    m_navigationController->openWifiTransfer(this);
}

void MainWindow::openWifiConfig() {
    m_navigationController->openWifiSettings(this);
}

void MainWindow::showFolder()
{
    m_navigationController->openFolder(this, m_displayLanguage);
}

void MainWindow::on_receptionFichierButton_clicked()
{
    m_navigationController->openBluetoothReceiver(this);
}

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

void MainWindow::applyCustomShape(QList<QPolygonF> shapes) {


    //qDebug() << "Slot applyCustomShape() appelé dans MainWindow avec" << shapes.size() << "formes.";
    if (shapeVisualization) {
        shapeVisualization->displayCustomShapes(shapes);
        shapeVisualization->setCurrentCustomShapeName("");
    } else {
        //qDebug() << "Erreur : shapeVisualization est nullptr.";
    }
    // ---------- FIN de applyCustomShape ----------
    this->showFullScreen();            // plein écran
}

/* =====================================================================
 *  On sélectionne une forme custom depuis l'inventory
 * ===================================================================*/
void MainWindow::onCustomShapeSelected(const QList<QPolygonF> &polygons,
                                       const QString &name)
{
    /* 1) Empêcher la modification pendant une découpe ---------------- */
    if (shapeVisualization && shapeVisualization->isDecoupeEnCours()) {
        QMessageBox::warning(this,
                             tr("Découpe en cours"),
                             tr("Impossible de modifier la forme pendant la découpe."));
        return;
    }

    /* 2) Passage en mode Custom -------------------------------------- */
    if (!shapeVisualization) {
        this->showFullScreen();
        return;
    }
    shapeVisualization->setCustomMode();

    /* 3) Mettre à l’échelle la zone de découpe ----------------------- */
    const QRectF bounds = combinedBoundingRect(polygons);

    int largeur = ui->Largeur->value();
    int hauteur = ui->Longueur->value();
    if (largeur <= 0)
        largeur = bounds.width()  > 0 ? qRound(bounds.width())  : 100;
    if (hauteur <= 0)
        hauteur = bounds.height() > 0 ? qRound(bounds.height()) : 100;

    shapeVisualization->updateDimensions(largeur, hauteur);
    shapeVisualization->displayCustomShapes(polygons);
    shapeVisualization->setCurrentCustomShapeName(name);

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

void MainWindow::resetDrawing() {
    //qDebug() << "Slot resetDrawing() appelé dans MainWindow ! (Rien à faire car le reset est dans custom)";
}

void MainWindow::updateSpinBoxLongueur(int value) {
    ui->Longueur->setValue(value);
}

void MainWindow::updateSpinBoxLargeur(int value) {
    ui->Largeur->setValue(value);
}

void MainWindow::updateSliderLongueur(int value) {
    ui->Slider_longueur->setValue(value);
}

void MainWindow::updateSliderLargeur(int value) {
    ui->Slider_largeur->setValue(value);
}

void MainWindow::updateShapeCount() {
    int count = ui->shapeCountSpinBox->value();
    int width = ui->Largeur->value();
    int height = ui->Longueur->value();
    ShapeModel::Type type = selectedShapeType;
    shapeVisualization->setShapeCount(count, type, width, height);
}

// Slot pour mettre à jour le label du nombre de formes placées
void MainWindow::updateShapeCountLabel(int count) {
    ui->shapeCountLabel->setText(QString("Formes placées: %1").arg(count));
}

//slot pour spin box espacment
void MainWindow::updateSpacing(int value)
{
    // Vérifier que le widget de visualisation existe
    if (shapeVisualization) {
        // Met à jour l'espacement et redessine
        shapeVisualization->setSpacing(value);
    }
}

void MainWindow::StartPixel()
{
    emit requestStartCut();
}


void MainWindow::updateProgressBar(int percentage, const QString &remainingTimeText) {
    ui->progressBar->setValue(percentage);
    if (ui->timeRemainingLabel) {
        ui->timeRemainingLabel->setText(remainingTimeText);
    }
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

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);       // Traduction automatique des widgets de l'UI
        retranslateDynamicUi();        // Traduction manuelle de ce que tu as créé en C++
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::onShapeSelectedFromInventory(ShapeModel::Type type)
{
    selectedShapeType = type;
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

    shapeVisualization->setPredefinedMode();
    shapeVisualization->setModel(type);
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

ShapeVisualization* MainWindow::getShapeVisualization() const
{
    return shapeVisualization;
}

bool MainWindow::promptAndSaveCurrentCustomShape()
{
    if (!shapeVisualization)
        return false;

    QList<QPolygonF> shapes = shapeVisualization->currentCustomShapes();
    if (shapes.isEmpty())
        return false;

    bool ok = false;
    QString shapeName;
    do {
        shapeName = m_navigationController->promptForName(this,
                                                          tr("Nom de la forme"),
                                                          tr("Entrez un nom pour votre forme :"),
                                                          &ok);
        if (!ok)
            return false;
        if (shapeName.isEmpty())
            continue;
        if (Inventory::getInstance()->shapeNameExists(shapeName)) {
            QMessageBox msg(QMessageBox::Warning,
                            tr("Nom déjà utilisé"),
                            tr("Ce nom est déjà utilisé, veuillez en choisir un autre."),
                            QMessageBox::NoButton,
                            this);
            QTimer::singleShot(2300, &msg, &QMessageBox::accept);
            msg.exec();
            ok = false;
        }
    } while (!ok);

    Inventory::getInstance()->addSavedCustomShape(shapes, shapeName);
    shapeVisualization->setCurrentCustomShapeName(shapeName);
    return true;
}

void MainWindow::openAIImagePromptDialog()
{
    AiGenerationRequest request;
    if (!m_aiServiceManager->openGenerationPrompt(this, request))
        return;

    ui->progressBarAI->setVisible(true);
    ui->progressBarAI->setMinimum(0);
    ui->progressBarAI->setMaximum(0);
    emit requestAiGeneration(request.prompt, request.model, request.quality, request.size, request.colorPrompt);
}



void MainWindow::onCutFinished(bool /*success*/)
{
    ui->progressBar->setValue(0);
}

void MainWindow::onAiGenerationStatus(const QString &msg)
{
    ui->labelAIGenerationStatus->setText(msg);
}

void MainWindow::onAiImageReady(const QString &path)
{
    ui->progressBarAI->setVisible(false);

    AiImageProcessingOptions options;
    QString error;
    if (!m_aiServiceManager->resolveImageProcessingOptions(this, path, options, error)) {
        if (!error.isEmpty())
            QMessageBox::critical(this, tr("Erreur image"), error);
        QTimer::singleShot(3000, this, [this]() { ui->labelAIGenerationStatus->clear(); });
        return;
    }

    ui->labelAIGenerationStatus->clear();
    openImageInCustom(path, options.internalContours, options.colorEdges);
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
