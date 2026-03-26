#pragma once

#include <QObject>
#include <QList>
#include <QPolygonF>
#include <QString>
#include <QTranslator>

#include "Language.h"
#include "AIServiceManager.h"
#include "ShapeModel.h"
#include "NavigationController.h"
#include "ShapeController.h"
#include "WorkspaceModel.h"

class MainWindow; // Forward declaration
class ShapeVisualization;
class TrajetMotor;
class OpenAIService;

class MainWindowCoordinator : public QObject {
    Q_OBJECT

public:
    explicit MainWindowCoordinator(NavigationController *navigationController,
                                   AIServiceManager *aiServiceManager,
                                   ShapeController *shapeController,
                                   WorkspaceModel *model,
                                   QObject *parent = nullptr);

    NavigationController *navigationController() const { return m_navigationController; }
    AIServiceManager     *aiServiceManager()     const { return m_aiServiceManager; }
    ShapeController      *shapeController()      const { return m_shapeController; }
    WorkspaceModel       *workspaceModel()       const { return m_model; }

    void setDialogParent(QWidget *parent) { m_dialogParent = parent; }
    void setMainWindow(MainWindow *window);
    void setShapeVisualization(ShapeVisualization *visualization);
    void connectToView(MainWindow *view);

    // --- Actions déléguées (navigation) ---
    void openWifiSettings(QWidget *parent);
    void openInventory(QWidget *mainWindow, QWidget *inventoryWindow);
    void openCustomEditor(QWidget *mainWindow, Language language);
    void openFolder(QWidget *mainWindow, Language language);
    void openTestGpio(QWidget *mainWindow);
    void openBluetoothReceiver(QWidget *mainWindow);
    void openWifiTransfer(QWidget *mainWindow);

    void handleSaveLayoutRequest(QWidget *mainWindow,
                                 ShapeVisualization *visualization,
                                 ShapeModel::Type selectedType);

    bool openAiGenerationPrompt(QWidget *parent, AiGenerationRequest &request);

    void openImageInCustom(const QString &filePath,
                           bool internalContours,
                           bool colorEdges);

    // --- Logique de découpe ---
public slots:
    void startCutting();
    void stopCutting();
    void pauseCutting();

    // --- Logique IA ---
    void requestAiGeneration(const QString& prompt,
                             const QString& model,
                             const QString& quality,
                             const QString& size,
                             bool colorPrompt);

    // --- Logique de langue ---
    void changeLanguage(Language lang);

    // --- Réactions aux signaux de la View ---
    void onDimensionsChanged(int largeur, int longueur);
    void onShapeCountChanged(int count);
    void onSpacingChanged(int spacing);

    void onCircleRequested();
    void onRectangleRequested();
    void onTriangleRequested();
    void onStarRequested();
    void onHeartRequested();

    void onOptimize1Requested(bool checked);
    void onOptimize2Requested(bool checked);

    void onMoveUpRequested();
    void onMoveDownRequested();
    void onMoveLeftRequested();
    void onMoveRightRequested();
    void onRotateLeftRequested();
    void onRotateRightRequested();
    void onAddShapeRequested();
    void onDeleteShapeRequested();

    void onInventoryRequested();
    void onCustomEditorRequested();
    void onFolderRequested();
    void onTestGpioRequested();
    void onBluetoothReceiverRequested();
    void onWifiTransferRequested();

    void onSaveLayoutRequested();
    void onGenerateAiRequested();

    // Réactions aux événements de l'Inventory
    void onCustomShapeSelected(const QList<QPolygonF> &polygons, const QString &name);
    void onShapeSelectedFromInventory(ShapeModel::Type type);

signals:
    // Signaux de découpe
    void cutProgressUpdated(int percent, const QString &timeTxt);
    void cutFinished(bool success);
    void cutControlsEnabled(bool enabled);

    // Signaux IA
    void aiGenerationStatus(const QString &msg);
    void aiImageReady(const QString &path);

    // Signaux de langue
    void languageApplied(Language lang, bool ok);

    // Signaux MainWindowCoordinator existants
    void generationStatusChanged(const QString &status);
    void imageReadyForImport(const QString &path, bool internalContours, bool colorEdges);
    void requestShowFullScreen();

private:
    bool ensureServicesInitialized();
    bool loadLanguage(Language lang);

    NavigationController *m_navigationController;
    AIServiceManager     *m_aiServiceManager;
    ShapeController      *m_shapeController;
    WorkspaceModel       *m_model;

    QWidget  *m_dialogParent = nullptr;
    MainWindow *m_view       = nullptr;
    ShapeVisualization *m_shapeVisualization = nullptr;

    TrajetMotor *m_trajetMotor = nullptr;
    OpenAIService *m_aiService = nullptr;
    QTranslator m_translator;
    Language m_currentLanguage = Language::French;
    bool m_pauseRequested = false;
};
