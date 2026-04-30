#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QPushButton>
#include "Language.h"
#include "StmProtocol.h"
#include <QList>
#include <QPolygonF>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QAction;
class QMenu;
class QResizeEvent;
class QTabWidget;
struct LayoutData;
class ShapeVisualization;
class MainWindowCoordinator;
class WorkspaceViewModel;
class MainWindowViewModel;
class Inventory;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr,
                        MainWindowCoordinator *coordinator = nullptr,
                        WorkspaceViewModel *model = nullptr,
                        Inventory *inventory = nullptr);
    ~MainWindow();

    ShapeVisualization* getShapeVisualization() const;
    Language            displayLanguage() const;

    // --- Méthodes appelées par le Coordinator (via slots) ---
public slots:
    void updateProgressBar(int percentage, const QString &remainingTimeText);
    void setSpinboxSliderEnabled(bool enabled);
    void onCutFinished(bool success);
    void onLanguageApplied(Language lang, bool ok);
    void showAiProgressBar();
    void hideAiProgressBar();
    void applySelectedLayoutToControls(const LayoutData &layout);
    void displayCustomShapes(const QList<QPolygonF> &shapes, const QString &name = QString());
    void applyLayout(const LayoutData &layout);
    void displayBaseShapeLayout(const QList<QPolygonF> &polys, const QString &name, const LayoutData &layout);
    void refreshQuickShapeButtons();

    // --- Signaux émis par la View vers le Controller ---
signals:
    // Contrôle de coupe
    void requestStartCut();
    void requestPauseCut();
    void requestStopCut();
    void requestSpeedChange(int speed_mm_s);  // Émis par SpinBox_vitesse

    // Dimensions (la View émet, le Controller réagit)
    void dimensionsChangeRequested(int largeur, int longueur);
    void cutSurfaceChangeRequested(int widthMm, int heightMm, int xMm, int yMm);
    void cutSurfaceEditModeChanged(bool editing);
    void shapeCountChangeRequested(int count);
    void spacingChangeRequested(int spacing);

    // Formes prédéfinies
    void circleRequested();
    void rectangleRequested();
    void triangleRequested();
    void starRequested();
    void heartRequested();
    void baseShapeRequested(int type);
    void customShapeRequested(const QString &name);

    // Navigation
    void inventoryRequested();
    void customEditorRequested();
    void folderRequested();
    void wifiTransferRequested();
    void stmTestRequested();

    // Optimisation
    void optimizePlacement1Requested(bool checked);
    void optimizePlacement2Requested(bool checked);

    // Déplacement / rotation / ajout / suppression
    void moveUpRequested();
    void moveDownRequested();
    void moveLeftRequested();
    void moveRightRequested();
    void rotateLeftRequested();
    void rotateRightRequested();
    void addShapeRequested();
    void deleteShapeRequested();

    // Sauvegarde
    void saveLayoutRequested();

    // AI
    void generateAiRequested();
    void requestAiGeneration(const QString &prompt,
                             const QString &model,
                             const QString &quality,
                             const QString &size,
                             bool colorPrompt);

    // Langue
    void requestLanguageChange(Language lang);

    // Commandes machine secondaires (status bar)
    void homeRequested();
    void positionResetRequested();
    void rearmRequested();

private slots:
    void onMachineStateChanged(MachineState state);
    void setLanguageFrench();
    void setLanguageEnglish();
    void toggleTheme();

protected:
    void changeEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void setupUI();
    void setupWorkspaceLayout();
    void setupMenus();
    void applyStyleSheets();
    void updateThemeButton();
    void setupViewConnections();
    void setupModels();
    void retranslateDynamicUi();
    void syncControlsFromModel();
    void rebuildQuickShapeButtons();
    int quickShapeCapacity() const;
    void clearQuickShapeButtons();
    void updatePositionLabelDisplay();
    void setupSurfaceEditTabs();
    void setSurfaceEditMode(bool editing);
    void syncSurfaceControlsFromVisualization();
    void updateSurfaceControlLimits();
    void emitSurfaceChangeFromControls();

    // --- Theme state ---
    bool m_isDarkTheme = false;

    // --- Label state ---
    double m_lastHeadX = 0.0;
    double m_lastHeadY = 0.0;
    int m_lastShapeCount = 0;
    bool m_isCutting = false;
    bool m_surfaceEditMode = false;

    // --- UI state ---
    QMenu   *settingsMenu    = nullptr;
    QMenu   *languageMenu    = nullptr;
    QAction *actionFrench    = nullptr;
    QAction *actionEnglish   = nullptr;
    QAction *actionWifiConfig = nullptr;

    // --- UI pointers ---
    Ui::MainWindow     *ui                 = nullptr;
    ShapeVisualization *shapeVisualization  = nullptr;
    QTimer             *m_emergencyFlashTimer = nullptr;
    QTabWidget         *m_surfaceEditTabs = nullptr;
    QWidget            *m_surfaceEditPage = nullptr;
    QWidget            *m_shapeEditPage = nullptr;

    // --- Données centralisées ---
    WorkspaceViewModel *m_model = nullptr;

    // --- Coordinator et ViewModel (seuls contacts autorisés depuis la View) ---
    MainWindowCoordinator *m_coordinator    = nullptr;
    MainWindowViewModel   *m_viewModel      = nullptr;
    bool                   m_ownsCoordinator = false;
    Inventory             *m_inventory      = nullptr;
    QList<QPushButton*>    m_quickShapeButtons;
};

#endif // MAINWINDOW_H
