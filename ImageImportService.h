#ifndef IMAGEIMPORTSERVICE_H
#define IMAGEIMPORTSERVICE_H

#include <QString>
#include <QPainterPath>

class ImageImportService
{
public:
    static QPainterPath processImageToPath(const QString &filePath,
                                           bool internalContours,
                                           bool colorEdges);
};

#endif // IMAGEIMPORTSERVICE_H
