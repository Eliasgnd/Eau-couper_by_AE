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
class QLabel;
class FormeVisualization;
class CustomDrawArea;
class NavigationController;
class AIServiceManager;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    FormeVisualization* getFormeVisualization() const;
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
    void requestWifiConfig();
    void requestBluetoothReceiver();
    void requestOpenTestGpio();
    void requestOpenDossier();

public slots:
    void updateProgressBar(int percentage, const QString &remainingTimeText);
    void setSpinboxSliderEnabled(bool enabled);
    void onCutFinished(bool success);
    void onAiGenerationStatus(const QString &msg);
    void onAiImageReady(const QString &path);
    void onLanguageApplied(Language lang, bool ok);

private slots:
    void updateForme();
    void changeToCircle();
    void changeToRectangle();
    void changeToTriangle();
    void changeToStar();
    void changeToHeart();
    void showInventaire();
    void showCustom();
    void openTestGpio();
    void on_receptionFichierButton_clicked();
    void openWifiTransfer();
    void openWifiConfig();
    void applyCustomShape(QList<QPolygonF>);
    void resetDrawing();

    void updateSpinBoxLargeur(int value);
    void updateSpinBoxLongueur(int value);
    void updateSliderLongueur(int value);
    void updateSliderLargeur(int value);
    void updateShapeCount();
    void updateShapeCountLabel(int count);
    void updateSpacing(int value);
    void onCustomShapeSelected(const QList<QPolygonF> &polygons, const QString &name);

    void openAIImagePromptDialog();
    void showDossier();

    void setLanguageFrench();
    void setLanguageEnglish();

protected:
    void changeEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void setupUI();
    void setupConnections();
    void setupNavigationConnections();
    void setupShapeConnections();
    void setupSystemConnections();
    void setupModels();
    void retranslateDynamicUi();
    void onShapeSelectedFromInventaire(ShapeModel::Type type);
    void StartPixel();
    bool promptAndSaveCurrentCustomShape();

    Language m_displayLanguage = Language::French;
    QMenu *settingsMenu = nullptr;
    QMenu *languageMenu = nullptr;
    QAction *actionFrench = nullptr;
    QAction *actionEnglish = nullptr;
    QAction *actionWifiConfig = nullptr;

    Ui::MainWindow *ui;
    FormeVisualization *formeVisualization = nullptr;
    CustomDrawArea *drawArea = nullptr;
    NavigationController *m_navigationController = nullptr;
    AIServiceManager *m_aiServiceManager = nullptr;
    ShapeModel::Type selectedShapeType = ShapeModel::Type::Circle;
    QLabel *shapeCountLabel = nullptr;
};

#endif // MAINWINDOW_H
