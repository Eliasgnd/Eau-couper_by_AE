#pragma once

#include <QObject>

#include "ui_mainwindow.h"
#include "Language.h"
#include "AIServiceManager.h"
#include "ShapeModel.h"

class NavigationController;
class ShapeController;
class QWidget;
class ShapeVisualization;
struct LayoutData;

enum class Language;

class MainWindowCoordinator : public QObject
{
    Q_OBJECT
public:
    MainWindowCoordinator(NavigationController *navigationController,
                          AIServiceManager *aiServiceManager,
                          ShapeController *shapeController,
                          QObject *parent = nullptr);

    NavigationController *navigationController() const { return m_navigationController; }
    AIServiceManager *aiServiceManager() const { return m_aiServiceManager; }
    ShapeController *shapeController() const { return m_shapeController; }

    void setDialogParent(QWidget *parent);

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

    void bindTo(Ui::MainWindow *ui,
                QWidget *mainWindow,
                const std::function<Language()> &languageProvider);
signals:
    void generationStatusChanged(const QString &status);
    void imageReadyForImport(const QString &path, bool internalContours, bool colorEdges);

private:
    NavigationController *m_navigationController = nullptr;
    AIServiceManager *m_aiServiceManager = nullptr;
    ShapeController *m_shapeController = nullptr;
};
