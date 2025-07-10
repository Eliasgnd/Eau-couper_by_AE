#ifndef IMAGEEDGEIMPORTER_H
#define IMAGEEDGEIMPORTER_H

#include <QImage>
#include <QString>

class ImageEdgeImporter
{
public:
    ImageEdgeImporter();

    // Charge l'image couleur depuis filePath, la convertit en niveaux de gris et
    // applique Canny. Renvoie true si tout s'est bien passé et
    // remplit colorImage et edgeImage.
    bool loadAndProcess(const QString &filePath, QImage &colorImage, QImage &edgeImage);
};

#endif // IMAGEEDGEIMPORTER_H
