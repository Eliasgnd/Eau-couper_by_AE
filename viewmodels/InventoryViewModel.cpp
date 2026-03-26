#include "InventoryViewModel.h"
#include "InventoryController.h"

InventoryViewModel::InventoryViewModel(InventoryController &controller, QObject *parent)
    : QObject(parent)
    , m_controller(controller)
{
    rebuildState();
}

void InventoryViewModel::setSearchText(const QString &text)
{
    if (m_searchText == text)
        return;
    m_searchText = text;
    emit searchTextChanged(m_searchText);
    rebuildState();
}

void InventoryViewModel::setSortMode(InventorySortMode mode)
{
    if (m_sortMode == mode)
        return;
    m_sortMode = mode;
    emit sortModeChanged(m_sortMode);
    rebuildState();
}

void InventoryViewModel::setFilterMode(InventoryFilterMode mode)
{
    if (m_filterMode == mode)
        return;
    m_filterMode = mode;
    emit filterModeChanged(m_filterMode);
    rebuildState();
}

void InventoryViewModel::addCustomShape(const QList<QPolygonF> &polygons, const QString &name)
{
    m_controller.addCustomShape(polygons, name);
    rebuildState();
}

void InventoryViewModel::selectFolder(const QString &folderName)
{
    m_controller.onFolderSelected(folderName);
    rebuildState();
}

void InventoryViewModel::selectBaseShape(ShapeModel::Type type)
{
    m_controller.onBaseShapeSelected(type);
}

bool InventoryViewModel::selectCustomShape(const QString &shapeName,
                                           QList<QPolygonF> &polygonsOut,
                                           QString &resolvedNameOut)
{
    return m_controller.onCustomShapeSelected(shapeName, polygonsOut, resolvedNameOut);
}

bool InventoryViewModel::createFolder(const QString &name, const QString &parentFolder)
{
    bool ok = m_controller.createFolder(name, parentFolder);
    if (ok)
        rebuildState();
    return ok;
}

bool InventoryViewModel::renameFolder(const QString &oldName, const QString &newName)
{
    bool ok = m_controller.renameFolder(oldName, newName);
    if (ok)
        rebuildState();
    return ok;
}

bool InventoryViewModel::deleteFolder(const QString &folderName)
{
    bool ok = m_controller.deleteFolder(folderName);
    if (ok)
        rebuildState();
    return ok;
}

bool InventoryViewModel::renameCustomShape(int index, const QString &newName)
{
    bool ok = m_controller.renameCustomShape(index, newName);
    if (ok)
        rebuildState();
    return ok;
}

bool InventoryViewModel::deleteCustomShape(int index)
{
    bool ok = m_controller.deleteCustomShape(index);
    if (ok)
        rebuildState();
    return ok;
}

void InventoryViewModel::navigateBack()
{
    rebuildState();
}

void InventoryViewModel::refresh()
{
    rebuildState();
}

void InventoryViewModel::rebuildState()
{
    if (m_state.inFolderView && !m_state.currentFolder.isEmpty())
        m_state = m_controller.buildFolderState(m_state.currentFolder, m_searchText, m_sortMode, m_filterMode);
    else
        m_state = m_controller.buildRootState(m_searchText, m_sortMode, m_filterMode);

    emit stateChanged(m_state);
}
