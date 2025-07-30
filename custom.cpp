#include "custom.h"
#include "MainWindow.h"
#include "qlayout.h"
#include "qsplitter.h"
#include "ui_custom.h"
#include "clavier.h"
#include "inventaire.h"
#include "LogoImporter.h"
#include "ImageEdgeImporter.h"
#include <QGraphicsView>
#include <QGraphicsScene>
#include "Language.h"
#include <QSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QInputDialog>
#include <QDebug>
#include <QMenu>
#include <QToolButton>
#include <QSignalBlocker>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QImage>
#include <QProgressDialog>
#include <QApplication>
#include <algorithm>
#include <QFontComboBox>
#include <QHBoxLayout>
#include <cmath>
#include "ScreenUtils.h"

// Constructeur : création de l'interface et des connexions
custom::custom(Language lang, QWidget *parent)
    : QWidget(parent),
    ui(new Ui::custom)
{
    ui->setupUi(this);

    // ----- Splitter : facteurs de stretch et tailles initiales (après show) -----
    if (auto sp = findChild<QSplitter*>("mainSplitter")) {
        sp->setStretchFactor(0, 0);   // gauche
        sp->setStretchFactor(1, 1);   // centre
        sp->setStretchFactor(2, 0);   // droite
        sp->setCollapsible(0, true);
        sp->setCollapsible(2, true);

        // Mémorise des largeurs par défaut (serviront quand on ré-ouvre)
        ui->verticalLayoutWidget->setProperty("lastWidth", 160);
        ui->verticalLayoutWidget_2->setProperty("lastWidth", 220);

        // Définir les tailles après que le widget soit posé (pour avoir la vraie largeur)
        QTimer::singleShot(0, this, [this, sp](){
            int W = this->width();
            int left  = ui->verticalLayoutWidget->isVisible()  ? 160 : 0;
            int right = ui->verticalLayoutWidget_2->isVisible()? 220 : 0;
            int center = std::max(200, W - (left + right + 40));
            sp->setSizes({left, center, right});

            // Mets les flèches dans le bon sens au démarrage
            ui->chevronLeft->setText(left  == 0 ? "›" : "‹");   // gauche plié -> flèche vers la droite
            ui->chevronRight->setText(right == 0 ? "‹" : "›");  // droite plié -> flèche vers la gauche
        });

        // Si l'utilisateur replie un côté à la main (poignée du splitter), on met à jour les flèches
        connect(sp, &QSplitter::splitterMoved, this, [this, sp](int /*pos*/, int /*index*/){
            const QList<int> s = sp->sizes();
            // Si la colonne n'est pas à 0, on mémorise sa dernière largeur non nulle
            if (s[0] > 0) ui->verticalLayoutWidget->setProperty("lastWidth", s[0]);
            if (s[2] > 0) ui->verticalLayoutWidget_2->setProperty("lastWidth", s[2]);

            ui->chevronLeft->setText( s[0] == 0 ? "›" : "‹" );
            ui->chevronRight->setText( s[2] == 0 ? "‹" : "›" );
        });
    }

    // ----- Helpers : plier/déplier les panneaux avec mémorisation de largeur -----
    auto toggleLeftPanel = [this]() {
        auto sp = findChild<QSplitter*>("mainSplitter");
        if (!sp) return;
        QList<int> s = sp->sizes(); // {gauche, centre, droite}

        bool isCollapsed = (s[0] == 0) || !ui->verticalLayoutWidget->isVisible();
        if (isCollapsed) {
            // Ré-ouvrir : récupère la dernière largeur non nulle (ou défaut 160)
            int left = ui->verticalLayoutWidget->property("lastWidth").toInt();
            if (left <= 0) left = 160;
            ui->verticalLayoutWidget->setVisible(true);
            int centre = std::max(100, s[1] - left);
            sp->setSizes({left, centre, s[2]});
            ui->chevronLeft->setText("‹");
        } else {
            // Replier : mémorise la largeur avant de fermer
            int left = std::max(0, s[0]);
            if (left > 0) ui->verticalLayoutWidget->setProperty("lastWidth", left);
            ui->verticalLayoutWidget->setVisible(false);
            sp->setSizes({0, s[0] + s[1], s[2]});
            ui->chevronLeft->setText("›");
        }
    };

    auto toggleRightPanel = [this]() {
        auto sp = findChild<QSplitter*>("mainSplitter");
        if (!sp) return;
        QList<int> s = sp->sizes(); // {gauche, centre, droite}

        bool isCollapsed = (s[2] == 0) || !ui->verticalLayoutWidget_2->isVisible();
        if (isCollapsed) {
            int right = ui->verticalLayoutWidget_2->property("lastWidth").toInt();
            if (right <= 0) right = 220;
            ui->verticalLayoutWidget_2->setVisible(true);
            int centre = std::max(100, s[1] - right);
            sp->setSizes({s[0], centre, right});
            ui->chevronRight->setText("›");
        } else {
            int right = std::max(0, s[2]);
            if (right > 0) ui->verticalLayoutWidget_2->setProperty("lastWidth", right);
            ui->verticalLayoutWidget_2->setVisible(false);
            sp->setSizes({s[0], s[1] + s[2], 0});
            ui->chevronRight->setText("‹");
        }
    };

    // Connexions des chevrons (bas gauche / bas droite)
    connect(ui->chevronLeft,  &QToolButton::clicked, this, toggleLeftPanel);
    connect(ui->chevronRight, &QToolButton::clicked, this, toggleRightPanel);

    // Valeurs d’icône/flèche initiales (au cas où)
    ui->chevronLeft->setText("‹");
    ui->chevronRight->setText("›");

    ui->buttonCopyPaste->setVisible(false);

    ScreenUtils::placeOnSecondaryScreen(this);

    // Création des vues pour l'image couleur et l'image de bords
    m_colorView = new QGraphicsView(this);
    m_edgeView  = new QGraphicsView(this);
    m_colorScene = new QGraphicsScene(this);
    m_edgeScene  = new QGraphicsScene(this);
    m_colorView->setScene(m_colorScene);
    m_edgeView->setScene(m_edgeScene);
    m_colorView->setVisible(false);
    m_edgeView->setVisible(false);

    // Création de l'instance de CustomDrawArea
    drawArea = new CustomDrawArea(this);

    // Ajout des vues et de drawArea dans le widget "drawingWidget"
    if (ui->drawingWidget) {
        if (!ui->drawingWidget->layout())
            ui->drawingWidget->setLayout(new QVBoxLayout());
        auto *dwLayout = qobject_cast<QVBoxLayout*>(ui->drawingWidget->layout());
        QHBoxLayout *imgLayout = new QHBoxLayout();
        imgLayout->addWidget(m_colorView);
        imgLayout->addWidget(m_edgeView);
        dwLayout->addLayout(imgLayout);
        dwLayout->addWidget(drawArea);
    } else {
        //qDebug() << "Erreur : ui->drawingWidget est nullptr !";
    }

    // Changer la couleur du bouton "fermer" quand il est actif
    connect(drawArea, &CustomDrawArea::closeModeChanged,
            this, [this](bool enabled){
        ui->buttonCloseShape->setProperty("closeMode", enabled);
        // et repolish pour forcer la réapplication du style
        ui->buttonCloseShape->style()->unpolish(ui->buttonCloseShape);
        ui->buttonCloseShape->style()->polish(ui->buttonCloseShape);
        ui->buttonCloseShape->update();
    });

    // Changer la couleur du bouton "relier" quand il est actif
    connect(drawArea, &CustomDrawArea::shapeSelection,
            this, [this](bool enabled){
        ui->buttonConnect->setProperty("closeMode", enabled);
        // et repolish pour forcer la réapplication du style
        ui->buttonConnect->style()->unpolish(ui->buttonConnect);
        ui->buttonConnect->style()->polish(ui->buttonConnect);
        ui->buttonConnect->update();
    });

    // Changer la couleur du bouton "selection" quand le mode est actif
    connect(drawArea, &CustomDrawArea::multiSelectionModeChanged,
            this, [this](bool enabled){
        ui->buttonSelection->setProperty("closeMode", enabled);
        ui->buttonSelection->style()->unpolish(ui->buttonSelection);
        ui->buttonSelection->style()->polish(ui->buttonSelection);
        ui->buttonSelection->update();
        ui->buttonCopyPaste->setVisible(enabled);
        if (enabled)
            ui->buttonCopyPaste->setText(tr("Copier"));
    });

    // Bouton "Appliquer" : émission du signal avec les formes personnalisées puis fermeture
    connect(ui->Appliquer, &QPushButton::clicked, this, [this]() {
        //qDebug() << "Signal applyCustomShapeSignal émis avec les formes !";
        emit applyCustomShapeSignal(drawArea->getCustomShapes());
        this->close();
        delete this;
    });


    // Bouton "Reset" : efface le dessin et émet le signal correspondant
    connect(ui->Reset, &QPushButton::clicked, this, [this]() {
        //qDebug() << "Signal resetDrawingSignal émis !";
        drawArea->clearDrawing();
        drawArea->cancelSelection();
        drawArea->cancelCloseMode();
        drawArea->cancelDeplacerMode();
        drawArea->cancelGommeMode();
        drawArea->cancelSupprimerMode();
        emit resetDrawingSignal();
    });

    // Bouton "Menu" : retourne à la fenêtre principale
    connect(ui->buttonMenu, &QPushButton::clicked, this, &custom::goToMainWindow);

    // Bouton "Save" : enregistre la forme personnalisée
    connect(ui->buttonSave, &QPushButton::clicked, this, &custom::saveCustomShape);

    //Relier deux extrémités
    connect(ui->buttonConnect, &QPushButton::clicked, this, [this]() {
        if (drawArea->isConnectMode()) {
            drawArea->cancelSelection();  // ← désactive proprement le mode
        } else {
            drawArea->startShapeSelection();  // ← active le mode
        }
    });


    // Bouton de sélection multiple
    connect(ui->buttonSelection, &QPushButton::clicked, drawArea, &CustomDrawArea::toggleMultiSelectMode);

    // Bouton copier/coller
    connect(ui->buttonCopyPaste, &QPushButton::clicked, this, &custom::onCopyPasteClicked);


    connect(ui->buttonCloseShape, &QPushButton::clicked, this, [this]() {
        drawArea->cancelSelection();
        drawArea->cancelDeplacerMode();
        drawArea->startCloseMode();
    });


    // ----------------- Fonctions complémentaires -----------------

    // Bouton "Gomme" : active le mode gomme
    connect(ui->buttonGomme, &QPushButton::clicked, this, [this]() {
        drawArea->setDrawMode(CustomDrawArea::DrawMode::Gomme);
        updateFormeButtonIcon(CustomDrawArea::DrawMode::Gomme);
        //qDebug() << "Mode Gomme sélectionné";
    });

    // Configuration du slider de lissage
    ui->smoothingSlider->setMinimum(0);
    ui->smoothingSlider->setMaximum(10);
    ui->smoothingSlider->setSingleStep(1);
    ui->smoothingSlider->setPageStep(1);
    ui->smoothingSlider->setTickInterval(1);
    ui->smoothingSlider->setTickPosition(QSlider::TicksBelow);
    ui->smoothingSlider->setValue(0);  // ← important : on force la valeur visible

    // Appliquer immédiatement le niveau de lissage réel
    drawArea->setSmoothingLevel(0);    // ← synchronise le modèle avec la vue
    ui->labelSmoothingValue->setText("Puissance du lissage : 0%");

    // Ensuite seulement tu connectes les signaux
    connect(ui->smoothingSlider, &QSlider::valueChanged,
            drawArea, &CustomDrawArea::setSmoothingLevel);

    connect(ui->smoothingSlider, &QSlider::valueChanged, this, [=](int value){
        int percent = static_cast<int>(std::round(100.0 * value / 10.0));
        ui->labelSmoothingValue->setText(QString("Puissance du lissage : %1%").arg(percent));
    });

    connect(drawArea, &CustomDrawArea::smoothingLevelChanged, this, [=](int level){
        QSignalBlocker b(ui->smoothingSlider);
        ui->smoothingSlider->setValue(level);
        int percent = static_cast<int>(std::round(100.0 * level / 10.0));
        ui->labelSmoothingValue->setText(QString("Puissance du lissage : %1%").arg(percent));
    });


    // --- Déclarations préliminaires pour le menu et le conteneur de police ---
    QMenu *menuForme = new QMenu(this);
    QWidget *fontContainer = new QWidget(this);  // Conteneur pour les contrôles de police

    // Création des actions pour les différents modes de dessin
    QAction *actionAlaMain = new QAction(tr("À la main"), this);
    QAction *actionPointParPoint = new QAction(tr("Point par point"), this);
    QAction *actionLigne = new QAction(tr("Ligne droite"), this);
    QAction *actionCercle = new QAction(tr("Cercle"), this);
    QAction *actionRectangle = new QAction(tr("Rectangle"), this);
    QAction *actionText = new QAction(tr("Texte"), this);
    QAction *actionThinText = new QAction(tr("Texte fin"), this);

    // Ajout des actions au menu
    menuForme->addAction(actionAlaMain);
    menuForme->addAction(actionPointParPoint);
    menuForme->addAction(actionLigne);
    menuForme->addAction(actionCercle);
    menuForme->addAction(actionRectangle);
    menuForme->addAction(actionText);
    menuForme->addAction(actionThinText);

    ui->buttonForme->setMenu(menuForme);
    ui->buttonForme->setPopupMode(QToolButton::InstantPopup);
    updateFormeButtonIcon(drawArea->getDrawMode());

    // --- Création et configuration du conteneur pour la sélection de police ---
    QHBoxLayout *fontLayout = new QHBoxLayout(fontContainer);
    fontLayout->setContentsMargins(2, 2, 2, 2);  // Marges réduites
    fontLayout->setSpacing(5);                   // Espacement réduit

    QFontComboBox *fontCombo = new QFontComboBox(fontContainer);
    // Afficher uniquement les polices vectorielles
    fontCombo->setFontFilters(QFontComboBox::ScalableFonts);
    fontCombo->setMinimumWidth(120);
    fontCombo->setFixedHeight(23);

    QSpinBox *fontSizeSpin = new QSpinBox(fontContainer);
    fontSizeSpin->setRange(8, 72);
    fontSizeSpin->setValue(drawArea->getTextFont().pointSize());
    // Augmenter la largeur de la spinbox et fixer la hauteur
    fontSizeSpin->setMinimumWidth(80);
    fontSizeSpin->setFixedHeight(23);

    fontLayout->addWidget(new QLabel(tr("Police:"), fontContainer));
    fontLayout->addWidget(fontCombo);
    fontLayout->addWidget(new QLabel(tr("Taille:"), fontContainer));
    fontLayout->addWidget(fontSizeSpin);

    // Définir des dimensions fixes pour le conteneur
    fontContainer->setFixedWidth(700);
    fontContainer->setFixedHeight(50);
    // Masquer le conteneur par défaut
    fontContainer->hide();

    // Ajouter le conteneur dans le layout de drawingWidget si disponible, sinon dans le layout principal
    if (ui->drawingWidget) {
        if (!ui->drawingWidget->layout())
            ui->drawingWidget->setLayout(new QVBoxLayout());
        ui->drawingWidget->layout()->addWidget(fontContainer);
    } else {
        if (layout())
            layout()->addWidget(fontContainer);
        else {
            QVBoxLayout *mainLayout = new QVBoxLayout(this);
            mainLayout->addWidget(fontContainer);
            setLayout(mainLayout);
        }
    }

    // --- Connexions pour les actions de mode ---
    // Mode Texte : afficher fontContainer
    connect(actionText, &QAction::triggered, this, [=]() {
        drawArea->setDrawMode(CustomDrawArea::DrawMode::Text);
        updateFormeButtonIcon(CustomDrawArea::DrawMode::Text);
        //qDebug() << "Mode Texte activé";
        fontContainer->show();
    });
    // Mode Texte fin : afficher fontContainer
    connect(actionThinText, &QAction::triggered, this, [=]() {
        drawArea->setDrawMode(CustomDrawArea::DrawMode::ThinText);
        updateFormeButtonIcon(CustomDrawArea::DrawMode::ThinText);
        fontContainer->show();
    });
    // Pour les autres modes, cacher fontContainer
    connect(actionAlaMain, &QAction::triggered, this, [=]() {
        drawArea->setDrawMode(CustomDrawArea::DrawMode::Freehand);
        updateFormeButtonIcon(CustomDrawArea::DrawMode::Freehand);
        //qDebug() << "Mode À la main activé";
        fontContainer->hide();
    });
    connect(actionPointParPoint, &QAction::triggered, this, [=]() {
        drawArea->setDrawMode(CustomDrawArea::DrawMode::PointParPoint);
        updateFormeButtonIcon(CustomDrawArea::DrawMode::PointParPoint);
        //qDebug() << "Mode Point par point activé";
        fontContainer->hide();
    });
    connect(actionLigne, &QAction::triggered, this, [=]() {
        drawArea->setDrawMode(CustomDrawArea::DrawMode::Line);
        updateFormeButtonIcon(CustomDrawArea::DrawMode::Line);
        //qDebug() << "Mode Ligne activé";
        fontContainer->hide();
    });
    connect(actionCercle, &QAction::triggered, this, [=]() {
        drawArea->setDrawMode(CustomDrawArea::DrawMode::Circle);
        updateFormeButtonIcon(CustomDrawArea::DrawMode::Circle);
        //qDebug() << "Mode Cercle activé";
        fontContainer->hide();
    });
    connect(actionRectangle, &QAction::triggered, this, [=]() {
        drawArea->setDrawMode(CustomDrawArea::DrawMode::Rectangle);
        updateFormeButtonIcon(CustomDrawArea::DrawMode::Rectangle);
        //qDebug() << "Mode Rectangle activé";
        fontContainer->hide();
    });

    // --- Connexions pour les autres boutons ---
    connect(ui->buttonSupprimer, &QPushButton::clicked, this, [this]() {
        if (drawArea->hasSelection()) {
            drawArea->deleteSelectedShapes();
        } else {
            drawArea->setDrawMode(CustomDrawArea::DrawMode::Supprimer);
            updateFormeButtonIcon(CustomDrawArea::DrawMode::Supprimer);
            //qDebug() << "Mode Supprimer sélectionné";
        }
    });
    connect(ui->buttonDeplacer, &QPushButton::clicked, this, [this]() {
        if (drawArea->isDeplacerMode()) {
            // ✅ Si déjà activé → désactive
            drawArea->cancelDeplacerMode();
        } else {
            // ✅ Sinon → active
            drawArea->startDeplacerMode();
        }
    });

    connect(drawArea, &CustomDrawArea::deplacerModeChanged,
            this, [this](bool enabled){

        qDebug() << "[UI] Signal deplacerModeChanged reçu :" << enabled;

        ui->buttonDeplacer->setProperty("deplacerMode", enabled);
        qDebug() << "[UI] Property appliquée à buttonDeplacer ="
                 << ui->buttonDeplacer->property("closeMode").toBool();
//        ui->buttonDeplacer->setText(enabled ? "DÉPLACER ✅" : "DÉPLACER");
        ui->buttonDeplacer->setStyleSheet(ui->buttonDeplacer->styleSheet());

        ui->buttonDeplacer->update();
    });

    qDebug() << "[DEBUG] Connexion faite avec deplacerModeChanged";

    connect(ui->buttonSupprimer, &QPushButton::clicked, this, [this]() {
        if (drawArea->isSupprimerMode()) {
            drawArea->cancelSupprimerMode();
        } else {
            drawArea->startSupprimerMode();
        }
    });
    connect(drawArea, &CustomDrawArea::supprimerModeChanged,
            this, [this](bool enabled){
                qDebug() << "[UI] Signal supprimerModeChanged reçu :" << enabled;

                ui->buttonSupprimer->setProperty("supprimerMode", enabled);
                qDebug() << "[UI] Property supprimerMode ="
                         << ui->buttonSupprimer->property("supprimerMode").toBool();

                ui->buttonSupprimer->setStyleSheet(ui->buttonSupprimer->styleSheet());
                ui->buttonSupprimer->update();
            });

    connect(ui->buttonGomme, &QPushButton::clicked, this, [this]() {
        if (drawArea->isGommeMode()) {
            drawArea->cancelGommeMode();
        } else {
            drawArea->startGommeMode();
        }
    });

    connect(drawArea, &CustomDrawArea::gommeModeChanged,
            this, [this](bool enabled){
                qDebug() << "[UI] Signal gommeModeChanged reçu :" << enabled;

                ui->buttonGomme->setProperty("gommeMode", enabled);
                qDebug() << "[UI] Property gommeMode ="
                         << ui->buttonGomme->property("gommeMode").toBool();

                ui->buttonGomme->setStyleSheet(ui->buttonGomme->styleSheet());
                ui->buttonGomme->update();
            });


    connect(ui->buttonRetour, &QPushButton::clicked, drawArea, &CustomDrawArea::undoLastAction);

    // Menu pour les différents types d'importation d'image
    QMenu *importMenu = new QMenu(this);
    QAction *actionImportLogo = new QAction(tr("Importer un logo"), this);
    QAction *actionImportImage = new QAction(tr("Importer image couleur"), this);
    importMenu->addAction(actionImportLogo);
    importMenu->addAction(actionImportImage);
    ui->buttonImporter->setMenu(importMenu);
    ui->buttonImporter->setPopupMode(QToolButton::InstantPopup);
    connect(actionImportLogo, &QAction::triggered, this, &custom::importerLogo);
    connect(actionImportImage, &QAction::triggered, this, &custom::importerImageCouleur);

    // --- Connexions pour mettre à jour la police dans drawArea ---
    connect(fontCombo, &QFontComboBox::currentFontChanged, this, [=]() {
        QFont newFont = fontCombo->currentFont();
        newFont.setPointSize(fontSizeSpin->value());
        drawArea->setTextFont(newFont);
    });
    connect(fontSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [=]() {
        QFont newFont = fontCombo->currentFont();
        newFont.setPointSize(fontSizeSpin->value());
        drawArea->setTextFont(newFont);
    });

    // --- Création du QComboBox pour les polices favorites ---
    QComboBox *favFontCombo = new QComboBox(this);
    favFontCombo->setMinimumWidth(120);
    favFontCombo->setFixedHeight(23);
    favFontCombo->setStyleSheet("QComboBox { font-size: 10pt; padding: 2px; }");
    favFontCombo->clear();
    favFontCombo->addItems(m_favoriteFonts);  // m_favoriteFonts est un membre (QStringList)

    // --- Création d'un bouton pour ajouter la police actuelle aux favoris ---
    QPushButton *btnAddFav = new QPushButton(tr("Ajouter aux favoris"), this);
    btnAddFav->setFixedHeight(23);
    btnAddFav->setStyleSheet("QPushButton { font-size: 10pt; padding: 2px; }");
    connect(btnAddFav, &QPushButton::clicked, this, [=]() {
        QFont currentFont = fontCombo->currentFont();
        QString fontFamily = currentFont.family();
        if (!m_favoriteFonts.contains(fontFamily)) {
            m_favoriteFonts.append(fontFamily);
            favFontCombo->clear();
            favFontCombo->addItems(m_favoriteFonts);
            //qDebug() << "Ajouté aux favoris:" << fontFamily;
        }
    });

    // --- Création d'un bouton pour supprimer la police favorite sélectionnée ---
    QPushButton *btnRemoveFav = new QPushButton(tr("Supprimer favoris"), this);
    btnRemoveFav->setFixedHeight(23);
    btnRemoveFav->setStyleSheet("QPushButton { font-size: 10pt; padding: 2px; }");
    connect(btnRemoveFav, &QPushButton::clicked, this, [=]() {
        int idx = favFontCombo->currentIndex();
        if (idx >= 0 && idx < m_favoriteFonts.size()) {
            QString removed = m_favoriteFonts.takeAt(idx);
            favFontCombo->clear();
            favFontCombo->addItems(m_favoriteFonts);
            //qDebug() << "Favori supprimé:" << removed;
            // Optionnel : appliquer une police par défaut si nécessaire.
        }
    });

    // --- Regrouper le label "Favoris", le QComboBox, et les boutons dans un layout horizontal ---
    QHBoxLayout *favLayout = new QHBoxLayout;
    favLayout->setContentsMargins(15, 2, 2, 2);  // Décalage à gauche de 20 pixels
    favLayout->setSpacing(5);
    favLayout->addWidget(new QLabel(tr("Favoris:"), this));
    favLayout->addWidget(favFontCombo);
    favLayout->addWidget(btnAddFav);
    favLayout->addWidget(btnRemoveFav);

    // --- Ajouter ce layout horizontal dans le conteneur de police existant ---
    // fontContainer est déjà créé et dispose d'un layout horizontal (fontLayout)
    fontLayout->addLayout(favLayout);

    // --- Connecter la sélection dans le combo des favoris pour mettre à jour le QFontComboBox principal ---
    connect(favFontCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index) {
        if (index >= 0 && index < m_favoriteFonts.size()) {
            QFont newFont(m_favoriteFonts.at(index), fontSizeSpin->value());
            fontCombo->setCurrentFont(newFont);
            drawArea->setTextFont(newFont);
            //qDebug() << "Police favorite sélectionnée :" << newFont.family();
        }
    });

    connect(ui->buttonSnapGrid, &QPushButton::clicked, this, [=]() {
        // Inverser l’état actuel
        bool enabled = !drawArea->isSnapToGridEnabled();
        drawArea->setSnapToGridEnabled(enabled);

        // Mise à jour visuelle
        ui->buttonSnapGrid->setText(enabled ? "Aimanté ✅" : "Aimanté ❌");
        ui->buttonSnapGrid->setProperty("snapMode", enabled);

        qDebug() << "[SnapGrid] Activé =" << enabled;

        // Mise à jour du style dynamique
        ui->buttonSnapGrid->style()->unpolish(ui->buttonSnapGrid);
        ui->buttonSnapGrid->style()->polish(ui->buttonSnapGrid);
        ui->buttonSnapGrid->update();
    });

    connect(ui->buttonToggleGrid, &QPushButton::clicked, this, [=]() {
        bool visible = !drawArea->isGridVisible();
        drawArea->setGridVisible(visible);

        ui->buttonToggleGrid->setText(visible ? "Grille ✅" : "Grille ❌");
        ui->buttonToggleGrid->setProperty("gridVisible", visible);

        ui->buttonToggleGrid->style()->unpolish(ui->buttonToggleGrid);
        ui->buttonToggleGrid->style()->polish(ui->buttonToggleGrid);
        ui->buttonToggleGrid->update();
    });



    // slider --> change spacing
    connect(ui->sliderGridSpacing, &QSlider::valueChanged,
            this, [=](int value){
        drawArea->setGridSpacing(value);                         // met à jour la grille
        ui->labelGridSpacing->setText(QString("%1 px").arg(value)); // feedback visuel
    });

    // 1) valeur initiale dès le lancement
    drawArea->setEraserRadius(ui->tailleGomme->value());

    // 2) quand on déplace le curseur → on met à jour la gomme
    connect(ui->tailleGomme, &QSlider::valueChanged,
            this, [=](int v){
                drawArea->setEraserRadius(static_cast<qreal>(v));

    });



}

custom::~custom()
{
    delete ui;
}

void custom::goToMainWindow()
{
    this->close();
    MainWindow::getInstance()->showFullScreen();
}

void custom::closeCustom()
{
    this->close();
    delete this;
}

void custom::ouvrirClavier()
{
    Clavier clavier(this);
    if (clavier.exec() == QDialog::Accepted) {
        QString texteSaisi = clavier.getText();
    }
}

void custom::saveCustomShape() {
    // Récupère toutes les formes (traits) du CustomDrawArea
    QList<QPolygonF> shapes = drawArea->getCustomShapes();
    if (shapes.isEmpty()) {
        //qDebug() << "Aucune forme à enregistrer.";
        return;
    }

    bool ok = false;
    QString shapeName;
    do {
        shapeName = QInputDialog::getText(this, tr("Nom de la forme"),
                                          tr("Entrez un nom pour votre forme :"),
                                          QLineEdit::Normal, "", &ok);
        if (!ok)
            return; // Annulation
        if (shapeName.isEmpty())
            continue;
        // custom.cpp ── dans saveCustomShape()
        if (Inventaire::getInstance()->shapeNameExists(shapeName))
        {
            // Boîte d'avertissement SANS boutons, modale, fermée après 2,5 s
            QMessageBox msg(QMessageBox::Warning,
                            tr("Nom déjà utilisé"),
                            tr("Ce nom est déjà utilisé, veuillez en choisir un autre."),
                            QMessageBox::NoButton,
                            this);               // parent

            QTimer::singleShot(2300, &msg, &QMessageBox::accept); // auto-fermeture
            msg.exec();                                           // MODAL et bloquant

            ok = false;    // force une nouvelle itération du do/while
        }
    } while(!ok);

    // Calculer le rectangle englobant toutes les formes
    QRectF boundingRect;
    for (const QPolygonF &poly : shapes) {
        boundingRect = boundingRect.united(poly.boundingRect());
    }

    int padding = 10;
    QSize imageSize(qRound(boundingRect.width()) + 2 * padding,
                    qRound(boundingRect.height()) + 2 * padding);
    QImage previewImage(imageSize, QImage::Format_ARGB32_Premultiplied);
    previewImage.fill(Qt::white);

    // Dessiner toutes les formes dans l'image
    QPainter painter(&previewImage);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(-boundingRect.topLeft() + QPointF(padding, padding));
    QPen pen(Qt::black, 2);
    painter.setPen(pen);
    for (const QPolygonF &poly : shapes) {
        painter.drawPolyline(poly);
    }
    painter.end();

    // Sauvegarde de l'image dans l'inventaire
    Inventaire::getInstance()->addSavedCustomShape(shapes, shapeName);
}

static QList<QPainterPath> separateIntoSubpaths(const QPainterPath &path)
{
    QList<QPainterPath> subpaths;
    QPainterPath current;
    int count = path.elementCount();
    for (int i = 0; i < count; ++i) {
        QPainterPath::Element e = path.elementAt(i);
        if (e.isMoveTo()) {
            if (!current.isEmpty())
                subpaths.append(current);
            current = QPainterPath();
            current.moveTo(e.x, e.y);
        } else {
            current.lineTo(e.x, e.y);
        }
    }
    if (!current.isEmpty())
        subpaths.append(current);
    return subpaths;
}


void custom::importerLogo()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("Sélectionner une image"),
        "",
        tr("Images (*.png *.jpg *.bmp *.svg *.webp)")
        );
    if (filePath.isEmpty())
        return;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Contours internes"),
        tr("Voulez-vous inclure également les contours internes ?"),
        QMessageBox::Yes | QMessageBox::No
        );
    bool includeInternal = (reply == QMessageBox::Yes);

    LogoImporter importer;
    QPainterPath outline = importer.importLogo(filePath, includeInternal, 128);
    if (outline.isEmpty()) {
        //qDebug() << "Le chemin importé est vide, vérifiez l'image ou la méthode d'import.";
        return;
    }

    QRectF br = outline.boundingRect();
    double maxDimension = std::max(br.width(), br.height());
    if (maxDimension < 0.0001)
        return;
    double scaleFactor = 300.0 / maxDimension;
    QTransform transform;
    transform.translate(-br.x(), -br.y());
    transform.scale(scaleFactor, scaleFactor);
    QPainterPath scaledOutline = transform.map(outline);
    QRectF scaledBounds = scaledOutline.boundingRect();
    QPointF drawingCenter(drawArea->width() / 2.0, drawArea->height() / 2.0);
    QPointF offset = drawingCenter - scaledBounds.center();
    scaledOutline.translate(offset);
    //qDebug() << "Bounding rect final (centré):" << scaledOutline.boundingRect();

    QList<QPainterPath> subpaths = separateIntoSubpaths(scaledOutline);
    //qDebug() << "Nombre de sous-chemins importés:" << subpaths.size();
    for (const QPainterPath &sp : subpaths) {
        drawArea->addImportedLogoSubpath(sp);
    }
}

void custom::importerImageCouleur()
{
    QString filePath = QFileDialog::getOpenFileName(
        this, tr("Sélectionner une image"),
        "",  tr("Images (*.png *.jpg *.bmp *.webp)"));
    if (filePath.isEmpty()) return;

    QPainterPath edge;
    if (!m_imageImporter.loadAndProcess(filePath, edge)) {
        QMessageBox::warning(this, tr("Erreur"),
                             tr("Contour introuvable."));
        return;
    }

    // mise à l'échelle + centrage (identique à importerLogo)
    QRectF br = edge.boundingRect();
    double scale = 300.0 / std::max(br.width(), br.height());
    QTransform T;
    T.translate(-br.x(), -br.y());
    T.scale(scale, scale);
    QPainterPath scaled = T.map(edge);
    scaled.translate(QPointF(drawArea->width()/2.0,
                             drawArea->height()/2.0) -
                     scaled.boundingRect().center());

    QList<QPainterPath> subs =
        CustomDrawArea::separateIntoSubpaths(scaled);
    for (const QPainterPath &sp : subs)
        drawArea->addImportedLogoSubpath(sp);
}

void custom::onCopyPasteClicked()
{
    if (ui->buttonCopyPaste->text() == tr("Copier")) {
        drawArea->copySelectedShapes();
        ui->buttonCopyPaste->setText(tr("Coller"));
    } else {
        drawArea->enablePasteMode();
        // The next click in draw area will paste
        ui->buttonCopyPaste->setVisible(false);
        ui->buttonSelection->setProperty("closeMode", false);
        ui->buttonSelection->style()->unpolish(ui->buttonSelection);
        ui->buttonSelection->style()->polish(ui->buttonSelection);
        ui->buttonSelection->update();
    }
}

void custom::updateFormeButtonIcon(CustomDrawArea::DrawMode mode)
{
    static const QHash<CustomDrawArea::DrawMode, QString> iconMap = {
        { CustomDrawArea::DrawMode::Freehand,      ":/icons/freehand.svg" },
        { CustomDrawArea::DrawMode::PointParPoint, ":/icons/Point_to_point.svg" },
        { CustomDrawArea::DrawMode::Line,          ":/icons/line.svg" },
        { CustomDrawArea::DrawMode::Rectangle,     ":/icons/square.svg" },
        { CustomDrawArea::DrawMode::Circle,        ":/icons/circle.svg" },
        { CustomDrawArea::DrawMode::Text,          ":/icons/text.svg" },
        { CustomDrawArea::DrawMode::ThinText,      ":/icons/narrow_text.svg" }
    };

    auto it = iconMap.constFind(mode);
    if (it != iconMap.constEnd())
        ui->buttonForme->setIcon(QIcon(it.value()));
}

void custom::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QWidget::changeEvent(event);
}

