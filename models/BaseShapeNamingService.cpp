#include "BaseShapeNamingService.h"

QString BaseShapeNamingService::baseShapeName(ShapeModel::Type type, Language lang)
{
    switch (type) {
    case ShapeModel::Type::Circle:
        return lang == Language::French ? QStringLiteral("Cercle")    : QStringLiteral("Circle");
    case ShapeModel::Type::Rectangle:
        return lang == Language::French ? QStringLiteral("Rectangle") : QStringLiteral("Rectangle");
    case ShapeModel::Type::Triangle:
        return lang == Language::French ? QStringLiteral("Triangle")  : QStringLiteral("Triangle");
    case ShapeModel::Type::Star:
        return lang == Language::French ? QStringLiteral("Étoile")    : QStringLiteral("Star");
    case ShapeModel::Type::Heart:
        return lang == Language::French ? QStringLiteral("Cœur")      : QStringLiteral("Heart");
    }
    return {};
}
