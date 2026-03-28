#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ShapeModel.h"
#include "Language.h"
#include <QList>
#include <QPolygonF>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QAction;
class QMenu;
struct LayoutData;
class ShapeVisualization;
class MainWindowCoordinator;
class WorkspaceViewModel;
class MainWindowViewModel;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr,
                        MainWindowCoordinator *coordinator = nullptr,
                        WorkspaceViewModel *model = nullptr);
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
    void applyBaseShapeLayout(ShapeModel::Type type, const LayoutData &layout);

    // --- Signaux émis par la View vers le Controller ---
signals:
    // Contrôle de coupe
    void requestStartCut();
    void requestPauseCut();
    void requestStopCut();

    // Dimensions (la View émet, le Controller réagit)
    void dimensionsChangeRequested(int largeur, int longueur);
    void shapeCountChangeRequested(int count);
    void spacingChangeRequested(int spacing);

    // Formes prédéfinies
    void circleRequested();
    void rectangleRequested();
    void triangleRequested();
    void starRequested();
    void heartRequested();

    // Navigation
    void inventoryRequested();
    void customEditorRequested();
    void folderRequested();
    void testGpioRequested();
    void bluetoothReceiverRequested();
    void wifiTransferRequested();

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

private slots:
    void setLanguageFrench();
    void setLanguageEnglish();

protected:
    void changeEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void setupUI();
    void setupWorkspaceLayout();
    void setupMenus();
    void applyStyleSheets();
    void setupViewConnections();
    void setupModels();
    void retranslateDynamicUi();
    void syncControlsFromModel();

    // --- UI state ---
    QMenu   *settingsMenu    = nullptr;
    QMenu   *languageMenu    = nullptr;
    QAction *actionFrench    = nullptr;
    QAction *actionEnglish   = nullptr;
    QAction *actionWifiConfig = nullptr;

    // --- UI pointers ---
    Ui::MainWindow     *ui                 = nullptr;
    ShapeVisualization *shapeVisualization  = nullptr;

    // --- Données centralisées ---
    WorkspaceViewModel *m_model = nullptr;

    // --- Coordinator et ViewModel (seuls contacts autorisés depuis la View) ---
    MainWindowCoordinator *m_coordinator    = nullptr;
    MainWindowViewModel   *m_viewModel      = nullptr;
    bool                   m_ownsCoordinator = false;
};

#endif // MAINWINDOW_H
