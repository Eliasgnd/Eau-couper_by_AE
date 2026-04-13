#include "InventoryController.h"

#include "InventoryModel.h"
#include "InventoryMutationService.h"
#include "InventoryQueryService.h"
#include "InventorySortFilterService.h"
#include "BaseShapeNamingService.h"

#include <QDateTime>

namespace {
InventoryViewItem makeFolderItem(const InventoryFolder &folder, int index)
{
    InventoryViewItem item;
    item.kind = InventoryViewItem::Kind::Folder;
    item.id = QStringLiteral("folder:%1").arg(index);
    item.displayName = folder.name;
    item.usageCount = folder.usageCount;
    item.lastUsed = folder.lastUsed;
    item.isInFolder = !folder.parentFolder.isEmpty();
    item.folderName = folder.parentFolder;
    item.payload = folder.name;
    return item;
}

InventoryViewItem makeBaseShapeItem(ShapeModel::Type type,
                                    const QString &name,
                                    int usageCount,
                                    const QDateTime &lastUsed,
                                    const QString &folderName)
{
    InventoryViewItem item;
    item.kind = InventoryViewItem::Kind::BaseShape;
    item.id = QStringLiteral("base:%1").arg(static_cast<int>(type));
    item.displayName = name;
    item.usageCount = usageCount;
    item.lastUsed = lastUsed;
    item.isInFolder = !folderName.isEmpty();
    item.folderName = folderName;
    item.payload = static_cast<int>(type);
    return item;
}

InventoryViewItem makeCustomShapeItem(const CustomShapeData &shape, int index)
{
    InventoryViewItem item;
    item.kind = InventoryViewItem::Kind::CustomShape;
    item.id = QStringLiteral("custom:%1").arg(index);
    item.displayName = shape.name;
    item.usageCount = shape.usageCount;
    item.lastUsed = shape.lastUsed;
    item.isInFolder = !shape.folder.isEmpty();
    item.folderName = shape.folder;
    item.payload = index;
    return item;
}
}

InventoryController::InventoryController(InventoryModel &model)
    : m_model(model)
{
}

void InventoryController::initialize()
{
    m_model.load();
    if (m_model.baseShapeOrder().isEmpty()) {
        m_model.baseShapeOrder() = {ShapeModel::Type::Circle,
                                    ShapeModel::Type::Rectangle,
                                    ShapeModel::Type::Triangle,
                                    ShapeModel::Type::Star,
                                    ShapeModel::Type::Heart};
        m_model.save();
    }
}

void InventoryController::addCustomShape(const QList<QPolygonF> &polygons, const QString &name)
{
    CustomShapeData data;
    data.polygons = polygons;
    data.name = name;
    data.folder = "";
    data.layouts.clear();
    data.usageCount = 0;
    data.lastUsed = QDateTime();
    m_model.customShapes().append(data);
    m_model.save();
}

bool InventoryController::createFolder(const QString &name, const QString &parentFolder)
{
    if (!InventoryMutationService::createFolder(m_model.folders(), name, parentFolder)) {
        return false;
    }

    m_model.save();
    return true;
}

bool InventoryController::renameFolder(const QString &oldName, const QString &newName)
{
    if (!InventoryMutationService::renameFolder(m_model.folders(), oldName, newName))
        return false;
    m_model.save();
    return true;
}

bool InventoryController::deleteFolder(const QString &folderName)
{
    if (!InventoryMutationService::deleteFolder(m_model.folders(), m_model.customShapes(), folderName))
        return false;
    m_model.save();
    return true;
}

void InventoryController::onFolderSelected(const QString &folderName)
{
    if (InventoryMutationService::incrementFolderUsage(m_model.folders(),
                                                       folderName,
                                                       QDateTime::currentDateTime())) {
        m_model.save();
    }
}

void InventoryController::onBaseShapeSelected(ShapeModel::Type type)
{
    InventoryMutationService::incrementBaseShapeUsage(m_model.baseUsageCount(),
                                                      m_model.baseLastUsed(),
                                                      type,
                                                      QDateTime::currentDateTime());
    m_model.save();
}

bool InventoryController::onCustomShapeSelected(const QString &shapeName,
                                                QList<QPolygonF> &polygonsOut,
                                                QString &resolvedNameOut)
{
    const int shapeIndex = InventoryMutationService::incrementCustomShapeUsageByName(m_model.customShapes(),
                                                                                      shapeName,
                                                                                      QDateTime::currentDateTime());
    if (shapeIndex < 0) {
        return false;
    }

    m_model.save();
    polygonsOut = m_model.customShapes()[shapeIndex].polygons;
    resolvedNameOut = m_model.customShapes()[shapeIndex].name;
    return true;
}

bool InventoryController::renameCustomShape(int index, const QString &newName)
{
    if (index < 0 || index >= m_model.customShapes().size())
        return false;
    if (newName.trimmed().isEmpty())
        return false;
    if (InventoryQueryService::shapeNameExists(m_model.customShapes(), newName.trimmed()))
        return false;
    m_model.customShapes()[index].name = newName.trimmed();
    m_model.save();
    return true;
}

bool InventoryController::deleteCustomShape(int index)
{
    if (index < 0 || index >= m_model.customShapes().size())
        return false;
    m_model.customShapes().removeAt(index);
    m_model.save();
    return true;
}

bool InventoryController::moveCustomShapeToFolder(int index, const QString &folderName)
{
    if (index < 0 || index >= m_model.customShapes().size())
        return false;
    m_model.customShapes()[index].folder = folderName;
    m_model.save();
    return true;
}

bool InventoryController::removeCustomShapeFromFolderToParent(int index)
{
    if (index < 0 || index >= m_model.customShapes().size())
        return false;
    const QString oldFolder = m_model.customShapes()[index].folder;
    QString parentFolder;
    for (const InventoryFolder &f : m_model.folders()) {
        if (f.name == oldFolder) {
            parentFolder = f.parentFolder;
            break;
        }
    }
    m_model.customShapes()[index].folder = parentFolder;
    m_model.save();
    return true;
}

bool InventoryController::moveBaseShapeToFolder(ShapeModel::Type type, const QString &folderName)
{
    m_model.baseShapeFolders()[type] = folderName;
    m_model.save();
    return true;
}

bool InventoryController::removeBaseShapeFromFolderToParent(ShapeModel::Type type)
{
    const QString oldFolder = m_model.baseShapeFolders().value(type);
    QString parentFolder;
    for (const InventoryFolder &f : m_model.folders()) {
        if (f.name == oldFolder) {
            parentFolder = f.parentFolder;
            break;
        }
    }
    m_model.baseShapeFolders()[type] = parentFolder;
    m_model.save();
    return true;
}

bool InventoryController::addLayoutToCustomShape(const QString &shapeName, const LayoutData &layout)
{
    if (!InventoryMutationService::addLayoutToShape(m_model.customShapes(), shapeName, layout))
        return false;
    m_model.save();
    return true;
}

bool InventoryController::renameLayoutForCustomShape(const QString &shapeName, int index, const QString &newName)
{
    if (!InventoryMutationService::renameLayout(m_model.customShapes(), shapeName, index, newName))
        return false;
    m_model.save();
    return true;
}

bool InventoryController::deleteLayoutForCustomShape(const QString &shapeName, int index)
{
    if (!InventoryMutationService::deleteLayout(m_model.customShapes(), shapeName, index))
        return false;
    m_model.save();
    return true;
}

bool InventoryController::addLayoutToBaseShape(ShapeModel::Type type, const LayoutData &layout)
{
    if (!InventoryMutationService::addLayoutToBaseShape(m_model.baseShapeLayouts(), type, layout))
        return false;
    m_model.save();
    return true;
}

bool InventoryController::renameLayoutForBaseShape(ShapeModel::Type type, int index, const QString &newName)
{
    if (!InventoryMutationService::renameBaseLayout(m_model.baseShapeLayouts(), type, index, newName))
        return false;
    m_model.save();
    return true;
}

bool InventoryController::deleteLayoutForBaseShape(ShapeModel::Type type, int index)
{
    if (!InventoryMutationService::deleteBaseLayout(m_model.baseShapeLayouts(), type, index))
        return false;
    m_model.save();
    return true;
}

bool InventoryController::incrementLayoutUsageForCustomShape(const QString &shapeName, int index)
{
    if (!InventoryMutationService::incrementLayoutUsage(m_model.customShapes(),
                                                        shapeName,
                                                        index,
                                                        QDateTime::currentDateTime())) {
        return false;
    }
    m_model.save();
    return true;
}

bool InventoryController::incrementLayoutUsageForBaseShape(ShapeModel::Type type, int index)
{
    if (!InventoryMutationService::incrementBaseLayoutUsage(m_model.baseShapeLayouts(),
                                                            type,
                                                            index,
                                                            QDateTime::currentDateTime())) {
        return false;
    }
    m_model.save();
    return true;
}

InventoryViewState InventoryController::buildRootState(const QString &filterText,
                                                       InventorySortMode sortMode,
                                                       InventoryFilterMode filterMode) const
{
    InventoryViewState state;
    state.inFolderView = false;
    state.searchText = filterText;
    state.sortMode = sortMode;
    state.filterMode = filterMode;

    const QString f = filterText.trimmed().toLower();
    const bool includeFolders = filterMode != InventoryFilterMode::ShapesOnly;
    const bool includeShapes = filterMode != InventoryFilterMode::FoldersOnly;

    if (includeFolders) {
        for (int i = 0; i < m_model.folders().size(); ++i) {
            const InventoryFolder &folder = m_model.folders().at(i);
            if (!folder.parentFolder.isEmpty())
                continue;
            if (!f.isEmpty() && !folder.name.toLower().contains(f) &&
                !InventoryQueryService::folderContainsMatchingShape(folder.name,
                                                                    f,
                                                                    m_model.folders(),
                                                                    m_model.customShapes(),
                                                                    m_model.baseShapeFolders(),
                                                                    m_model.languageRef())) {
                continue;
            }
            state.items.append(makeFolderItem(folder, i));
        }
    }

    if (includeShapes) {
        for (ShapeModel::Type type : m_model.baseShapeOrder()) {
            const QString name = BaseShapeNamingService::baseShapeName(type, m_model.languageRef());
            if (!f.isEmpty() && !name.toLower().contains(f))
                continue;

            const QString folder = m_model.baseShapeFolders().value(type);
            if (!folder.isEmpty())
                continue;

            state.items.append(makeBaseShapeItem(type,
                                                 name,
                                                 m_model.baseUsageCount().value(type),
                                                 m_model.baseLastUsed().value(type),
                                                 folder));
        }

        for (int i = 0; i < m_model.customShapes().size(); ++i) {
            const CustomShapeData &shape = m_model.customShapes().at(i);
            if (!shape.folder.isEmpty())
                continue;
            if (!f.isEmpty() && !shape.name.toLower().contains(f))
                continue;
            state.items.append(makeCustomShapeItem(shape, i));
        }
    }

    InventorySortFilterService::sortItems(state.items, sortMode);
    return state;
}

InventoryViewState InventoryController::buildFolderState(const QString &folderName,
                                                         const QString &filterText,
                                                         InventorySortMode sortMode,
                                                         InventoryFilterMode filterMode) const
{
    InventoryViewState state;
    state.inFolderView = true;
    state.currentFolder = folderName;
    state.searchText = filterText;
    state.sortMode = sortMode;
    state.filterMode = filterMode;

    const QString search = filterText.trimmed().toLower();
    const bool includeFolders = filterMode != InventoryFilterMode::ShapesOnly;
    const bool includeShapes = filterMode != InventoryFilterMode::FoldersOnly;

    if (includeFolders) {
        for (int i = 0; i < m_model.folders().size(); ++i) {
            const InventoryFolder &folder = m_model.folders().at(i);
            if (folder.parentFolder != folderName)
                continue;
            if (!search.isEmpty() && !folder.name.toLower().contains(search) &&
                !InventoryQueryService::folderContainsMatchingShape(folder.name,
                                                                    search,
                                                                    m_model.folders(),
                                                                    m_model.customShapes(),
                                                                    m_model.baseShapeFolders(),
                                                                    m_model.languageRef())) {
                continue;
            }
            state.items.append(makeFolderItem(folder, i));
        }
    }

    if (includeShapes) {
        for (ShapeModel::Type type : m_model.baseShapeOrder()) {
            if (m_model.baseShapeFolders().value(type) != folderName)
                continue;
            const QString name = BaseShapeNamingService::baseShapeName(type, m_model.languageRef());
            if (!search.isEmpty() && !name.toLower().contains(search))
                continue;
            state.items.append(makeBaseShapeItem(type,
                                                 name,
                                                 m_model.baseUsageCount().value(type),
                                                 m_model.baseLastUsed().value(type),
                                                 folderName));
        }

        for (int i = 0; i < m_model.customShapes().size(); ++i) {
            const CustomShapeData &shape = m_model.customShapes().at(i);
            if (shape.folder != folderName)
                continue;
            if (!search.isEmpty() && !shape.name.toLower().contains(search))
                continue;
            state.items.append(makeCustomShapeItem(shape, i));
        }
    }

    InventorySortFilterService::sortItems(state.items, sortMode);
    return state;
}

bool InventoryController::shapeNameExists(const QString &name) const
{
    return InventoryQueryService::shapeNameExists(m_model.customShapes(), name);
}

QStringList InventoryController::getAllShapeNames(Language lang) const
{
    return InventoryQueryService::getAllShapeNames(m_model.customShapes(), lang);
}

QList<LayoutData> InventoryController::getLayoutsForShape(const QString &shapeName) const
{
    return InventoryMutationService::getLayoutsForShape(m_model.customShapes(), shapeName);
}

QList<LayoutData> InventoryController::getLayoutsForBaseShape(ShapeModel::Type type) const
{
    return InventoryMutationService::getLayoutsForBaseShape(m_model.baseShapeLayouts(), type);
}

bool InventoryController::folderIsEmpty(const QString &folderName) const
{
    return InventoryQueryService::folderIsEmpty(folderName,
                                                m_model.folders(),
                                                m_model.customShapes(),
                                                m_model.baseShapeFolders());
}

QString InventoryController::parentFolderOf(const QString &folderName) const
{
    return InventoryQueryService::parentFolderOf(m_model.folders(), folderName);
}

const QList<CustomShapeData> &InventoryController::customShapes() const
{
    return m_model.customShapes();
}

const QList<InventoryFolder> &InventoryController::folders() const
{
    return m_model.folders();
}

const QMap<ShapeModel::Type, QString> &InventoryController::baseShapeFolders() const
{
    return m_model.baseShapeFolders();
}

Language InventoryController::language() const
{
    return m_model.languageRef();
}

void InventoryController::setLanguage(Language lang)
{
    m_model.languageRef() = lang;
}
