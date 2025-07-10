#include "custom.h"
#include "MainWindow.h"
#include "qlayout.h"
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
        emit resetDrawingSignal();
    });

    // Bouton "Menu" : retourne à la fenêtre principale
    connect(ui->buttonMenu, &QPushButton::clicked, this, &custom::goToMainWindow);

    // Bouton "Save" : enregistre la forme personnalisée
    connect(ui->buttonSave, &QPushButton::clicked, this, &custom::saveCustomShape);

    //Relier deux extrémités
    connect(ui->buttonConnect, &QPushButton::clicked, drawArea, &::CustomDrawArea::startShapeSelection);

    // Bouton de sélection multiple
    connect(ui->buttonSelection, &QPushButton::clicked, drawArea, &CustomDrawArea::toggleMultiSelectMode);

    // Bouton copier/coller
    connect(ui->buttonCopyPaste, &QPushButton::clicked, this, &custom::onCopyPasteClicked);


    connect(ui->buttonCloseShape, &QPushButton::clicked,
            drawArea, &CustomDrawArea::startCloseMode);


    ui->buttonLissage->setCheckable(true);      // bouton ON / OFF
    ui->buttonLissage->setChecked(false);       // désactivé au lancement
    ui->buttonLissage->setText("Lissage OFF");

    // 1) Quand l’utilisateur clique ➜ on (dé)active le lissage
    connect(ui->buttonLissage, &QPushButton::toggled,
            drawArea,           &CustomDrawArea::setSmoothingEnabled);

    // 2) Quand le code change l’état ➜ on met le texte à jour
    connect(drawArea, &CustomDrawArea::smoothingChanged,
            ui->buttonLissage, [=](bool on){
                ui->buttonLissage->blockSignals(true);          // pas de boucle
                ui->buttonLissage->setChecked(on);
                ui->buttonLissage->setText(on ? "Lissage ON"
                                              : "Lissage OFF");
                ui->buttonLissage->blockSignals(false);
            });

    // ----------------- Fonctions complémentaires -----------------

    // Bouton "Gomme" : active le mode gomme
    connect(ui->buttonGomme, &QPushButton::clicked, this, [this]() {
        drawArea->setDrawMode(CustomDrawArea::DrawMode::Gomme);
        //qDebug() << "Mode Gomme sélectionné";

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

    // Ajout des actions au menu
    menuForme->addAction(actionAlaMain);
    menuForme->addAction(actionPointParPoint);
    menuForme->addAction(actionLigne);
    menuForme->addAction(actionCercle);
    menuForme->addAction(actionRectangle);
    menuForme->addAction(actionText);

    ui->buttonForme->setMenu(menuForme);
    ui->buttonForme->setPopupMode(QToolButton::InstantPopup);

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
        //qDebug() << "Mode Texte activé";
        fontContainer->show();
    });
    // Pour les autres modes, cacher fontContainer
    connect(actionAlaMain, &QAction::triggered, this, [=]() {
        drawArea->setDrawMode(CustomDrawArea::DrawMode::Freehand);
        //qDebug() << "Mode À la main activé";
        fontContainer->hide();
    });
    connect(actionPointParPoint, &QAction::triggered, this, [=]() {
        drawArea->setDrawMode(CustomDrawArea::DrawMode::PointParPoint);
        //qDebug() << "Mode Point par point activé";
        fontContainer->hide();
    });
    connect(actionLigne, &QAction::triggered, this, [=]() {
        drawArea->setDrawMode(CustomDrawArea::DrawMode::Line);
        //qDebug() << "Mode Ligne activé";
        fontContainer->hide();
    });
    connect(actionCercle, &QAction::triggered, this, [=]() {
        drawArea->setDrawMode(CustomDrawArea::DrawMode::Circle);
        //qDebug() << "Mode Cercle activé";
        fontContainer->hide();
    });
    connect(actionRectangle, &QAction::triggered, this, [=]() {
        drawArea->setDrawMode(CustomDrawArea::DrawMode::Rectangle);
        //qDebug() << "Mode Rectangle activé";
        fontContainer->hide();
    });

    // --- Connexions pour les autres boutons ---
    connect(ui->buttonSupprimer, &QPushButton::clicked, this, [this]() {
        if (drawArea->hasSelection()) {
            drawArea->deleteSelectedShapes();
        } else {
            drawArea->setDrawMode(CustomDrawArea::DrawMode::Supprimer);
            //qDebug() << "Mode Supprimer sélectionné";
        }
    });
    connect(ui->buttonDeplacer, &QPushButton::clicked, this, [this]() {
        drawArea->setDrawMode(CustomDrawArea::DrawMode::Deplacer);
        //qDebug() << "Mode Déplacer activé";
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

    // Suppose que votre bouton s’appelle buttonSnapGrid dans l’UI
    connect(ui->buttonSnapGrid, &QPushButton::clicked, this, [=]() {
        // Inverser l’état actuel
        bool newState = !drawArea->isSnapToGridEnabled();
        drawArea->setSnapToGridEnabled(newState);

        // (optionnel) changer le texte ou la couleur du bouton pour refléter l’état
        ui->buttonSnapGrid->setText(newState ? "Snap Grille : ON" : "Snap Grille : OFF");
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
        tr("Images (*.png *.jpg *.bmp *.svg)")
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
        "",  tr("Images (*.png *.jpg *.bmp)"));
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

void custom::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QWidget::changeEvent(event);
}

