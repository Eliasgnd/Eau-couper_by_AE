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
#include <QDoubleSpinBox>
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
#include <QGridLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <cmath>
#include "ScreenUtils.h"
#include <QStatusBar>
#include <functional>

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
    m_assistanceBar = new QFrame(this);
    m_assistanceBar->setObjectName("editorAssistanceBar");
    m_assistanceBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    auto *assistanceLayout = new QHBoxLayout(m_assistanceBar);
    assistanceLayout->setContentsMargins(14, 10, 14, 10);
    assistanceLayout->setSpacing(10);

    // --- Création d'un conteneur sombre et arrondi pour les textes ---
    QFrame *textContainer = new QFrame(m_assistanceBar);

    // NOUVEAU : On dit au conteneur de s'étendre, mais sans forcer
    textContainer->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    textContainer->setStyleSheet(
        "QFrame { "
        "   background-color: rgba(15, 23, 42, 210); "
        "   border-radius: 12px; "
        "}"
        "QLabel#hudTitle { "
        "   color: white; "
        "   font-weight: bold; "
        "   font-size: 14px; "
        "}"
        "QLabel#hudHint { "
        "   color: #e2e8f0; " // Gris clair
        "}"
        "QLabel#hudDetail { "
        "   color: #7dd3fc; " // Bleu clair
        "}"
        );

    auto *assistanceTextLayout = new QVBoxLayout(textContainer);
    assistanceTextLayout->setContentsMargins(12, 8, 12, 8); // Marges internes
    assistanceTextLayout->setSpacing(2);

    m_assistanceTitle = new QLabel(tr("Mode : Trace libre"), textContainer);
    m_assistanceTitle->setObjectName("hudTitle");

    m_assistanceHint = new QLabel(tr("Glissez le doigt pour dessiner librement."), textContainer);
    m_assistanceHint->setObjectName("hudHint");
    m_assistanceHint->setWordWrap(true);
    // NOUVEAU : Le secret est ici. Ça empêche le texte de pousser les boutons !
    m_assistanceHint->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    m_assistanceDetail = new QLabel(tr("Grille visible  |  Aimant OFF  |  Contrainte OFF"), textContainer);
    m_assistanceDetail->setObjectName("hudDetail");
    m_assistanceDetail->setWordWrap(true);
    // NOUVEAU : Pareil ici.
    m_assistanceDetail->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    assistanceTextLayout->addWidget(m_assistanceTitle);
    assistanceTextLayout->addWidget(m_assistanceHint);
    assistanceTextLayout->addWidget(m_assistanceDetail);

    // --- RECRÉATION DES BOUTONS (avec leur taille minimum pour les protéger) ---
    m_precisionConstraintButton = new QPushButton(tr("Contrainte OFF"), m_assistanceBar);
    m_precisionConstraintButton->setMinimumSize(130, 42);

    m_segmentStatusButton = new QPushButton(tr("Segments OFF"), m_assistanceBar);
    m_segmentStatusButton->setMinimumSize(150, 42);

    m_cancelModeButton = new QPushButton(tr("Quitter l'outil"), m_assistanceBar);
    m_cancelModeButton->setMinimumSize(120, 42);

    m_undoPointButton = new QPushButton(tr("Annuler point"), m_assistanceBar);
    m_undoPointButton->setMinimumSize(120, 42);
    m_undoPointButton->setVisible(false);

    m_undoSegmentButton = new QPushButton(tr("Annuler segment"), m_assistanceBar);
    m_undoSegmentButton->setMinimumSize(130, 42);
    m_undoSegmentButton->setVisible(false);

    m_finishPointPathButton = new QPushButton(tr("Terminer trace"), m_assistanceBar);
    m_finishPointPathButton->setMinimumSize(130, 42);
    m_finishPointPathButton->setVisible(false);

    // --- AJOUT AU BANDEAU ---
    // Le '1' donne tout l'espace libre au texte sombre, et les boutons gardent leur taille fixe à droite.
    assistanceLayout->addWidget(textContainer, 1);
    assistanceLayout->addWidget(m_precisionConstraintButton);
    assistanceLayout->addWidget(m_segmentStatusButton);
    assistanceLayout->addWidget(m_undoPointButton);
    assistanceLayout->addWidget(m_undoSegmentButton);
    assistanceLayout->addWidget(m_finishPointPathButton);
    assistanceLayout->addWidget(m_cancelModeButton);

    m_selectionActionsWidget = new QWidget(this);
    m_selectionActionsWidget->setVisible(false);
    m_selectionActionsWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    m_selectionInspectorWidget = new QFrame(this);
    m_selectionInspectorWidget->setObjectName("selectionInspector");
    m_selectionInspectorWidget->setVisible(false);
    m_selectionInspectorWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    auto *inspectorLayout = new QVBoxLayout(m_selectionInspectorWidget);
    inspectorLayout->setContentsMargins(10, 10, 10, 10);
    inspectorLayout->setSpacing(8);
    m_selectionCountLabel = new QLabel(tr("Aucune selection"), m_selectionInspectorWidget);
    m_selectionCountLabel->setObjectName("selectionInspectorTitle");
    inspectorLayout->addWidget(m_selectionCountLabel);

    auto *formLayout = new QFormLayout();
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setSpacing(6);
    auto makeSelectionSpin = [this]() {
        auto *spin = new QDoubleSpinBox(m_selectionInspectorWidget);
        spin->setRange(-100000.0, 100000.0);
        spin->setDecimals(1);
        spin->setSingleStep(1.0);
        spin->setMinimumHeight(34);
        spin->setKeyboardTracking(false);
        return spin;
    };
    m_selectionXSpin = makeSelectionSpin();
    m_selectionYSpin = makeSelectionSpin();
    m_selectionWidthSpin = makeSelectionSpin();
    m_selectionHeightSpin = makeSelectionSpin();
    m_selectionRotationSpin = makeSelectionSpin();
    m_selectionWidthSpin->setMinimum(1.0);
    m_selectionHeightSpin->setMinimum(1.0);
    m_selectionRotationSpin->setRange(-360.0, 360.0);
    m_selectionRotationSpin->setSingleStep(15.0);

    formLayout->addRow(tr("X"), m_selectionXSpin);
    formLayout->addRow(tr("Y"), m_selectionYSpin);
    formLayout->addRow(tr("Largeur"), m_selectionWidthSpin);
    formLayout->addRow(tr("Hauteur"), m_selectionHeightSpin);
    formLayout->addRow(tr("Rotation"), m_selectionRotationSpin);
    inspectorLayout->addLayout(formLayout);

    auto *viewButtonsLayout = new QHBoxLayout();
    viewButtonsLayout->setContentsMargins(0, 0, 0, 0);
    viewButtonsLayout->setSpacing(6);
    m_zoomSelectionButton = new QPushButton(tr("Zoom selection"), m_selectionInspectorWidget);
    m_fitDrawingButton = new QPushButton(tr("Voir tout"), m_selectionInspectorWidget);
    m_zoomSelectionButton->setMinimumHeight(38);
    m_fitDrawingButton->setMinimumHeight(38);
    viewButtonsLayout->addWidget(m_zoomSelectionButton);
    viewButtonsLayout->addWidget(m_fitDrawingButton);
    inspectorLayout->addLayout(viewButtonsLayout);

    // 1. On fixe la hauteur max à 70 comme tu le souhaites (et un minimum pour la lisibilité)
    auto *selectionToolsLayout = new QVBoxLayout(m_selectionActionsWidget);
    selectionToolsLayout->setContentsMargins(0, 4, 0, 2);
    selectionToolsLayout->setSpacing(6);

    // 2. LE SECRET EST ICI : On force le panneau à ne pas s'étirer verticalement dans le layout parent



    m_touchDuplicateButton = new QPushButton(tr("  Dupliquer"), m_selectionActionsWidget);
    m_touchDeleteButton = new QPushButton(tr("  Supprimer"), m_selectionActionsWidget);

    // 1. Boutons principaux toujours visibles (Garde ceux que tu avais déjà dans ton .h)

    // 2. Menu : Alignement & actions rapides
    QPushButton *btnAlign = new QPushButton(tr("  Alignement ▾"), m_selectionActionsWidget);
    m_alignMenuButton = btnAlign;
    QMenu *menuAlign = new QMenu(this);
    QAction *actionAlignH = new QAction(tr("Aligner au centre H"), this);
    QAction *actionAlignV = new QAction(tr("Aligner au centre V"), this);
    QAction *actionCenter = new QAction(tr("Centrer sur l'écran"), this);
    menuAlign->addAction(actionAlignH);
    menuAlign->addAction(actionAlignV);
    menuAlign->addSeparator();
    menuAlign->addAction(actionCenter);
    btnAlign->setMenu(menuAlign);

    m_touchNudgeLeftButton = new QPushButton(tr("←"), m_selectionActionsWidget);
    m_touchNudgeRightButton = new QPushButton(tr("→"), m_selectionActionsWidget);
    m_touchNudgeUpButton = new QPushButton(tr("↑"), m_selectionActionsWidget);
    m_touchNudgeDownButton = new QPushButton(tr("↓"), m_selectionActionsWidget);

    // Regroupement pour appliquer le style et la taille
    const QList<QPushButton*> selectionButtons = {
        m_touchDuplicateButton, m_touchDeleteButton, btnAlign
    };

    auto *nudgeLayout = new QHBoxLayout();
    nudgeLayout->setSpacing(6);
    nudgeLayout->setContentsMargins(0, 0, 0, 0);

    for (QPushButton *btn : selectionButtons) {
        btn->setMinimumSize(0, 44);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        selectionToolsLayout->addWidget(btn);
    }
    const QList<QPushButton*> nudgeButtons = {
        m_touchNudgeLeftButton, m_touchNudgeUpButton, m_touchNudgeDownButton, m_touchNudgeRightButton
    };
    for (QPushButton *btn : nudgeButtons) {
        btn->setMinimumSize(0, 38);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        nudgeLayout->addWidget(btn);
    }

    // Répartition de l'espace horizontal
    selectionToolsLayout->addLayout(nudgeLayout);

    // Connexions ViewModel → View
    if (m_viewModel) {
        connect(m_viewModel, &CustomEditorViewModel::subpathsReady,
                this, [this](const QList<QPainterPath> &subs) {
            drawArea->addImportedLogoSubpaths(subs);
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
        dwLayout->addWidget(m_assistanceBar);
        dwLayout->addWidget(drawArea);
    } else {
        //qDebug() << "Erreur : ui->drawingWidget est nullptr !";
    }

    if (ui->leftPanel && ui->leftPanel->layout()) {
        if (auto *leftLayout = qobject_cast<QVBoxLayout*>(ui->leftPanel->layout())) {
            const int insertIndex = qMax(0, leftLayout->count() - 1);
            leftLayout->insertWidget(insertIndex, m_selectionActionsWidget);
            leftLayout->insertWidget(insertIndex, m_selectionInspectorWidget);
        }
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
    });

    if (ui->buttonSupprimer)
        ui->buttonSupprimer->setVisible(false);

    connect(drawArea, &CustomDrawArea::selectionStateChanged,
            this, &CustomEditor::updateTouchSelectionPanel);
    connect(drawArea, &CustomDrawArea::canvasStatusChanged,
            this, &CustomEditor::updateCanvasStatus);
    connect(drawArea, &CustomDrawArea::historyStateChanged,
            this, &CustomEditor::updateHistoryButtons);
    // Connexions des boutons principaux
    connect(m_touchDuplicateButton, &QPushButton::clicked, drawArea, &CustomDrawArea::duplicateSelectedShapes);
    connect(m_touchDeleteButton, &QPushButton::clicked, drawArea, &CustomDrawArea::deleteSelectedShapes);
    connect(m_zoomSelectionButton, &QPushButton::clicked, drawArea, &CustomDrawArea::zoomToSelection);
    connect(m_fitDrawingButton, &QPushButton::clicked, drawArea, &CustomDrawArea::fitAllShapesInView);

    const auto connectSelectionSpin = [this](QDoubleSpinBox *spin, const std::function<void(double)> &handler) {
        connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
                [this, handler](double value) {
            if (m_updatingSelectionInspector || !drawArea || !drawArea->hasSelection()) return;
            handler(value);
            refreshSelectionInspector();
        });
    };
    connectSelectionSpin(m_selectionXSpin, [this](double value) {
        drawArea->setSelectedShapesPosition(value, drawArea->selectedShapesBounds().top());
    });
    connectSelectionSpin(m_selectionYSpin, [this](double value) {
        drawArea->setSelectedShapesPosition(drawArea->selectedShapesBounds().left(), value);
    });
    connectSelectionSpin(m_selectionWidthSpin, [this](double value) {
        drawArea->resizeSelectedShapes(value, drawArea->selectedShapesBounds().height());
    });
    connectSelectionSpin(m_selectionHeightSpin, [this](double value) {
        drawArea->resizeSelectedShapes(drawArea->selectedShapesBounds().width(), value);
    });
    connectSelectionSpin(m_selectionRotationSpin, [this](double value) {
        drawArea->setSelectedRotation(value);
    });

    connect(actionCenter, &QAction::triggered, drawArea, &CustomDrawArea::centerSelectionInViewport);

    // Connexions du Menu "Alignement"
    connect(actionAlignH, &QAction::triggered, drawArea, &CustomDrawArea::alignSelectedHCenter);
    connect(actionAlignV, &QAction::triggered, drawArea, &CustomDrawArea::alignSelectedVCenter);

    // Connexions de deplacement rapide
    connect(m_touchNudgeLeftButton, &QPushButton::clicked, this, [this]() { drawArea->moveSelectedShapes(-5.0, 0.0, tr("Nudge gauche")); });
    connect(m_touchNudgeRightButton, &QPushButton::clicked, this, [this]() { drawArea->moveSelectedShapes(5.0, 0.0, tr("Nudge droite")); });
    connect(m_touchNudgeUpButton, &QPushButton::clicked, this, [this]() { drawArea->moveSelectedShapes(0.0, -5.0, tr("Nudge haut")); });
    connect(m_touchNudgeDownButton, &QPushButton::clicked, this, [this]() { drawArea->moveSelectedShapes(0.0, 5.0, tr("Nudge bas")); });

    connect(m_cancelModeButton, &QPushButton::clicked, drawArea, &CustomDrawArea::cancelActiveModes);
    connect(m_undoPointButton, &QPushButton::clicked, drawArea, &CustomDrawArea::undoPointByPointPoint);
    connect(m_undoSegmentButton, &QPushButton::clicked, drawArea, &CustomDrawArea::undoPointByPointSegment);
    connect(m_finishPointPathButton, &QPushButton::clicked, drawArea, &CustomDrawArea::finishPointByPointShape);
    connect(ui->buttonRetablir, &QPushButton::clicked, drawArea, &CustomDrawArea::redoLastAction);
    connect(m_precisionConstraintButton, &QPushButton::clicked, this, [this]() {
        drawArea->setPrecisionConstraintEnabled(!drawArea->isPrecisionConstraintEnabled());
        refreshModeButtons();
    });
    connect(m_segmentStatusButton, &QPushButton::clicked, this, [this]() {
        drawArea->setSegmentStatusVisible(!drawArea->isSegmentStatusVisible());
        refreshModeButtons();
    });

    // Bouton "Appliquer" : émission du signal avec les formes personnalisées puis fermeture
    connect(ui->Appliquer, &QPushButton::clicked, this, [this]() {
        //qDebug() << "Signal applyCustomShapeSignal émis avec les formes !";
        const QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            tr("Verifier avant application"),
            drawArea->validationSummary(),
            QMessageBox::Yes | QMessageBox::No,
            drawArea->hasValidationIssues() ? QMessageBox::No : QMessageBox::Yes);
        if (reply != QMessageBox::Yes)
            return;
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

                ui->buttonGomme->style()->unpolish(ui->buttonGomme);
                ui->buttonGomme->style()->polish(ui->buttonGomme);
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
        refreshModeButtons();
    });

    connect(ui->buttonToggleGrid, &QPushButton::clicked, this, [=]() {
        bool visible = !drawArea->isGridVisible();
        drawArea->setGridVisible(visible);

        ui->buttonToggleGrid->setText(visible ? "Grille ✅" : "Grille ❌");
        ui->buttonToggleGrid->setProperty("gridVisible", visible);

        ui->buttonToggleGrid->style()->unpolish(ui->buttonToggleGrid);
        ui->buttonToggleGrid->style()->polish(ui->buttonToggleGrid);
        ui->buttonToggleGrid->update();
        refreshModeButtons();
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

    updateCanvasStatus(tr("Mode : %1").arg(modeToString(drawArea->getDrawMode())),
                       tr("Glissez le doigt pour dessiner librement."),
                       tr("Grille visible  |  Aimant OFF  |  Contrainte OFF"));
    updateHistoryButtons(false, QString(), false, QString());
    applyStyleSheets();
    refreshModeButtons();

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
    Q_UNUSED(summary);
    if (!m_selectionActionsWidget) return;

    m_selectionActionsWidget->setVisible(hasSelection);
    if (m_selectionInspectorWidget)
        m_selectionInspectorWidget->setVisible(hasSelection);
    if (ui->buttonConnect)
        ui->buttonConnect->setVisible(!hasSelection);
    if (ui->buttonCloseShape)
        ui->buttonCloseShape->setVisible(!hasSelection);
    if (hasSelection && m_assistanceTitle)
        m_assistanceTitle->setText(tr("Selection tactile"));
    refreshSelectionInspector();
}

void CustomEditor::refreshSelectionInspector()
{
    if (!drawArea || !m_selectionInspectorWidget) return;

    const bool hasSelection = drawArea->hasSelection();
    m_selectionInspectorWidget->setVisible(hasSelection);
    if (!hasSelection) return;

    const QRectF bounds = drawArea->selectedShapesBounds();
    m_updatingSelectionInspector = true;
    if (m_selectionCountLabel)
        m_selectionCountLabel->setText(tr("%1 forme(s) selectionnee(s)").arg(drawArea->selectedShapesCount()));
    if (m_selectionXSpin) m_selectionXSpin->setValue(bounds.left());
    if (m_selectionYSpin) m_selectionYSpin->setValue(bounds.top());
    if (m_selectionWidthSpin) m_selectionWidthSpin->setValue(qMax<qreal>(1.0, bounds.width()));
    if (m_selectionHeightSpin) m_selectionHeightSpin->setValue(qMax<qreal>(1.0, bounds.height()));
    if (m_selectionRotationSpin) m_selectionRotationSpin->setValue(drawArea->selectedRotationAngle());
    m_updatingSelectionInspector = false;
}

void CustomEditor::updateCanvasStatus(const QString &modeLabel, const QString &hint, const QString &detail)
{
    if (m_assistanceTitle)  m_assistanceTitle->setText(modeLabel);
    if (m_assistanceHint)   m_assistanceHint->setText(hint);
    if (m_assistanceDetail) m_assistanceDetail->setText(detail);
    if (ui->labelCurrentMode) ui->labelCurrentMode->setText(modeLabel);
    refreshModeButtons();
}

void CustomEditor::updateHistoryButtons(bool canUndo, const QString &undoText,
                                        bool canRedo, const QString &redoText)
{
    if (ui->buttonRetour) {
        ui->buttonRetour->setEnabled(canUndo);
        ui->buttonRetour->setToolTip(canUndo && !undoText.isEmpty() ? undoText : tr("Annuler la derniere action"));
    }
    if (ui->buttonRetablir) {
        ui->buttonRetablir->setEnabled(canRedo);
        ui->buttonRetablir->setToolTip(canRedo && !redoText.isEmpty() ? redoText : tr("Retablir la derniere action"));
    }
}

void CustomEditor::refreshModeButtons()
{
    const bool pointMode = drawArea && drawArea->getDrawMode() == CustomDrawArea::DrawMode::PointParPoint;
    if (m_cancelModeButton)
    {
        m_cancelModeButton->setVisible(!pointMode);
        m_cancelModeButton->setEnabled(drawArea && drawArea->hasActiveSpecialMode());
    }
    if (m_precisionConstraintButton)
    {
        m_precisionConstraintButton->setVisible(!pointMode);
        m_precisionConstraintButton->setText(drawArea && drawArea->isPrecisionConstraintEnabled()
                                                 ? tr("Contrainte ON")
                                                 : tr("Contrainte OFF"));
    }
    if (m_segmentStatusButton)
        m_segmentStatusButton->setText(drawArea && drawArea->isSegmentStatusVisible()
                                           ? tr("Segments ON")
                                           : tr("Segments OFF"));
    if (m_undoPointButton)
        m_undoPointButton->setVisible(pointMode);
    if (m_undoSegmentButton)
        m_undoSegmentButton->setVisible(pointMode);
    if (m_finishPointPathButton)
        m_finishPointPathButton->setVisible(pointMode);
    refreshSelectionInspector();

    const auto repolish = [](QWidget *widget) {
        if (!widget) return;
        widget->style()->unpolish(widget);
        widget->style()->polish(widget);
        widget->update();
    };

    if (ui->buttonGomme) {
        ui->buttonGomme->setProperty("gommeMode", drawArea && drawArea->isGommeMode());
        repolish(ui->buttonGomme);
    }
    if (ui->buttonSelection) {
        ui->buttonSelection->setProperty("closeMode", drawArea && drawArea->hasSelection());
        repolish(ui->buttonSelection);
    }
    if (ui->buttonConnect)
    {
        ui->buttonConnect->setVisible(!(drawArea && drawArea->hasSelection()));
        repolish(ui->buttonConnect);
    }
    if (ui->buttonCloseShape)
    {
        ui->buttonCloseShape->setVisible(!(drawArea && drawArea->hasSelection()));
        repolish(ui->buttonCloseShape);
    }
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
    if (m_assistanceBar) {
        m_assistanceBar->setStyleSheet(
            "QFrame#editorAssistanceBar {"
            " background: rgba(15, 23, 42, 0.08);"
            " border: 1px solid rgba(43, 122, 255, 0.22);"
            " border-radius: 14px;"
            "}"
        );
    }
    if (m_selectionInspectorWidget) {
        m_selectionInspectorWidget->setStyleSheet(
            "QFrame#selectionInspector {"
            " background: rgba(15, 23, 42, 0.06);"
            " border: 1px solid rgba(43, 122, 255, 0.24);"
            " border-radius: 10px;"
            "}"
            "QLabel#selectionInspectorTitle {"
            " font-weight: bold;"
            "}"
        );
    }
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
