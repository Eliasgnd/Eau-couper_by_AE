#include "MainWindow.h"
#include "ScreenUtils.h"
#include "ShapeModel.h"
#include "qmessagebox.h"
#include "ui_mainwindow.h"
#include "inventaire.h"
#include "custom.h"
#include "Dispositions.h"
#include "FormeVisualization.h"
#include "clavier.h"
#include "TestGpio.h"
#include "BluetoothReceiverDialog.h"
#include "trajetmotor.h"
#include "Language.h"
#include "LogoImporter.h"
#include "AIImagePromptDialog.h"
#include "DossierWidget.h"
#include "ImagePaths.h"
#include "AIImageProcessDialog.h"
#include "WifiTransferWidget.h"
#include "WifiConfigDialog.h"
#include "AspectRatioWrapper.h"

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
#include <QInputDialog>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QRegularExpression>


//Ceci est le Mainwindow,bienvenue dans le mainwindow

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // La barre de titre (index 0) ne prend pas de stretch, la vue (index 1) prend tout, le bas (index 2) reste compact.
    ui->centerVBox->setStretch(0, 0);  // topBarLayout
    ui->centerVBox->setStretch(1, 1);  // horizontalLayout contenant formeVisualizationWidget
    ui->centerVBox->setStretch(2, 0);  // horizontalLayout_2 (commandes + Réglages)
    ui->centerVBox->setStretch(3, 0);  // verticalLayout_2 (vide chez toi)

    // --- AJOUT : facteurs de stretch des 3 colonnes du layout principal ---
    // Étirements des colonnes (pas dans le .ui pour éviter l'erreur uic)
    ui->mainHorizontalLayout->setStretch(0, 0); // colonne gauche (fixe)
    ui->mainHorizontalLayout->setStretch(1, 1); // colonne droite (expansive)

    auto fv = ui->formeVisualizationWidget;   // FormeVisualization* issu du .ui
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
    connect(fv, &FormeVisualization::sheetSizeMmChanged,
            wrapper, [wrapper](const QSizeF& mm) {
                if (mm.width() > 0.0 && mm.height() > 0.0)
                    wrapper->setAspect(mm.width() / mm.height());
            });

    // 1) Crée le menu langue et ses actions (attributs)
    languageMenu  = new QMenu(tr("Langue"), this);
    actionFrench  = languageMenu->addAction(tr("Français"));
    actionEnglish = languageMenu->addAction(tr("Anglais"));
    connect(actionFrench ,  &QAction::triggered, this, &MainWindow::setLanguageFrench);
    connect(actionEnglish,  &QAction::triggered, this, &MainWindow::setLanguageEnglish);

    // Menu principal Paramètres
    settingsMenu = new QMenu(tr("Paramètres"), this);
    actionWifiConfig = settingsMenu->addAction(tr("Configurer le Wi-Fi"));
    settingsMenu->addMenu(languageMenu);
    connect(actionWifiConfig, &QAction::triggered, this, &MainWindow::openWifiConfig);

    // 2) Crée un QToolButton stylé dans le coin droit de la barre de menus
    QToolButton *settingsBtn = new QToolButton(this);
    settingsBtn->setText(tr("Paramètres"));
    settingsBtn->setIcon(QIcon(":/icons/settings.svg"));          // engrenage (ajoute-le à resources.qrc)
    settingsBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    settingsBtn->setPopupMode(QToolButton::InstantPopup);         // clic = ouvre le menu
    settingsBtn->setMenu(settingsMenu);                           // rattache le menu

    // 3) StyleSheet cohérent avec le reste de ton UI
    settingsBtn->setStyleSheet(R"(
    QToolButton {
    background-color: #00BCD4;        /* Cyan primaire */
    color: white;
    font-size: 14px;
    font-weight: bold;
    padding: 6px 14px;
    border: 2px solid #008C9E;
    border-radius: 8px;
    }
    QToolButton::menu-indicator {
    image: url(:/icons/chevron-down-white.svg);  /* petit chevron blanc (ajoute-le au .qrc) */
    subcontrol-position: right center;
    subcontrol-origin: padding;
    padding-left: 6px;                           /* espace entre texte et chevron */
    }
    QToolButton:hover  { background-color: #26C6DA; }  /* plus clair au survol */
    QToolButton:pressed{ background-color: #008C9E; }  /* plus foncé au clic  */
    )");

    // 4) Place le bouton dans le coin supérieur droit de la QMenuBar
    menuBar()->setCornerWidget(settingsBtn, Qt::TopRightCorner);


    // place la fenêtre sur le 2ᵉ écran
    // ScreenUtils::placeOnSecondaryScreen(this);

    // Connection de l'inventaire à la forme
    QObject::connect(Inventaire::getInstance(), &Inventaire::shapeSelected,
                     this, &MainWindow::onShapeSelectedFromInventaire);


    // Initialiser la classe FormeVisualization à partir du widget de l'UI
    formeVisualization = qobject_cast<FormeVisualization*>(ui->formeVisualizationWidget);
    // Création du contrôleur de découpe avant toute connexion
    trajetMotor = new TrajetMotor(formeVisualization, this);
    trajetMotor->setMainWindow(this);

    // Connecter le signal du nombre de formes placées pour mettre à jour le label
    connect(formeVisualization, &FormeVisualization::shapesPlacedCount,
            this, &MainWindow::updateShapeCountLabel);

    // Connecter les spinbox pour les dimensions
    connect(ui->Largeur, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updateForme);
    connect(ui->Longueur, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updateForme);

    // Connecter les boutons pour changer les modèles
    connect(ui->Cercle, &QPushButton::clicked, this, &MainWindow::changeToCircle);
    connect(ui->Rectangle, &QPushButton::clicked, this, &MainWindow::changeToRectangle);
    connect(ui->Triangle, &QPushButton::clicked, this, &MainWindow::changeToTriangle);
    connect(ui->Etoile, &QPushButton::clicked, this, &MainWindow::changeToStar);
    connect(ui->Coeur, &QPushButton::clicked, this, &MainWindow::changeToHeart);

    //connexion pour les formes de l'inventaire
    connect(Inventaire::getInstance(), &Inventaire::customShapeSelected,
            this,                       &MainWindow::onCustomShapeSelected);


    // Naviguer entre les pages
    connect(ui->buttonInventaire, &QPushButton::clicked, this, &MainWindow::showInventaire);
    connect(ui->buttonCustom, &QPushButton::clicked, this, &MainWindow::showCustom);
    // "Images générées" should open the gallery of generated images
    connect(ui->buttonTestGpio, &QPushButton::clicked, this, &MainWindow::showDossier);

    connect(ui->buttonGenerateAI, &QPushButton::clicked, this, &MainWindow::openAIImagePromptDialog);
    // Small button with the download icon opens the GPIO test page
    connect(ui->buttonViewGeneratedImages, &QPushButton::clicked, this, &MainWindow::openTestGpio);
    connect(ui->buttonFileReceiver, &QPushButton::clicked, this, &MainWindow::on_receptionFichierButton_clicked);
    connect(ui->buttonWifiTransfer, &QPushButton::clicked, this, &MainWindow::openWifiTransfer);

    // Connecter les spinboxes aux sliders
    connect(ui->Longueur, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updateSliderLongueur);
    connect(ui->Largeur, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updateSliderLargeur);
    connect(ui->Slider_longueur, &QSlider::valueChanged, this, &MainWindow::updateSpinBoxLongueur);
    connect(ui->Slider_largeur, &QSlider::valueChanged, this, &MainWindow::updateSpinBoxLargeur);

    // Bon : on récupère directement les valeurs actuelles des spinboxes longueur/largeur
    updateSliderLongueur(ui->Longueur->value());
    updateSliderLargeur (ui->Largeur ->value());


    // Connection entre spinbox nombre de formes et FormeVisualization
    connect(ui->shapeCountSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updateShapeCount);
    // Connection bouton optimisation


   // activation et desactivation de l'optimisation

    ui->optimizePlacementButton->setCheckable(true);
    ui->optimizePlacementButton2->setCheckable(true);

    connect(ui->optimizePlacementButton, &QPushButton::clicked, this, [=]() {
        if (ui->optimizePlacementButton->isChecked()) {
            ui->optimizePlacementButton2->setChecked(false);
            formeVisualization->optimizePlacement();
        } else {
            int largeur = ui->Largeur->value();
            int longueur = ui->Longueur->value();
            formeVisualization->updateDimensions(largeur, longueur);

            // Optionnel : logique si on veut désactiver aussi
        }
    });

    connect(ui->optimizePlacementButton2, &QPushButton::clicked, this, [=]() {
        if (ui->optimizePlacementButton2->isChecked()) {
            ui->optimizePlacementButton->setChecked(false);
            formeVisualization->optimizePlacement2();
        } else {
            int largeur = ui->Largeur->value();
            int longueur = ui->Longueur->value();
            formeVisualization->updateDimensions(largeur, longueur);

            // Optionnel
        }
    });







    //spinBox pour l'espacement
    connect(ui->spaceSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::updateSpacing);

    // Dans le constructeur de MainWindow, par exemple après l'initialisation de l'UI :
    connect(formeVisualization, &FormeVisualization::spacingChanged,
            this, [this](int newSpacing) {
                ui->spaceSpinBox->setValue(newSpacing);
            });

    // Déplacement : 1 pixel par clic (dx et dy peuvent être modifiés selon vos préférences)
    connect(ui->ButtonUp, &QPushButton::clicked, this, [this]() {
        formeVisualization->moveSelectedShapes(0, -1); // vers le haut
    });
    connect(ui->ButtonDown, &QPushButton::clicked, this, [this]() {
        formeVisualization->moveSelectedShapes(0, 1);  // vers le bas
    });
    connect(ui->ButtonLeft, &QPushButton::clicked, this, [this]() {
        formeVisualization->moveSelectedShapes(-1, 0); // vers la gauche
    });
    connect(ui->ButtonRight, &QPushButton::clicked, this, [this]() {
        formeVisualization->moveSelectedShapes(1, 0);  // vers la droite
    });

    // Rotation : par exemple, 5° par clic. Vous pouvez ajuster la valeur.
    connect(ui->ButtonRotationLeft, &QPushButton::clicked, this, [this]() {
        formeVisualization->rotateSelectedShapes(-90); // rotation vers la gauche
    });
    connect(ui->ButtonRotationRight, &QPushButton::clicked, this, [this]() {
        formeVisualization->rotateSelectedShapes(90);  // rotation vers la droite
    });

    connect(ui->ButtonAddShape, &QPushButton::clicked, this, [this]() {
        formeVisualization->addShapeBottomRight();
    });

    connect(ui->ButtonDeleteShape, &QPushButton::clicked, this, [this]() {
        formeVisualization->deleteSelectedShapes();
    });

    connect(ui->ButtonSaveLayout, &QPushButton::clicked, this, [this]() {
        auto saveLayout = [this]() {
            bool ok;
            QString name = QInputDialog::getText(this,
                                                tr("Nom de la disposition"),
                                                tr("Entrez un nom"),
                                                QLineEdit::Normal,
                                                "",
                                                &ok);
            if (!ok || name.isEmpty())
                return;
            LayoutData layout = formeVisualization->captureCurrentLayout(name);
            if (formeVisualization->isCustomMode())
                Inventaire::getInstance()->addLayoutToShape(formeVisualization->currentCustomShapeName(), layout);
            else
                Inventaire::getInstance()->addLayoutToBaseShape(selectedShapeType, layout);
        };

        if (formeVisualization->isCustomMode() && formeVisualization->currentCustomShapeName().isEmpty()) {
            QMessageBox::warning(this,
                                 tr("Disposition"),
                                 tr("Forme non sauvegardée — veuillez la sauvegarder d'abord."));
            if (promptAndSaveCurrentCustomShape())
                saveLayout();
            return;
        }

        saveLayout();
    });

    // Connecter bouton start a la detection des pixel noirs puis le controle des moteur en fonction
    connect(ui->Play, &QPushButton::clicked, this, &MainWindow::StartPixel);
    connect(formeVisualization, &FormeVisualization::optimizationStateChanged, this,
            [this](bool optimized) {
                if (!optimized) {
                    ui->optimizePlacementButton->setChecked(false);
                    ui->optimizePlacementButton2->setChecked(false);
                }
            });


    // Pause ↔ Reprendre
    connect(ui->Pause, &QPushButton::clicked, this, [this]() {
        static bool paused = false;
        if (!paused) {
            trajetMotor->pause();
        } else {
            trajetMotor->resume();
        }
        paused = !paused;
    });

    // Stop
    connect(ui->Stop, &QPushButton::clicked, this, [this]() {
        trajetMotor->stopCut();
        ui->progressBar->setValue(0);
    });

    // **NOUVELLE CONNEXION** pour la barre de progression de la découpe
    connect(trajetMotor, &TrajetMotor::decoupeProgress,
            this, &MainWindow::updateProgressBar);

    connect(ui->Stop, &QPushButton::clicked, this, [this]() {
        trajetMotor->stopCut();
        formeVisualization->setDecoupeEnCours(false);
        setSpinboxSliderEnabled(true);
    });



    // Configuration de la barre de progression
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(0);
    ui->progressBar->setFormat("%p%");
    ui->progressBar->setAlignment(Qt::AlignCenter);


    if (ui->timeRemainingLabel)
        ui->timeRemainingLabel->setText(tr("Temps restant estim\u00e9 : 0s"));

}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::updateForme() {
    int largeur = ui->Largeur->value();
    int longueur = ui->Longueur->value();
    formeVisualization->updateDimensions(largeur, longueur);
}

void MainWindow::changeToCircle() {
    //qDebug() << "Changement de forme: Cercle";
    selectedShapeType = ShapeModel::Type::Circle;
    formeVisualization->setPredefinedMode();
    formeVisualization->setModel(ShapeModel::Type::Circle);
}

void MainWindow::changeToRectangle() {
    //qDebug() << "Changement de forme: Rectangle";
    selectedShapeType = ShapeModel::Type::Rectangle;
    formeVisualization->setPredefinedMode();
    formeVisualization->setModel(ShapeModel::Type::Rectangle);
}

void MainWindow::changeToTriangle() {
    //qDebug() << "Changement de forme: Triangle";
    selectedShapeType = ShapeModel::Type::Triangle;
    formeVisualization->setPredefinedMode();
    formeVisualization->setModel(ShapeModel::Type::Triangle);
}

void MainWindow::changeToStar() {
    //qDebug() << "Changement de forme: Étoile";
    selectedShapeType = ShapeModel::Type::Star;
    formeVisualization->setPredefinedMode();
    formeVisualization->setModel(ShapeModel::Type::Star);
}

void MainWindow::changeToHeart() {
    //qDebug() << "Changement de forme: Cœur";
    selectedShapeType = ShapeModel::Type::Heart;
    formeVisualization->setPredefinedMode();
    formeVisualization->setModel(ShapeModel::Type::Heart);
}

void MainWindow::showInventaire() {
    this->hide();
    Inventaire::getInstance()->showFullScreen();
}

void MainWindow::showCustom() {
    this->hide();
    custom *customWindow = new custom(currentLanguage);

    connect(customWindow, &custom::applyCustomShapeSignal,
            this, &MainWindow::applyCustomShape);
    connect(customWindow, &custom::resetDrawingSignal,
            this, &MainWindow::resetDrawing);

    customWindow->showFullScreen();
}

void MainWindow::openTestGpio() {
    this->hide();
    TestGpio *test = new TestGpio();
    test->showFullScreen();
}

void MainWindow::openWifiTransfer() {
    this->hide();
    WifiTransferWidget *w = new WifiTransferWidget();
    w->showFullScreen();
}

void MainWindow::openWifiConfig() {
    this->hide();
    WifiConfigDialog *dlg = new WifiConfigDialog();
    dlg->showFullScreen();
}

void MainWindow::showDossier()
{
    this->hide();
    DossierWidget *page = new DossierWidget(currentLanguage);
    page->showFullScreen();
}

void MainWindow::on_receptionFichierButton_clicked()
{
    this->hide();
    BluetoothReceiverDialog *page = new BluetoothReceiverDialog();
    page->showFullScreen();
}

void MainWindow::openImageInCustom(const QString &filePath,
                                   bool internalContours,
                                   bool colorEdges)
{
    this->hide();

    QPainterPath outline;
    if (colorEdges) {
        ImageEdgeImporter edgeImporter;
        if (!edgeImporter.loadAndProcess(filePath, outline))
            return;
    } else {
        LogoImporter importer;
        outline = importer.importLogo(filePath, internalContours, 128);
        if (outline.isEmpty())
            return;
    }

    custom *cw = new custom(currentLanguage);

    connect(cw, &custom::applyCustomShapeSignal, this, &MainWindow::applyCustomShape);
    connect(cw, &custom::resetDrawingSignal,     this, &MainWindow::resetDrawing);

    CustomDrawArea *area = cw->getDrawArea();
    cw->showFullScreen();   // ← important : la taille de drawArea sera correcte à la prochaine itération d'event loop

    // ⚠️ On décale le centrage à "après mise en page"
    QTimer::singleShot(0, cw, [area, outline]() mutable {
        if (!area) return;

        // --- fit & center simple (sans marge) ---
        QRectF br = outline.boundingRect();
        if (br.isEmpty()) return;

        const double maxDim = std::max(br.width(), br.height());
        if (maxDim <= 0.0) return;

        // Ici, on garde ton idée d'une taille cible, mais basée sur le drawArea réel
        const double target = 0.8; // occupe ~80% du côté le plus court
        const double w = std::max(1, area->width());
        const double h = std::max(1, area->height());
        const double scale = target * std::min(w, h) / maxDim;

        QTransform T;
        T.translate(-br.x(), -br.y());
        T.scale(scale, scale);
        QPainterPath scaled = T.map(outline);

        QRectF sb = scaled.boundingRect();
        QPointF center(w / 2.0, h / 2.0);
        scaled.translate(center - sb.center());

        const QList<QPainterPath> subs = CustomDrawArea::separateIntoSubpaths(scaled);
        for (const QPainterPath &sp : subs)
            area->addImportedLogoSubpath(sp);
    });
}

void MainWindow::applyCustomShape(QList<QPolygonF> shapes) {


    //qDebug() << "Slot applyCustomShape() appelé dans MainWindow avec" << shapes.size() << "formes.";
    if (formeVisualization) {
        formeVisualization->displayCustomShapes(shapes);
        formeVisualization->setCurrentCustomShapeName("");
    } else {
        //qDebug() << "Erreur : formeVisualization est nullptr.";
    }
    // ---------- FIN de applyCustomShape ----------
    this->showFullScreen();            // plein écran
}

/* =====================================================================
 *  On sélectionne une forme custom depuis l'inventaire
 * ===================================================================*/
void MainWindow::onCustomShapeSelected(const QList<QPolygonF> &polygons,
                                       const QString &name)
{
    /* 1) Empêcher la modification pendant une découpe ---------------- */
    if (formeVisualization && formeVisualization->isDecoupeEnCours()) {
        QMessageBox::warning(this,
                             tr("Découpe en cours"),
                             tr("Impossible de modifier la forme pendant la découpe."));
        return;
    }

    /* 2) Passage en mode Custom -------------------------------------- */
    if (!formeVisualization) {
        this->showFullScreen();
        return;
    }
    formeVisualization->setCustomMode();

    /* 3) Mettre à l’échelle la zone de découpe ----------------------- */
    QPainterPath combinedPath;
    for (const QPolygonF &poly : polygons)
        combinedPath.addPolygon(poly);
    QRectF bounds = combinedPath.boundingRect();

    int largeur = ui->Largeur->value();
    int hauteur = ui->Longueur->value();
    if (largeur <= 0)
        largeur = bounds.width()  > 0 ? qRound(bounds.width())  : 100;
    if (hauteur <= 0)
        hauteur = bounds.height() > 0 ? qRound(bounds.height()) : 100;

    formeVisualization->updateDimensions(largeur, hauteur);
    formeVisualization->displayCustomShapes(polygons);
    formeVisualization->setCurrentCustomShapeName(name);

    /* 4) Si des dispositions existent, ouvrir la fenêtre Dispositions */
    QList<LayoutData> layouts = Inventaire::getInstance()->getLayoutsForShape(name);
    if (!layouts.isEmpty()) {
        Dispositions *disp = new Dispositions(name, layouts,
                                              polygons, currentLanguage);

        connect(disp, &Dispositions::layoutSelected, this,
                [this](const LayoutData &ld) {
                    formeVisualization->applyLayout(ld);

                    // verrouillage/déverrouillage propre des widgets
                    const bool block = true;
                    ui->Largeur->blockSignals(block);
                    ui->Longueur->blockSignals(block);
                    ui->shapeCountSpinBox->blockSignals(block);
                    ui->spaceSpinBox->blockSignals(block);
                    ui->Slider_largeur->blockSignals(block);
                    ui->Slider_longueur->blockSignals(block);

                    ui->Largeur->setValue(ld.largeur);
                    ui->Longueur->setValue(ld.longueur);
                    ui->Slider_largeur->setValue(ld.largeur);
                    ui->Slider_longueur->setValue(ld.longueur);
                    ui->shapeCountSpinBox->setValue(ld.items.size());
                    ui->spaceSpinBox->setValue(ld.spacing);

                    ui->Largeur->blockSignals(!block);
                    ui->Longueur->blockSignals(!block);
                    ui->shapeCountSpinBox->blockSignals(!block);
                    ui->spaceSpinBox->blockSignals(!block);
                    ui->Slider_largeur->blockSignals(!block);
                    ui->Slider_longueur->blockSignals(!block);
                });

        /* signaux de fermeture/retour */
        connect(disp, &Dispositions::shapeOnlySelected, this, [](){});
        connect(disp, &Dispositions::closed, this, [this, disp]() {
            this->showFullScreen();
            disp->deleteLater();
        });
        connect(disp, &Dispositions::requestOpenInventaire, this,
                [this, disp]() {
                    disp->deleteLater();
                    Inventaire::getInstance()->showFullScreen();
                });

        this->hide();
        disp->showFullScreen();
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
    formeVisualization->setShapeCount(count, type, width, height);
}

// Slot pour mettre à jour le label du nombre de formes placées
void MainWindow::updateShapeCountLabel(int count) {
    ui->shapeCountLabel->setText(QString("Formes placées: %1").arg(count));
}

//slot pour spin box espacment
void MainWindow::updateSpacing(int value)
{
    // Vérifier que le widget de visualisation existe
    if (formeVisualization) {
        // Met à jour l'espacement et redessine
        formeVisualization->setSpacing(value);
    }
}

// Par exemple, dans votre slot ou méthode :
void afficherClavier() {
    Clavier clavier;        // Création d'une instance du clavier
    if (clavier.exec() == QDialog::Accepted) {  // Affichage modal du clavier
        QString texteSaisi = clavier.getText();
        // Traitez le texte saisi selon vos besoins
        //qDebug() << "Texte saisi :" << texteSaisi;
    }
}

void MainWindow::StartPixel()
{
    if (trajetMotor->isPaused()) {
        // La découpe existe déjà mais est en pause → on la reprend
        trajetMotor->resume();
        return;
    }

    formeVisualization->resetAllShapeColors();

    if (!formeVisualization->validateShapes()) {
        QMessageBox::warning(this, tr("Formes invalides"),
                             tr("Certaines formes dépassent la zone ou se chevauchent."));
        return;
    }

    formeVisualization->setDecoupeEnCours(true);
    formeVisualization->resetAllShapeColors();

    // Sinon, aucune découpe en cours → on en (re)lance une nouvelle
    //qDebug() << "Demarrage Découpe";
    setSpinboxSliderEnabled(false);
    trajetMotor->executeTrajet();
    // Blocage des paramètres UI pendant la découpe

}


void MainWindow::updateProgressBar(int remaining, int total) {
    if (total == 0) return;
    int percent = (total - remaining) * 100 / total;

    // Démarrage du chrono à la première mise à jour
    if (!decoupeTimer.isValid() || remaining == total) {
        decoupeTimer.start();
        smoothedTotalMs = -1.0; // réinitialise l'estimation
    }

    // Mise à jour de la barre
    ui->progressBar->setValue(percent);

    // Calcul de l'estimation du temps restant
    QString timeText;
    if (percent > 0 && remaining > 0) {
        qint64 elapsedMs      = decoupeTimer.elapsed();
        double instantTotalMs = static_cast<double>(elapsedMs) / (percent / 100.0);
        if (smoothedTotalMs < 0)
            smoothedTotalMs = instantTotalMs;
        else
            smoothedTotalMs = 0.9 * smoothedTotalMs + 0.1 * instantTotalMs;
        qint64 remainingMs = static_cast<qint64>(smoothedTotalMs - elapsedMs);
        if (remainingMs < 0) remainingMs = 0;
        int seconds = remainingMs / 1000;
        int minutes = seconds / 60;
        seconds %= 60;
        if (minutes > 0)
            timeText = tr("Temps restant estim\u00e9 : %1m %2s").arg(minutes).arg(seconds);
        else
            timeText = tr("Temps restant estim\u00e9 : %1s").arg(seconds);
    } else {
        timeText = tr("Temps restant estim\u00e9 : 0s");
    }
    if (ui->timeRemainingLabel)
        ui->timeRemainingLabel->setText(timeText);

    // Remise à zéro à la fin
    if (remaining == 0) {
        decoupeTimer.invalidate();
        smoothedTotalMs = -1.0;
    }
}


MainWindow* MainWindow::getInstance()
{
    static MainWindow instance; // L'instance statique sera créée une seule fois
    return &instance;
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
    loadLanguage(Language::French);
}

void MainWindow::setLanguageEnglish()
{
    loadLanguage(Language::English);
}

bool MainWindow::loadLanguage(Language lang)
{
    qApp->removeTranslator(&translator);
    currentLanguage = lang;

    const QString locale = (lang == Language::French) ? "fr" : "en";
    const QString path = qApp->applicationDirPath()
                         + "/translations/machineDecoupeIHM_" + locale + ".qm";

    if (!translator.load(path)) {
        qWarning() << "❌ Impossible de charger la langue :" << path;
    } else {
        qApp->installTranslator(&translator);
        //qDebug() << "✅ Langue chargée :" << path;
    }

    // Mettre à jour tous les textes
    QEvent event(QEvent::LanguageChange);
    QCoreApplication::sendEvent(this, &event);  // Déclenche changeEvent
    retranslateDynamicUi();                     // Retraduit menus + labels créés dynamiquement

    return true;
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

void MainWindow::onShapeSelectedFromInventaire(ShapeModel::Type type)
{
    selectedShapeType = type;
    QList<LayoutData> layouts = Inventaire::getInstance()->getLayoutsForBaseShape(type);
    if (!layouts.isEmpty()) {
        QList<QPolygonF> polys = ShapeModel::shapePolygons(type, 100, 100);
        QString name = Inventaire::baseShapeName(type, currentLanguage);
        Dispositions *disp = new Dispositions(name, layouts, polys, currentLanguage, true, type);

        connect(disp, &Dispositions::layoutSelected, this, [this, type](const LayoutData &ld){
            QList<QPolygonF> shapePolys = ShapeModel::shapePolygons(type, 100, 100);
            formeVisualization->setCustomMode();
            formeVisualization->displayCustomShapes(shapePolys);
            formeVisualization->setCurrentCustomShapeName(Inventaire::baseShapeName(type, Language::French));
            formeVisualization->applyLayout(ld);

            ui->Largeur->blockSignals(true);
            ui->Longueur->blockSignals(true);
            ui->shapeCountSpinBox->blockSignals(true);
            ui->spaceSpinBox->blockSignals(true);
            ui->Slider_largeur->blockSignals(true);
            ui->Slider_longueur->blockSignals(true);

            ui->Largeur->setValue(ld.largeur);
            ui->Longueur->setValue(ld.longueur);
            ui->Slider_largeur->setValue(ld.largeur);
            ui->Slider_longueur->setValue(ld.longueur);
            ui->shapeCountSpinBox->setValue(ld.items.size());
            ui->spaceSpinBox->setValue(ld.spacing);

            ui->Largeur->blockSignals(false);
            ui->Longueur->blockSignals(false);
            ui->shapeCountSpinBox->blockSignals(false);
            ui->spaceSpinBox->blockSignals(false);
            ui->Slider_largeur->blockSignals(false);
            ui->Slider_longueur->blockSignals(false);
        });

        connect(disp, &Dispositions::shapeOnlySelected, this, [this, type]() {
            selectedShapeType = type;
            formeVisualization->setPredefinedMode();
            formeVisualization->setModel(type);
        });

        connect(disp, &Dispositions::closed, this, [this, disp]() {
            this->showFullScreen();
            disp->deleteLater();
        });

        connect(disp, &Dispositions::requestOpenInventaire, this, [this, disp]() {
            disp->deleteLater();
            Inventaire::getInstance()->showFullScreen();
        });

        this->hide();
        disp->showFullScreen();
        return;
    }

    formeVisualization->setPredefinedMode();
    formeVisualization->setModel(type);
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

FormeVisualization* MainWindow::getFormeVisualization() const
{
    return formeVisualization;
}

bool MainWindow::promptAndSaveCurrentCustomShape()
{
    if (!formeVisualization)
        return false;

    QList<QPolygonF> shapes = formeVisualization->currentCustomShapes();
    if (shapes.isEmpty())
        return false;

    bool ok = false;
    QString shapeName;
    do {
        shapeName = QInputDialog::getText(this,
                                          tr("Nom de la forme"),
                                          tr("Entrez un nom pour votre forme :"),
                                          QLineEdit::Normal,
                                          "",
                                          &ok);
        if (!ok)
            return false;
        if (shapeName.isEmpty())
            continue;
        if (Inventaire::getInstance()->shapeNameExists(shapeName)) {
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

    Inventaire::getInstance()->addSavedCustomShape(shapes, shapeName);
    formeVisualization->setCurrentCustomShapeName(shapeName);
    return true;
}

void MainWindow::openAIImagePromptDialog()
{
    AIImagePromptDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        generateAIImage(dlg.getPrompt(), dlg.getModel(), dlg.getQuality(), dlg.getSize(), dlg.isColor());
    }
}

void MainWindow::generateAIImage(const QString &userPrompt,
                                 const QString &model,
                                 const QString &quality,
                                 const QString &size,
                                 bool colorPrompt)
{
    if (userPrompt.isEmpty())
        return;

    QString finalPrompt;
    if (colorPrompt) {
        finalPrompt =
            "A minimal flat icon of a " + userPrompt +
            ", centered, on a white background. Clean shapes, no outlines, no text, no decorations, no shadow, no background. Suitable for icon design or sticker. Simple, geometric, and immediately recognizable.";
    } else {
        QString startPrompt = "A single black outline drawing of a ";
        QString styleSuffix =
            ", only the outer edge, no internal lines, no doors, no windows, no shading, no textures, white background, vector style, icon-like, extremely minimal";
        finalPrompt = startPrompt + userPrompt + styleSuffix;
    }
    qDebug() << "[AI] Prompt final :" << finalPrompt;

    if (!m_netManager)
        m_netManager = new QNetworkAccessManager(this);

    ui->labelAIGenerationStatus->setText("🧠 Génération en cours...");
    ui->progressBarAI->setVisible(true);
    ui->progressBarAI->setMinimum(0);
    ui->progressBarAI->setMaximum(0);

    // ✅ Modèle forcé : DALL-E 3
    QString modelStr   = "dall-e-3";
    QString qualityStr = "standard";      // ou "hd" selon ce que tu veux forcer
    QString sizeStr    = size;            // tu peux aussi fixer ici "1024x1024"

    // ✅ Estimation du prix uniquement pour DALL-E 3
    double price = 0.0;
    if (qualityStr == "standard") price = (sizeStr == "1024x1024") ? 0.04 : 0.08;
    else if (qualityStr == "hd")  price = (sizeStr == "1024x1024") ? 0.08 : 0.12;
    qDebug() << "[AI] Coût estimé de l'image : $" << price;

    QNetworkRequest req(QUrl("https://api.openai.com/v1/images/generations"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QByteArray apiKey = qEnvironmentVariable("OPENAI_API_KEY").toUtf8();
    if (apiKey.isEmpty()) {
        QMessageBox::critical(this, "Erreur API", "La clé API OpenAI est absente.\nDéfinissez OPENAI_API_KEY.");
        ui->labelAIGenerationStatus->setText("❌ Clé API manquante");
        ui->progressBarAI->setVisible(false);
        return;
    }
    req.setRawHeader("Authorization", "Bearer " + apiKey);

    QJsonObject body{
        {"model", modelStr},
        {"prompt", finalPrompt},
        {"n", 1},
        {"size", sizeStr},
        {"quality", qualityStr}  // ✅ toujours présent avec DALL-E 3
    };

    qDebug() << "[AI] Envoi de la requête avec modèle:" << modelStr << ", taille:" << sizeStr << ", qualité:" << qualityStr;

    QNetworkReply *reply = m_netManager->post(req, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply, userPrompt]() {
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "[AI] ❌ Erreur API :" << reply->errorString();
            ui->labelAIGenerationStatus->setText("❌ Erreur API");
            ui->progressBarAI->setVisible(false);
            QTimer::singleShot(3000, this, [this]() {
                ui->labelAIGenerationStatus->clear();
            });
            reply->deleteLater();
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QString url = doc["data"].toArray().first().toObject()["url"].toString();
        reply->deleteLater();

        qDebug() << "[AI] ✅ Image URL :" << url;

        QNetworkReply *imgReply = m_netManager->get(QNetworkRequest(QUrl(url)));
        connect(imgReply, &QNetworkReply::finished, this, [this, imgReply, userPrompt]() {
            ui->progressBarAI->setVisible(false);

            if (imgReply->error() != QNetworkReply::NoError) {
                qWarning() << "[AI] ❌ Échec téléchargement image :" << imgReply->errorString();
                ui->labelAIGenerationStatus->setText("❌ Erreur image");
                QTimer::singleShot(3000, this, [this]() {
                    ui->labelAIGenerationStatus->clear();
                });
                imgReply->deleteLater();
                return;
            }

            QImage img;
            img.loadFromData(imgReply->readAll());
            imgReply->deleteLater();

            if (img.isNull()) {
                qWarning() << "[AI] ❌ Image invalide.";
                ui->labelAIGenerationStatus->setText("❌ Image invalide");
                QTimer::singleShot(3000, this, [this]() {
                    ui->labelAIGenerationStatus->clear();
                });
                return;
            }

            // Archive image in global IA directory and use that path for further processing
            const QString imagesDirPath = ImagePaths::iaDir();
            QDir imagesDir(imagesDirPath);
            QString sanitized = userPrompt.normalized(QString::NormalizationForm_D);
            sanitized.remove(QRegularExpression("[\\p{Mn}]"));
            sanitized.replace(QRegularExpression("[^A-Za-z0-9]+"), "_");
            sanitized = sanitized.trimmed();
            if (sanitized.isEmpty())
                sanitized = "image";
            const QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm");
            QString archiveFile = imagesDir.filePath(timestamp + '_' + sanitized + ".png");
            if (!img.save(archiveFile))
                qWarning() << "[AI] ❌ Impossible d'enregistrer l'image IA";
            else
                qDebug() << "[AI] 💾 Image archivée dans :" << archiveFile;

            AIImageProcessDialog dlg(img);
            if (dlg.exec() != QDialog::Accepted) {
                ui->labelAIGenerationStatus->clear();
                return;
            }

            bool internal = false;
            bool color    = false;
            if (dlg.selectedMethod() == AIImageProcessDialog::LogoWithInternal)
                internal = true;
            else if (dlg.selectedMethod() == AIImageProcessDialog::ColorEdges)
                color = true;

            ui->labelAIGenerationStatus->clear();
            openImageInCustom(archiveFile, internal, color);
        });
    });
}
