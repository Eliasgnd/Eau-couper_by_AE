#ifndef IMPORTEDIMAGEGEOMETRYHELPER_H
#define IMPORTEDIMAGEGEOMETRYHELPER_H

#include <QPainterPath>
#include <QSize>

class ImportedImageGeometryHelper
{
public:
    static QPainterPath fitCentered(const QPainterPath &path,
                                    const QSize &targetSize,
                                    double occupancy = 0.8);
};

#endif // IMPORTEDIMAGEGEOMETRYHELPER_H
