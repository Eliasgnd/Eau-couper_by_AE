#include "MainWindow.h"
#include "ScreenUtils.h"
#include "ShapeModel.h"
#include "ui_mainwindow.h"
#include "inventaire.h"
#include "custom.h"
#include "LayoutSelector.h"
#include "FormeVisualization.h"
#include "clavier.h"
#include "trajetmotor.h"
#include "Language.h"


#include <QSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QPixmap>
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



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //qDebug() << "Titre du bouton Play =" << ui->Play->text();

    // 1) Crée le menu langue et ses actions (attributs)
    languageMenu  = new QMenu(tr("Langue"), this);
    actionFrench  = languageMenu->addAction(tr("Français"));
    actionEnglish = languageMenu->addAction(tr("Anglais"));
    connect(actionFrench ,  &QAction::triggered, this, &MainWindow::setLanguageFrench);
    connect(actionEnglish,  &QAction::triggered, this, &MainWindow::setLanguageEnglish);

    // 2) Crée un QToolButton stylé dans le coin droit de la barre de menus
    QToolButton *settingsBtn = new QToolButton(this);
    settingsBtn->setText(tr("Paramètres"));
    settingsBtn->setIcon(QIcon(":/icons/settings.svg"));          // engrenage (ajoute-le à resources.qrc)
    settingsBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    settingsBtn->setPopupMode(QToolButton::InstantPopup);         // clic = ouvre le menu
    settingsBtn->setMenu(languageMenu);                           // rattache le menu

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
    ScreenUtils::placeOnSecondaryScreen(this);

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
        if (!formeVisualization->isCustomMode() || formeVisualization->currentCustomShapeName().isEmpty()) {
            QMessageBox::warning(this, tr("Disposition"), tr("Sauvegardez d'abord la forme."));
            return;
        }
        bool ok;
        QString name = QInputDialog::getText(this, tr("Nom de la disposition"), tr("Entrez un nom"), QLineEdit::Normal, "", &ok);
        if (!ok || name.isEmpty())
            return;
        LayoutData layout = formeVisualization->captureCurrentLayout(name);
        Inventaire::getInstance()->addLayoutToShape(formeVisualization->currentCustomShapeName(), layout);
    });

    // Connecter bouton start a la detection des pixel noirs puis le controle des moteur en fonction
    connect(ui->Play, &QPushButton::clicked, this, &MainWindow::StartPixel);
    connect(formeVisualization, &FormeVisualization::optimizationStateChanged, this,
            [](bool /*optimized*/) {});


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

void MainWindow::applyCustomShape(QList<QPolygonF> shapes) {

    //qDebug() << "Slot applyCustomShape() appelé dans MainWindow avec" << shapes.size() << "formes.";
    if (formeVisualization) {
        formeVisualization->displayCustomShapes(shapes);
        formeVisualization->setCurrentCustomShapeName("");
    } else {
        //qDebug() << "Erreur : formeVisualization est nullptr.";
    }
    // Blocage des modifications pendant la découpe personnalisée

    formeVisualization->setDecoupeEnCours(true);
    this->showFullScreen();
}

void MainWindow::onCustomShapeSelected(const QList<QPolygonF> &polygons, const QString &name)
{
    if (formeVisualization) {
        // Forcer le mode custom
        formeVisualization->setCustomMode();

        // Combiner les polygones pour obtenir le rectangle englobant
        QPainterPath combinedPath;
        for (const QPolygonF &poly : polygons) {
            combinedPath.addPolygon(poly);
        }
        QRectF bounds = combinedPath.boundingRect();

        // Dimensions basées sur les valeurs actuelles des spin box
        int largeur = ui->Largeur->value();
        int hauteur = ui->Longueur->value();

        // Valeurs par défaut si les spin box n'ont pas encore été définies
        if (largeur <= 0)
            largeur = (bounds.width()  > 0) ? qRound(bounds.width())  : 100;
        if (hauteur <= 0)
            hauteur = (bounds.height() > 0) ? qRound(bounds.height()) : 100;

        // Mise à jour du widget de visualisation uniquement
        formeVisualization->updateDimensions(largeur, hauteur);
        formeVisualization->displayCustomShapes(polygons);
        formeVisualization->setCurrentCustomShapeName(name);

        QList<LayoutData> layouts = Inventaire::getInstance()->getLayoutsForShape(name);
        if (!layouts.isEmpty()) {
            LayoutSelector selector(layouts, polygons, currentLanguage, this);
            if (selector.exec() == QDialog::Accepted && selector.hasSelection()) {
                LayoutData ld = selector.selectedLayout();
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
            }
        }
    }
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
    if (screens.size() > 1) {
        QScreen* second = screens.at(0);
        // Attribuer le QWindow natif à l'écran secondaire
        if (auto win = this->windowHandle()) {
            win->setScreen(second);
        }
        // Passer en plein écran
        this->showFullScreen();
    }

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
