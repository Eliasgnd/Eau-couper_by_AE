#include "InventoryQueryService.h"
#include "BaseShapeNamingService.h"

namespace InventoryQueryService {

bool shapeNameExists(const QList<CustomShapeData> &customShapes, const QString &name)
{
    for (const auto &shapeData : customShapes) {
        if (shapeData.name.compare(name, Qt::CaseSensitive) == 0)
            return true;
    }
    return false;
}

QStringList getAllShapeNames(const QList<CustomShapeData> &customShapes, Language language)
{
    QStringList names;

    if (language == Language::French) {
        names << "Cercle" << "Rectangle" << "Triangle" << "Étoile" << "Cœur";
    } else {
        names << "Circle" << "Rectangle" << "Triangle" << "Star" << "Heart";
    }

    for (const CustomShapeData &data : customShapes)
        names << data.name;

    return names;
}

bool folderIsEmpty(const QString &folderName,
                   const QList<InventoryFolder> &folders,
                   const QList<CustomShapeData> &customShapes,
                   const QMap<ShapeModel::Type, QString> &baseShapeFolders)
{
    for (const InventoryFolder &folder : folders)
        if (folder.parentFolder == folderName)
            return false;

    for (const CustomShapeData &shape : customShapes)
        if (shape.folder == folderName)
            return false;

    for (auto it = baseShapeFolders.constBegin(); it != baseShapeFolders.constEnd(); ++it)
        if (it.value() == folderName)
            return false;

    return true;
}

bool folderContainsMatchingShape(const QString &folderName,
                                 const QString &text,
                                 const QList<InventoryFolder> &folders,
                                 const QList<CustomShapeData> &customShapes,
                                 const QMap<ShapeModel::Type, QString> &baseShapeFolders,
                                 Language language)
{
    const QString search = text.trimmed().toLower();
    if (search.isEmpty())
        return false;

    for (const InventoryFolder &folder : folders) {
        if (folder.parentFolder == folderName) {
            if (folder.name.toLower().contains(search))
                return true;
            if (folderContainsMatchingShape(folder.name,
                                            search,
                                            folders,
                                            customShapes,
                                            baseShapeFolders,
                                            language)) {
                return true;
            }
        }
    }

    for (const CustomShapeData &shape : customShapes)
        if (shape.folder == folderName && shape.name.toLower().contains(search))
            return true;

    for (auto it = baseShapeFolders.constBegin(); it != baseShapeFolders.constEnd(); ++it) {
        if (it.value() == folderName) {
            QString name = BaseShapeNamingService::baseShapeName(it.key(), language);
            if (name.toLower().contains(search))
                return true;
        }
    }

    return false;
}


QString parentFolderOf(const QList<InventoryFolder> &folders, const QString &folderName)
{
    for (const InventoryFolder &folder : folders) {
        if (folder.name == folderName)
            return folder.parentFolder;
    }
    return {};
}

} // namespace InventoryQueryService
