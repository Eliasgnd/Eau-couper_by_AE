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
    m_state.inFolderView = true;
    m_state.currentFolder = folderName;
    rebuildState();
}

void InventoryViewModel::selectBaseShape(ShapeModel::Type type)
{
    m_controller.onBaseShapeSelected(type);
    emit baseShapeSelected(type);
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

void InventoryViewModel::navigateToFolder(const QString &folderName)
{
    m_state.inFolderView = true;
    m_state.currentFolder = folderName;
    rebuildState();
}

bool InventoryViewModel::moveCustomShapeToFolder(int index, const QString &folderName)
{
    bool ok = m_controller.moveCustomShapeToFolder(index, folderName);
    if (ok) rebuildState();
    return ok;
}

bool InventoryViewModel::removeCustomShapeFromFolderToParent(int index)
{
    bool ok = m_controller.removeCustomShapeFromFolderToParent(index);
    if (ok) rebuildState();
    return ok;
}

bool InventoryViewModel::moveBaseShapeToFolder(ShapeModel::Type type, const QString &folderName)
{
    bool ok = m_controller.moveBaseShapeToFolder(type, folderName);
    if (ok) rebuildState();
    return ok;
}

bool InventoryViewModel::removeBaseShapeFromFolderToParent(ShapeModel::Type type)
{
    bool ok = m_controller.removeBaseShapeFromFolderToParent(type);
    if (ok) rebuildState();
    return ok;
}

bool InventoryViewModel::addLayoutToCustomShape(const QString &shapeName, const LayoutData &layout)
{
    return m_controller.addLayoutToCustomShape(shapeName, layout);
}

bool InventoryViewModel::renameLayoutForCustomShape(const QString &shapeName, int index, const QString &newName)
{
    return m_controller.renameLayoutForCustomShape(shapeName, index, newName);
}

bool InventoryViewModel::deleteLayoutForCustomShape(const QString &shapeName, int index)
{
    return m_controller.deleteLayoutForCustomShape(shapeName, index);
}

bool InventoryViewModel::addLayoutToBaseShape(ShapeModel::Type type, const LayoutData &layout)
{
    return m_controller.addLayoutToBaseShape(type, layout);
}

bool InventoryViewModel::renameLayoutForBaseShape(ShapeModel::Type type, int index, const QString &newName)
{
    return m_controller.renameLayoutForBaseShape(type, index, newName);
}

bool InventoryViewModel::deleteLayoutForBaseShape(ShapeModel::Type type, int index)
{
    return m_controller.deleteLayoutForBaseShape(type, index);
}

bool InventoryViewModel::incrementLayoutUsageForCustomShape(const QString &shapeName, int index)
{
    return m_controller.incrementLayoutUsageForCustomShape(shapeName, index);
}

bool InventoryViewModel::incrementLayoutUsageForBaseShape(ShapeModel::Type type, int index)
{
    return m_controller.incrementLayoutUsageForBaseShape(type, index);
}

bool InventoryViewModel::recordBaseShapeCut(ShapeModel::Type type)
{
    return m_controller.recordBaseShapeCut(type);
}

bool InventoryViewModel::recordCustomShapeCut(const QString &shapeName)
{
    return m_controller.recordCustomShapeCut(shapeName);
}

bool InventoryViewModel::findCustomShapeByName(const QString &shapeName,
                                               QList<QPolygonF> &polygonsOut,
                                               QString &resolvedNameOut) const
{
    return m_controller.findCustomShapeByName(shapeName, polygonsOut, resolvedNameOut);
}

QList<QuickShapeEntry> InventoryViewModel::getQuickAccessShapes(int maxCount) const
{
    return m_controller.getQuickAccessShapes(maxCount);
}

bool InventoryViewModel::shapeNameExists(const QString &name) const
{
    return m_controller.shapeNameExists(name);
}

QStringList InventoryViewModel::getAllShapeNames(Language lang) const
{
    return m_controller.getAllShapeNames(lang);
}

QList<LayoutData> InventoryViewModel::getLayoutsForShape(const QString &shapeName) const
{
    return m_controller.getLayoutsForShape(shapeName);
}

QList<LayoutData> InventoryViewModel::getLayoutsForBaseShape(ShapeModel::Type type) const
{
    return m_controller.getLayoutsForBaseShape(type);
}

bool InventoryViewModel::folderIsEmpty(const QString &folderName) const
{
    return m_controller.folderIsEmpty(folderName);
}

QString InventoryViewModel::parentFolderOf(const QString &folderName) const
{
    return m_controller.parentFolderOf(folderName);
}

Language InventoryViewModel::language() const
{
    return m_controller.language();
}

void InventoryViewModel::setLanguage(Language lang)
{
    m_controller.setLanguage(lang);
}

const QList<CustomShapeData> &InventoryViewModel::customShapes() const
{
    return m_controller.customShapes();
}

const QList<InventoryFolder> &InventoryViewModel::folders() const
{
    return m_controller.folders();
}

const QMap<ShapeModel::Type, QString> &InventoryViewModel::baseShapeFolders() const
{
    return m_controller.baseShapeFolders();
}

void InventoryViewModel::navigateBack()
{
    if (m_state.inFolderView) {
        const QString parent = m_controller.parentFolderOf(m_state.currentFolder);
        if (parent.isEmpty()) {
            m_state.inFolderView = false;
            m_state.currentFolder.clear();
        } else {
            m_state.currentFolder = parent;
        }
    }
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

QPainterPath InventoryViewModel::baseShapePreviewPath(ShapeModel::Type type, int size) const
{
    const QList<QPolygonF> polys = ShapeModel::shapePolygons(type, size, size);
    QPainterPath path;
    for (const QPolygonF &poly : polys)
        path.addPolygon(poly);
    return path;
}
