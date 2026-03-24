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
class NavigationController;
class AIServiceManager;
class ShapeController;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    ShapeVisualization* getShapeVisualization() const;
    AIServiceManager* aiServiceManager() const { return m_aiServiceManager; }
    Language displayLanguage() const { return m_displayLanguage; }
    void openImageInCustom(const QString &filePath,
                           bool internalContours = false,
                           bool colorEdges = false);

signals:
    void requestStartCut();
    void requestPauseCut();
    void requestStopCut();
    void requestAiGeneration(const QString &prompt,
                             const QString &model,
                             const QString &quality,
                             const QString &size,
                             bool colorPrompt);
    void requestLanguageChange(Language lang);

public slots:
    void updateProgressBar(int percentage, const QString &remainingTimeText);
    void setSpinboxSliderEnabled(bool enabled);
    void onCutFinished(bool success);
    void onLanguageApplied(Language lang, bool ok);

private slots:
    void applyCustomShape(QList<QPolygonF>);
    void resetDrawing();
    void updateShapeCount();
    void updateShapeCountLabel(int count);
    void updateSpacing(int value);
    void onCustomShapeSelected(const QList<QPolygonF> &polygons, const QString &name);

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
    void setupConnections();
    void setupNavigationConnections();
    void setupShapeConnections();
    void setupSystemConnections();
    void setupModels();
    void retranslateDynamicUi();
    void onShapeSelectedFromInventory(ShapeModel::Type type);
    void applySelectedLayoutToControls(const LayoutData &layout);

    // --- UI state ---
    QMenu *settingsMenu = nullptr;
    QMenu *languageMenu = nullptr;
    QAction *actionFrench = nullptr;
    QAction *actionEnglish = nullptr;
    QAction *actionWifiConfig = nullptr;

    // --- UI pointers ---
    Ui::MainWindow *ui;
    ShapeVisualization *shapeVisualization = nullptr;

    // --- Controllers ---
    NavigationController *m_navigationController = nullptr;
    AIServiceManager *m_aiServiceManager = nullptr;
    ShapeController *m_shapeController = nullptr;

    // --- Runtime state ---
    Language m_displayLanguage = Language::French;
};

#endif // MAINWINDOW_H
