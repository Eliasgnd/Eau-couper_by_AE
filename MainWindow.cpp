#include "MainWindow.h"
#include "ScreenUtils.h"
#include "ShapeModel.h"
#include "ui_mainwindow.h"
#include "inventaire.h"
#include "custom.h"
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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qDebug() << "Titre du bouton Play =" << ui->Play->text();
    // Menu Paramètres avec la sélection de langue
    settingsMenu = menuBar()->addMenu(tr("Paramètres"));
    languageMenu = settingsMenu->addMenu(tr("Langue"));
    actionFrench = languageMenu->addAction(tr("Français"));
    actionEnglish = languageMenu->addAction(tr("Anglais"));
    connect(actionFrench, &QAction::triggered, this, &MainWindow::setLanguageFrench);
    connect(actionEnglish, &QAction::triggered, this, &MainWindow::setLanguageEnglish);
    connect(ui->buttonSettings, &QPushButton::clicked, this, &MainWindow::showLanguageMenu);
    updateTranslations();
    // place la fenêtre sur le 2ᵉ écran
    ScreenUtils::placeOnSecondaryScreen(this);

    // Connection de l'inventaire à la forme
    QObject::connect(Inventaire::getInstance(), &Inventaire::shapeSelected,
                     ui->formeVisualizationWidget, &FormeVisualization::setModel);

    // Initialiser la classe FormeVisualization à partir du widget de l'UI
    formeVisualization = qobject_cast<FormeVisualization*>(ui->formeVisualizationWidget);

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
    connect(ui->optimizePlacementButton, &QPushButton::clicked, formeVisualization, &FormeVisualization::optimizePlacement);
    connect(ui->optimizePlacementButton2, &QPushButton::clicked, formeVisualization, &FormeVisualization::optimizePlacement2);

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

    // Connecter bouton start a la detection des pixel noirs puis le controle des moteur en fonction
    connect(ui->Play, &QPushButton::clicked, this, &MainWindow::StartPixel);
    connect(formeVisualization, &FormeVisualization::optimizationStateChanged, this,
            [this](bool /*optimized*/) {
                trajetMotor = new TrajetMotor(formeVisualization, this);
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
    });

    // **NOUVELLE CONNEXION** pour la barre de progression de la découpe
    connect(trajetMotor, &TrajetMotor::decoupeProgress,
            this, &MainWindow::updateProgressBar);

    // Configuration de la barre de progression
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(0);
    ui->progressBar->setFormat("%p%%");
    ui->progressBar->setAlignment(Qt::AlignCenter);



    trajetMotor = new TrajetMotor(formeVisualization, this);

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
    qDebug() << "Changement de forme: Cercle";
    selectedShapeType = ShapeModel::Type::Circle;
    formeVisualization->setPredefinedMode();
    formeVisualization->setModel(ShapeModel::Type::Circle);
}

void MainWindow::changeToRectangle() {
    qDebug() << "Changement de forme: Rectangle";
    selectedShapeType = ShapeModel::Type::Rectangle;
    formeVisualization->setPredefinedMode();
    formeVisualization->setModel(ShapeModel::Type::Rectangle);
}

void MainWindow::changeToTriangle() {
    qDebug() << "Changement de forme: Triangle";
    selectedShapeType = ShapeModel::Type::Triangle;
    formeVisualization->setPredefinedMode();
    formeVisualization->setModel(ShapeModel::Type::Triangle);
}

void MainWindow::changeToStar() {
    qDebug() << "Changement de forme: Étoile";
    selectedShapeType = ShapeModel::Type::Star;
    formeVisualization->setPredefinedMode();
    formeVisualization->setModel(ShapeModel::Type::Star);
}

void MainWindow::changeToHeart() {
    qDebug() << "Changement de forme: Cœur";
    selectedShapeType = ShapeModel::Type::Heart;
    formeVisualization->setPredefinedMode();
    formeVisualization->setModel(ShapeModel::Type::Heart);
}

void MainWindow::showInventaire() {
    this->hide();
    Inventaire::getInstance()->updateTranslations(currentLanguage);
    Inventaire::getInstance()->show();
}

void MainWindow::showCustom() {
    this->hide();
    custom *customWindow = new custom(currentLanguage);
    connect(customWindow, &custom::applyCustomShapeSignal,
            this, &MainWindow::applyCustomShape);
    connect(customWindow, &custom::resetDrawingSignal,
            this, &MainWindow::resetDrawing);
    customWindow->show();
}

void MainWindow::applyCustomShape(QList<QPolygonF> shapes) {
    qDebug() << "Slot applyCustomShape() appelé dans MainWindow avec" << shapes.size() << "formes.";
    if (formeVisualization) {
        formeVisualization->displayCustomShapes(shapes);
    } else {
        qDebug() << "Erreur : formeVisualization est nullptr.";
    }
    this->show();
}

void MainWindow::onCustomShapeSelected(const QList<QPolygonF> &polygons)
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

        // Calcul des dimensions pour la vue (mais on n'update plus les spin boxes)
        int largeur = (bounds.width() > 0) ? qRound(bounds.width()) : 100;
        int hauteur = (bounds.height() > 0) ? qRound(bounds.height()) : 100;

        // Mise à jour du widget de visualisation uniquement
        formeVisualization->updateDimensions(largeur, hauteur);
        formeVisualization->displayCustomShapes(polygons);
    }
    this->show();
}

void MainWindow::resetDrawing() {
    qDebug() << "Slot resetDrawing() appelé dans MainWindow ! (Rien à faire car le reset est dans custom)";
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
        qDebug() << "Texte saisi :" << texteSaisi;
    }
}

void MainWindow::StartPixel()
{
    if (trajetMotor->isPaused()) {
        // La découpe existe déjà mais est en pause → on la reprend
        trajetMotor->resume();
        return;
    }

    // Sinon, aucune découpe en cours → on en (re)lance une nouvelle
    //qDebug() << "Demarrage Découpe";
    trajetMotor->executeTrajet();
}


void MainWindow::updateProgressBar(int remaining, int total) {
    if (total == 0) return;
    int percent = 100 - (remaining * 100 / total);
    ui->progressBar->setValue(percent);
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
        qDebug() << "Écran" << i
                 << "nom =" << s->name()
                 << "géométrie =" << s->geometry()
                 << "disponible =" << s->availableGeometry();
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
    currentLanguage = Language::French;
    updateTranslations();
    Inventaire::getInstance()->updateTranslations(currentLanguage);
}

void MainWindow::setLanguageEnglish()
{
    currentLanguage = Language::English;
    updateTranslations();
    Inventaire::getInstance()->updateTranslations(currentLanguage);
}

void MainWindow::showLanguageMenu()
{
    if (languageMenu && ui->buttonSettings) {
        languageMenu->exec(ui->buttonSettings->mapToGlobal(QPoint(0, ui->buttonSettings->height())));
    }
}

void MainWindow::updateTranslations()
{
    if (currentLanguage == Language::French) {
        if (settingsMenu) settingsMenu->setTitle("Paramètres");
        if (languageMenu) languageMenu->setTitle("Langue");
        if (actionFrench) actionFrench->setText("Français");
        if (actionEnglish) actionEnglish->setText("Anglais");
        ui->Cercle->setText("Cercle");
        ui->Rectangle->setText("Rectangle");
        ui->Triangle->setText("Triangle");
        ui->Coeur->setText("Coeur");
        ui->Etoile->setText("Etoile");
        ui->optimizePlacementButton->setText("Optimiser placement");
        ui->optimizePlacementButton2->setText("Optimiser placement 2");
        ui->buttonInventaire->setText("Inventaire");
        ui->buttonCustom->setText("Custom");
        ui->buttonFileReceiver->setText("Réception fichier");
        if (ui->buttonSettings) ui->buttonSettings->setText("Paramètres");
        ui->Vitesse_txt->setText("Vitesse : ");
        ui->Pression_txt->setText("Pression : ");
        ui->Reglages_txt->setText("Réglages : ");
        ui->Taille_txt->setText("Dimensions : ");
        ui->Hauteur_txt->setText("Hauteur : ");
        ui->Taille_txt_2->setText("Quantité : ");
        ui->Taille_txt_3->setText("Espacement : ");
        ui->Titre->setText("Eau-Couper by AE");
    } else {
        if (settingsMenu) settingsMenu->setTitle("Settings");
        if (languageMenu) languageMenu->setTitle("Language");
        if (actionFrench) actionFrench->setText("French");
        if (actionEnglish) actionEnglish->setText("English");
        ui->Cercle->setText("Circle");
        ui->Rectangle->setText("Rectangle");
        ui->Triangle->setText("Triangle");
        ui->Coeur->setText("Heart");
        ui->Etoile->setText("Star");
        ui->optimizePlacementButton->setText("Optimize placement");
        ui->optimizePlacementButton2->setText("Optimize placement 2");
        ui->buttonInventaire->setText("Inventory");
        ui->buttonCustom->setText("Custom");
        ui->buttonFileReceiver->setText("Receive file");
        if (ui->buttonSettings) ui->buttonSettings->setText("Settings");
        ui->Vitesse_txt->setText("Speed: ");
        ui->Pression_txt->setText("Pressure: ");
        ui->Reglages_txt->setText("Settings: ");
        ui->Taille_txt->setText("Dimensions: ");
        ui->Hauteur_txt->setText("Height: ");
        ui->Taille_txt_2->setText("Quantity: ");
        ui->Taille_txt_3->setText("Spacing: ");
        ui->Titre->setText("Water-Cut by AE");
    }
    Inventaire::getInstance()->updateTranslations(currentLanguage);
}
