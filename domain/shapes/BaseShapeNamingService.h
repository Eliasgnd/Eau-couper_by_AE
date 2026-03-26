#pragma once

#include <QString>

#include "ShapeModel.h"
#include "Language.h"

namespace BaseShapeNamingService {

QString baseShapeName(ShapeModel::Type type, Language lang);

} // namespace BaseShapeNamingService
