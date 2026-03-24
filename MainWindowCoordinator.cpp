#include "MainWindowCoordinator.h"

#include "NavigationController.h"
#include "ShapeController.h"
#include "ShapeVisualization.h"

MainWindowCoordinator::MainWindowCoordinator(NavigationController *navigationController,
                                             AIServiceManager *aiServiceManager,
                                             ShapeController *shapeController,
                                             QObject *parent)
    : QObject(parent)
    , m_navigationController(navigationController)
    , m_aiServiceManager(aiServiceManager)
    , m_shapeController(shapeController)
{
    QObject::connect(m_aiServiceManager, &AIServiceManager::generationStatusChanged,
                     this, &MainWindowCoordinator::generationStatusChanged);
    QObject::connect(m_aiServiceManager, &AIServiceManager::imageReadyForImport,
                     this, &MainWindowCoordinator::imageReadyForImport);
}

void MainWindowCoordinator::setDialogParent(QWidget *parent)
{
    m_aiServiceManager->setDialogParent(parent);
}

void MainWindowCoordinator::openWifiSettings(QWidget *parent)
{
    m_navigationController->openWifiSettings(parent);
}

void MainWindowCoordinator::openInventory(QWidget *mainWindow, QWidget *inventoryWindow)
{
    m_navigationController->showInventory(mainWindow, inventoryWindow);
}

void MainWindowCoordinator::openCustomEditor(QWidget *mainWindow, Language language)
{
    m_navigationController->openCustomEditor(mainWindow, language);
}

void MainWindowCoordinator::openFolder(QWidget *mainWindow, Language language)
{
    m_navigationController->openFolder(mainWindow, language);
}

void MainWindowCoordinator::openTestGpio(QWidget *mainWindow)
{
    m_navigationController->openTestGpio(mainWindow);
}

void MainWindowCoordinator::openBluetoothReceiver(QWidget *mainWindow)
{
    m_navigationController->openBluetoothReceiver(mainWindow);
}

void MainWindowCoordinator::openWifiTransfer(QWidget *mainWindow)
{
    m_navigationController->openWifiTransfer(mainWindow);
}

void MainWindowCoordinator::handleSaveLayoutRequest(QWidget *mainWindow,
                                                    ShapeVisualization *visualization,
                                                    ShapeModel::Type selectedType)
{
    m_navigationController->handleSaveLayoutRequest(mainWindow, visualization, selectedType);
}

bool MainWindowCoordinator::openAiGenerationPrompt(QWidget *parent, AiGenerationRequest &request)
{
    return m_aiServiceManager->openGenerationPrompt(parent, request);
}
