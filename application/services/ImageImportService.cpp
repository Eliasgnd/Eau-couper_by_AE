#include "ImageImportService.h"

#include "LogoImporter.h"
#include "ImageEdgeImporter.h"

QPainterPath ImageImportService::processImageToPath(const QString &filePath,
                                                    bool internalContours,
                                                    bool colorEdges)
{
    QPainterPath outline;

    if (colorEdges) {
        ImageEdgeImporter edgeImporter;
        if (!edgeImporter.loadAndProcess(filePath, outline))
            return QPainterPath();
        return outline;
    }

    LogoImporter importer;
    outline = importer.importLogo(filePath, internalContours, 128);
    return outline;
}
