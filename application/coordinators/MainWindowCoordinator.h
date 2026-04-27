#pragma once

#include <QObject>
#include <QList>
#include <QPolygonF>
#include <QString>
#include <QTranslator>

#include "Language.h"
#include "AIDialogCoordinator.h"
#include "ShapeModel.h"
#include "DialogManager.h"
#include "ShapeCoordinator.h"
#include "WorkspaceViewModel.h"
#include "MainWindowViewModel.h"
#include "MachineViewModel.h"
#include "CuttingService.h"

class MainWindow; // Forward declaration
class ShapeVisualization;
class OpenAIService;
class Inventory;

class MainWindowCoordinator : public QObject {
    Q_OBJECT

public:
    explicit MainWindowCoordinator(DialogManager *navigationController,
                                   AIDialogCoordinator *aiServiceManager,
                                   ShapeCoordinator *shapeController,
                                   WorkspaceViewModel *model,
                                   QObject *parent = nullptr);

    DialogManager *navigationController() const { return m_navigationController; }
    AIDialogCoordinator     *aiServiceManager()     const { return m_aiServiceManager; }
    ShapeCoordinator      *shapeController()      const { return m_shapeController; }
    WorkspaceViewModel       *workspaceModel()       const { return m_model; }
    MachineViewModel         *machineViewModel()     const { return m_machineViewModel; }

    void setDialogParent(QWidget *parent) { m_dialogParent = parent; }
    void setMainWindow(MainWindow *window);
    void setShapeVisualization(ShapeVisualization *visualization);
    void setInventory(Inventory *inventory);
    void connectToView(MainWindow *view);
    void setViewModel(MainWindowViewModel *viewModel);
    MainWindowViewModel *viewModel() const { return m_viewModel; }

    // --- Actions déléguées (navigation) ---
    void openWifiSettings(QWidget *parent);
    void openInventory(QWidget *mainWindow, QWidget *inventoryWindow);
    void openCustomEditor(QWidget *mainWindow, Language language);
    void openFolder(QWidget *mainWindow, Language language);
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
    void setCuttingSpeed(int speed_mm_s);  // Transmis depuis SpinBox_vitesse → CuttingService

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
    void onBaseShapeQuickRequested(int type);
    void onCustomShapeQuickRequested(const QString &name);

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
    void onWifiTransferRequested();

    void onSaveLayoutRequested();
    void onGenerateAiRequested();
    void onStmTestRequested();

    void onHomeRequested();
    void onPositionResetRequested();
    void onRearmRequested();

    // Réactions aux événements de l'Inventory
    void onCustomShapeSelected(const QList<QPolygonF> &polygons, const QString &name);
    void onShapeSelectedFromInventory(ShapeModel::Type type);

signals:
    // Signaux IA (toujours émis directement — utilisés par d'autres composants)
    void aiGenerationStatus(const QString &msg);
    void aiImageReady(const QString &path);

    // Signaux de découpe, langue et état UI → désormais via MainWindowViewModel
    // (conservés pour compatibilité interne si besoin)
    void generationStatusChanged(const QString &status);
    void imageReadyForImport(const QString &path, bool internalContours, bool colorEdges);
    void requestShowFullScreen();

private:
    enum class ActiveShapeOrigin {
        Unknown,
        Base,
        Custom
    };

    bool ensureServicesInitialized();
    bool loadLanguage(Language lang);
    void setActiveBaseShape(ShapeModel::Type type);
    void setActiveCustomShape(const QString &name);
    void clearActiveShapeSelection();
    void recordCurrentShapeCut();
    void applyCustomShapeSelection(const QList<QPolygonF> &polygons, const QString &name);

    DialogManager *m_navigationController;
    AIDialogCoordinator     *m_aiServiceManager;
    ShapeCoordinator      *m_shapeController;
    WorkspaceViewModel       *m_model;

    QWidget  *m_dialogParent = nullptr;
    MainWindow *m_view       = nullptr;
    ShapeVisualization *m_shapeVisualization = nullptr;
    MainWindowViewModel *m_viewModel = nullptr;
    Inventory *m_inventory = nullptr;

    CuttingService   *m_cuttingService    = nullptr;
    MachineViewModel *m_machineViewModel  = nullptr;
    OpenAIService    *m_aiService         = nullptr;
    QTranslator m_translator;
    Language m_currentLanguage = Language::French;

    bool m_isCutting = false;
    ActiveShapeOrigin m_activeShapeOrigin = ActiveShapeOrigin::Base;
    ShapeModel::Type m_activeBaseShapeType = ShapeModel::Type::Circle;
    QString m_activeCustomShapeName;
};
