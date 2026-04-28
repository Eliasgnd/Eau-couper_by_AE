#ifndef LOGOIMPORTER_H
#define LOGOIMPORTER_H

#include <QPainterPath>
#include <QString>

// Classe permettant d'importer un logo et de générer un chemin graphique (QPainterPath)
class LogoImporter
{
public:
    LogoImporter(); // Constructeur

    // Méthode pour importer un logo à partir d'un fichier image
    // - filePath : Chemin du fichier image à importer
    // - includeInternalContours : Booléen indiquant s'il faut inclure les contours internes (true) ou uniquement le contour extérieur (false)
    // - threshold : Valeur du seuil pour la conversion en contours (entre 0 et 255)
    QPainterPath importLogo(const QString &filePath,
                            bool includeInternalContours = false,
                            int threshold = 128,
                            int maxOutputPoints = 4000,
                            int maxRasterSize = 1400);
};

#endif // LOGOIMPORTER_H
