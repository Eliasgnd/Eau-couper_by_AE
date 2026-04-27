#include "CustomEditor.h"
#include "CustomEditorViewModel.h"
#include "qlayout.h"
#include "qstatusbar.h"
#include "ui_CustomEditor.h"
#include <QFile>
#include <QTextStream>
#include <QSettings>
#include "ThemeManager.h"
#include "KeyboardDialog.h"
#include "NumericKeyboardDialog.h"
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
#include <QMainWindow>
#include <QProgressDialog>
#include <QApplication>
#include <algorithm>
#include <QFontComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <cmath>
#include "ScreenUtils.h"
#include <QStatusBar>

static QString modeToString(CustomDrawArea::DrawMode mode)
{
    using DM = CustomDrawArea::DrawMode;
    switch (mode) {
    case DM::Freehand:      return QObject::tr("Tracé libre");
    case DM::PointParPoint: return QObject::tr("Point par point");
    case DM::Line:          return QObject::tr("Ligne droite");
    case DM::Rectangle:     return QObject::tr("Rectangle");
    case DM::Circle:        return QObject::tr("Cercle");
    case DM::Text:          return QObject::tr("Texte");
    case DM::ThinText:      return QObject::tr("Texte fin");
    case DM::Deplacer:      return QObject::tr("Déplacer");
    case DM::Gomme:         return QObject::tr("Gomme");
    case DM::Supprimer:     return QObject::tr("Supprimer");
    default:                return QObject::tr("Tracé libre");
    }
}

// Constructeur : création de l'interface et des connexions
CustomEditor::CustomEditor(CustomEditorViewModel *viewModel, Language lang, QWidget *parent)
    : QWidget(parent),
    ui(new Ui::CustomEditor),
    m_viewModel(viewModel)
{
    Q_UNUSED(lang);
    ui->setupUi(this);

    // ----- Thème global via ThemeManager -----
    m_isDarkTheme = ThemeManager::instance()->isDark();
    updateThemeButton();
    connect(ThemeManager::instance(), &ThemeManager::themeChanged,
            this, [this](bool dark){ m_isDarkTheme = dark; updateThemeButton(); });
    connect(ui->buttonTheme, &QPushButton::clicked, this, &CustomEditor::toggleTheme);

    // ----- Helpers : plier/déplier les panneaux latéraux -----
    // Les chevrons sont dans le header (toujours visibles), ‹/› indique l'état
    auto toggleLeftPanel = [this]() {
        bool nowVisible = !ui->leftPanel->isVisible();
        ui->leftPanel->setVisible(nowVisible);
        ui->chevronLeft->setText(nowVisible ? "\u2039" : "\u203A"); // ‹ ouvert, › fermé
    };

    auto toggleRightPanel = [this]() {
        bool nowVisible = !ui->rightPanel->isVisible();
        ui->rightPanel->setVisible(nowVisible);
        ui->chevronRight->setText(nowVisible ? "\u203A" : "\u2039"); // › ouvert, ‹ fermé
    };

    connect(ui->chevronLeft,  &QToolButton::clicked, this, toggleLeftPanel);
    connect(ui->chevronRight, &QToolButton::clicked, this, toggleRightPanel);

    // État initial : panneaux ouverts, flèches pointant vers l'extérieur
    ui->chevronLeft->setText("\u2039");   // ‹
    ui->chevronRight->setText("\u203A");  // ›

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
    m_touchSelectionPanel = new QFrame(this);
    m_touchSelectionPanel->setObjectName("touchSelectionPanel");
    m_touchSelectionPanel->setVisible(false);

    // 1. On fixe la hauteur max à 70 comme tu le souhaites (et un minimum pour la lisibilité)
    m_touchSelectionPanel->setMinimumHeight(50);
    m_touchSelectionPanel->setMaximumHeight(70);

    // 2. LE SECRET EST ICI : On force le panneau à ne pas s'étirer verticalement dans le layout parent
    m_touchSelectionPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    m_touchSelectionPanel->setStyleSheet(
        "QFrame#touchSelectionPanel {"
        " background: rgba(43, 122, 255, 0.10);"
        " border: 2px solid rgba(43, 122, 255, 0.35);"
        " border-radius: 14px;"
        "}"
        );

    auto *touchPanelLayout = new QHBoxLayout(m_touchSelectionPanel);
    touchPanelLayout->setContentsMargins(12, 8, 12, 8); // Marges légèrement augmentées sur les côtés
    touchPanelLayout->setSpacing(10);

    m_touchSelectionLabel = new QLabel(tr("Aucune selection"), m_touchSelectionPanel);
    // On enlève les minimum/maximum width trop stricts et on utilise les SizePolicy
    m_touchSelectionLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_touchSelectionLabel->setWordWrap(true);

    m_touchDuplicateButton = new QPushButton(tr("Dupli."), m_touchSelectionPanel);
    m_touchDeleteButton = new QPushButton(tr("Suppr."), m_touchSelectionPanel);
    m_touchWidthButton = new QPushButton(tr("Larg."), m_touchSelectionPanel);
    m_touchHeightButton = new QPushButton(tr("Haut."), m_touchSelectionPanel);
    m_touchRotateButton = new QPushButton(tr("Angle"), m_touchSelectionPanel);

    const QList<QPushButton*> touchButtons = {
        m_touchDuplicateButton, m_touchDeleteButton, m_touchWidthButton, m_touchHeightButton, m_touchRotateButton
    };

    for (QPushButton *button : touchButtons) {
        // 2. Taille dynamique pour les boutons
        button->setMinimumSize(70, 30); // 30px de hauteur est beaucoup plus standard
        // Permet au bouton de s'étirer verticalement pour remplir l'espace disponible
        button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    }

    auto *touchButtonsLayout = new QHBoxLayout();
    touchButtonsLayout->setSpacing(8);
    touchButtonsLayout->setContentsMargins(0, 0, 0, 0);

    for (QPushButton *button : touchButtons) {
        touchButtonsLayout->addWidget(button);
    }
    // On garde le stretch pour éviter que les boutons deviennent immenses sur les très grands écrans
    touchButtonsLayout->addStretch(1);

    // 3. Répartition de l'espace horizontal (Stretch factors : 1 pour le label, 2 pour les boutons)
    touchPanelLayout->addWidget(m_touchSelectionLabel, 1, Qt::AlignVCenter);
    touchPanelLayout->addLayout(touchButtonsLayout, 2);

    // Connexions ViewModel → View
    if (m_viewModel) {
        connect(m_viewModel, &CustomEditorViewModel::subpathsReady,
                this, [this](const QList<QPainterPath> &subs) {
            for (const QPainterPath &sp : subs)
                drawArea->addImportedLogoSubpath(sp);
        });
        connect(m_viewModel, &CustomEditorViewModel::importFailed,
                this, [this](const QString &msg) {
            QMessageBox::warning(this, tr("Erreur"), msg);
        });
    }

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
        dwLayout->addWidget(m_touchSelectionPanel);
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

    connect(drawArea, &CustomDrawArea::selectionStateChanged,
            this, &CustomEditor::updateTouchSelectionPanel);
    connect(m_touchDeleteButton, &QPushButton::clicked, drawArea, &CustomDrawArea::deleteSelectedShapes);
    connect(m_touchDuplicateButton, &QPushButton::clicked, drawArea, &CustomDrawArea::duplicateSelectedShapes);
    connect(m_touchWidthButton, &QPushButton::clicked, this, [this]() {
        if (!drawArea->hasSelection()) return;
        int value = qRound(drawArea->selectedShapesBounds().width());
        if (NumericKeyboardDialog::openNumericKeyboardDialog(this, value))
            drawArea->resizeSelectedShapes(value, drawArea->selectedShapesBounds().height());
    });
    connect(m_touchHeightButton, &QPushButton::clicked, this, [this]() {
        if (!drawArea->hasSelection()) return;
        int value = qRound(drawArea->selectedShapesBounds().height());
        if (NumericKeyboardDialog::openNumericKeyboardDialog(this, value))
            drawArea->resizeSelectedShapes(drawArea->selectedShapesBounds().width(), value);
    });
    connect(m_touchRotateButton, &QPushButton::clicked, this, [this]() {
        if (!drawArea->hasSelection()) return;
        int angle = 0;
        if (NumericKeyboardDialog::openNumericKeyboardDialog(this, angle))
            drawArea->rotateSelectedShapes(angle);
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
    connect(ui->buttonMenu, &QPushButton::clicked, this, &CustomEditor::goToMainWindow);

    // Bouton "Save" : enregistre la forme personnalisée
    connect(ui->buttonSave, &QPushButton::clicked, this, &CustomEditor::saveCustomShape);

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
    connect(ui->buttonCopyPaste, &QPushButton::clicked, this, &CustomEditor::onCopyPasteClicked);


    connect(ui->buttonCloseShape, &QPushButton::clicked, this, [this]() {
        drawArea->cancelSelection();
        drawArea->cancelDeplacerMode();
        drawArea->startCloseMode();
    });


    // ----------------- Fonctions complémentaires -----------------

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
    updateShapeButtonIcon(drawArea->getDrawMode());
    connect(drawArea, &CustomDrawArea::drawModeChanged, this,
            [this](CustomDrawArea::DrawMode m){
                updateShapeButtonIcon(m);
                ui->labelCurrentMode->setText(tr("Mode : %1").arg(modeToString(m)));
                if (auto mw = qobject_cast<QMainWindow*>(window()))
                    mw->statusBar()->showMessage(modeToString(m));
            });
    // Initialiser le label avec le mode actuel
    ui->labelCurrentMode->setText(tr("Mode : %1").arg(modeToString(drawArea->getDrawMode())));

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
        updateShapeButtonIcon(CustomDrawArea::DrawMode::Text);
        //qDebug() << "Mode Texte activé";
        fontContainer->show();
    });
    // Mode Texte fin : afficher fontContainer
    connect(actionThinText, &QAction::triggered, this, [=]() {
        drawArea->setDrawMode(CustomDrawArea::DrawMode::ThinText);
        updateShapeButtonIcon(CustomDrawArea::DrawMode::ThinText);
        fontContainer->show();
    });
    // Pour les autres modes, cacher fontContainer
    connect(actionAlaMain, &QAction::triggered, this, [=]() {
        drawArea->setDrawMode(CustomDrawArea::DrawMode::Freehand);
        updateShapeButtonIcon(CustomDrawArea::DrawMode::Freehand);
        //qDebug() << "Mode À la main activé";
        fontContainer->hide();
    });
    connect(actionPointParPoint, &QAction::triggered, this, [=]() {
        drawArea->setDrawMode(CustomDrawArea::DrawMode::PointParPoint);
        updateShapeButtonIcon(CustomDrawArea::DrawMode::PointParPoint);
        //qDebug() << "Mode Point par point activé";
        fontContainer->hide();
    });
    connect(actionLigne, &QAction::triggered, this, [=]() {
        drawArea->setDrawMode(CustomDrawArea::DrawMode::Line);
        updateShapeButtonIcon(CustomDrawArea::DrawMode::Line);
        //qDebug() << "Mode Ligne activé";
        fontContainer->hide();
    });
    connect(actionCercle, &QAction::triggered, this, [=]() {
        drawArea->setDrawMode(CustomDrawArea::DrawMode::Circle);
        updateShapeButtonIcon(CustomDrawArea::DrawMode::Circle);
        //qDebug() << "Mode Cercle activé";
        fontContainer->hide();
    });
    connect(actionRectangle, &QAction::triggered, this, [=]() {
        drawArea->setDrawMode(CustomDrawArea::DrawMode::Rectangle);
        updateShapeButtonIcon(CustomDrawArea::DrawMode::Rectangle);
        //qDebug() << "Mode Rectangle activé";
        fontContainer->hide();
    });

    // --- Connexions pour les autres boutons ---
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
                 << ui->buttonDeplacer->property("deplacerMode").toBool();
//        ui->buttonDeplacer->setText(enabled ? "DÉPLACER ✅" : "DÉPLACER");
        ui->buttonDeplacer->setStyleSheet(ui->buttonDeplacer->styleSheet());

        ui->buttonDeplacer->update();
        if (enabled) updateShapeButtonIcon(CustomDrawArea::DrawMode::Deplacer);
    });

    qDebug() << "[DEBUG] Connexion faite avec deplacerModeChanged";

    connect(ui->buttonSupprimer, &QPushButton::clicked, this, [this]() {
        if (drawArea->hasSelection()) {
            drawArea->deleteSelectedShapes();
            return;
        }

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
                if (enabled) updateShapeButtonIcon(CustomDrawArea::DrawMode::Supprimer);
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
                if (enabled) updateShapeButtonIcon(CustomDrawArea::DrawMode::Gomme);
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
    connect(actionImportLogo, &QAction::triggered, this, &CustomEditor::importerLogo);
    connect(actionImportImage, &QAction::triggered, this, &CustomEditor::importerImageCouleur);

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
        ui->labelGridSpacing->setText(tr("Taille de la grille : %1 px").arg(value));
    });

    // 1) valeur initiale dès le lancement
    drawArea->setEraserRadius(ui->tailleGomme->value());

    // 2) quand on déplace le curseur → on met à jour la gomme
    connect(ui->tailleGomme, &QSlider::valueChanged,
            this, [=](int v){
                drawArea->setEraserRadius(static_cast<qreal>(v));

    });



}

CustomEditor::~CustomEditor()
{
    delete ui;
}

void CustomEditor::goToMainWindow()
{
    this->close();
}

void CustomEditor::closeEditor()
{
    this->close();
    delete this;
}

void CustomEditor::openKeyboardDialog()
{
    QStringList names = m_viewModel ? m_viewModel->getAllShapeNames() : QStringList{};
    KeyboardDialog clavier(names, this);
    if (clavier.exec() == QDialog::Accepted) {
        QString texteSaisi = clavier.getText();
    }
}

void CustomEditor::saveCustomShape() {
    QList<QPolygonF> shapes = drawArea->getCustomShapes();
    if (shapes.isEmpty())
        return;

    bool ok = false;
    QString shapeName;
    do {
        shapeName = QInputDialog::getText(this, tr("Nom de la forme"),
                                          tr("Entrez un nom pour votre forme :"),
                                          QLineEdit::Normal, "", &ok);
        if (!ok)
            return;
        if (shapeName.isEmpty())
            continue;
        if (m_viewModel && m_viewModel->shapeNameExists(shapeName)) {
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

    if (m_viewModel)
        m_viewModel->saveShape(shapes, shapeName);
}

void CustomEditor::importerLogo()
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

    if (m_viewModel)
        m_viewModel->importLogo(filePath, includeInternal,
                                QSizeF(drawArea->width(), drawArea->height()));
}

void CustomEditor::importerImageCouleur()
{
    QString filePath = QFileDialog::getOpenFileName(
        this, tr("Sélectionner une image"),
        "",  tr("Images (*.png *.jpg *.bmp *.webp)"));
    if (filePath.isEmpty())
        return;

    if (m_viewModel)
        m_viewModel->importImageColor(filePath,
                                      QSizeF(drawArea->width(), drawArea->height()));
}

void CustomEditor::onCopyPasteClicked()
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

void CustomEditor::updateShapeButtonIcon(CustomDrawArea::DrawMode mode)
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

void CustomEditor::updateTouchSelectionPanel(bool hasSelection, const QString &summary)
{
    if (!m_touchSelectionPanel || !m_touchSelectionLabel) return;

    m_touchSelectionPanel->setVisible(hasSelection);
    m_touchSelectionLabel->setText(summary);
    ui->labelCurrentMode->setText(hasSelection
                                      ? tr("Selection tactile")
                                      : tr("Mode : %1").arg(modeToString(drawArea->getDrawMode())));
}

void CustomEditor::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QWidget::changeEvent(event);
}

void CustomEditor::applyStyleSheets()
{
    updateThemeButton();
}

void CustomEditor::updateThemeButton()
{
    if (!ui->buttonTheme) return;
    ui->buttonTheme->setIcon(QIcon(m_isDarkTheme ? ":/icons/moon.svg" : ":/icons/sun.svg"));
}

void CustomEditor::toggleTheme()
{
    ThemeManager::instance()->toggle();
}

void CustomEditor::applyTheme(bool isDark)
{
    m_isDarkTheme = isDark;
    applyStyleSheets();
}
