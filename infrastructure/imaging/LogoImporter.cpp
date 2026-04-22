#include "LogoImporter.h"
#include <QImage>
#include <QPainterPath>
#include <QFileInfo>
#include <QSvgRenderer>
#include <QPainter>
#include <QDebug>

// --- AJOUT POUR OPENCV ---
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

// Fonction utilitaire pour convertir les contours lissés d'OpenCV en QPainterPath Qt
static QPainterPath convertContoursToPath(const std::vector<std::vector<cv::Point>>& contours) {
    QPainterPath path;
    for (const auto& contour : contours) {
        // Un contour doit avoir au moins 3 points pour former une forme fermée
        if (contour.size() < 3) continue;

        QPainterPath subPath;
        subPath.moveTo(contour[0].x, contour[0].y);
        for (size_t i = 1; i < contour.size(); ++i) {
            subPath.lineTo(contour[i].x, contour[i].y);
        }
        subPath.closeSubpath();
        path.addPath(subPath);
    }
    return path;
}

// -------------------------------------------------------------------
// Méthode publique de la classe LogoImporter
// -------------------------------------------------------------------
LogoImporter::LogoImporter() {}

QPainterPath LogoImporter::importLogo(const QString &filePath,
                                      bool includeInternalContours,
                                      int threshold)
{
    // 1) Chargement de l'image (SVG ou Raster)
    QFileInfo fi(filePath);
    QString ext = fi.suffix().toLower();
    QImage image;

    if (ext == "svg") {
        QSvgRenderer renderer(filePath);
        if (!renderer.isValid()) {
            qDebug() << "SVG invalide";
            return QPainterPath();
        }
        QSize defSize = renderer.defaultSize();
        if (!defSize.isValid() || defSize.isEmpty())
            defSize = QSize(500, 500);
        image = QImage(defSize, QImage::Format_ARGB32);
        image.fill(Qt::white);
        QPainter p(&image);
        renderer.render(&p);
        p.end();
        qDebug() << "SVG chargé via rendu, taille:" << image.size();
    } else {
        if (!image.load(filePath)) {
            qDebug() << "Impossible de charger l'image" << filePath;
            return QPainterPath();
        }
        qDebug() << "Image raster chargée, taille:" << image.size();
    }

    // 2) Conversion en niveaux de gris pour OpenCV
    QImage gray = image.convertToFormat(QImage::Format_Grayscale8);

    // 3) Création de la matrice OpenCV (cv::Mat) à partir de la QImage
    // On "enveloppe" la mémoire de l'image Qt pour éviter de la copier, c'est très rapide.
    cv::Mat mat(gray.height(), gray.width(), CV_8UC1,
                const_cast<uchar*>(gray.constBits()), gray.bytesPerLine());

    // 4) Seuillage (Threshold) via OpenCV
    cv::Mat binaryMat;
    // THRESH_BINARY_INV : Les parties foncées (traits du logo) deviennent blanches (255)
    // car OpenCV cherche les contours des zones blanches.
    cv::threshold(mat, binaryMat, threshold, 255, cv::THRESH_BINARY_INV);

    // 5) Extraction intelligente des contours
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    // RETR_CCOMP : Permet d'identifier les contours extérieurs et les trous intérieurs.
    // CHAIN_APPROX_TC89_L1 : Un premier algorithme qui repère les points de courbure naturels.
    cv::findContours(binaryMat, contours, hierarchy, cv::RETR_CCOMP, cv::CHAIN_APPROX_TC89_L1);

    if (contours.empty()) {
        qDebug() << "Aucun contour détecté.";
        return QPainterPath();
    }

    // 6) Lissage mathématique et filtrage des contours
    std::vector<std::vector<cv::Point>> processedContours;
    double maxArea = 0.0;
    int bestIdx = -1;

    for (size_t i = 0; i < contours.size(); i++) {
        std::vector<cv::Point> approx;

        // PARAMÈTRE DE LISSAGE : "epsilon"
        // 1.0 est une excellente valeur de départ.
        // Si les courbes sont encore un peu pixelisées, montez à 1.5 ou 2.0.
        // Si le lissage efface trop de détails (arrondit trop les angles), descendez à 0.5.
        double epsilon = 0.8;
        cv::approxPolyDP(contours[i], approx, epsilon, true);

        if (includeInternalContours) {
            processedContours.push_back(approx);
        } else {
            // Si on ne veut que l'externe, on cherche le contour avec la plus grande surface globale
            double area = cv::contourArea(approx);
            if (area > maxArea) {
                maxArea = area;
                bestIdx = processedContours.size(); // position où il va être inséré
                processedContours.push_back(approx);
            }
        }
    }

    // 7) Construction du QPainterPath à renvoyer à l'interface
    QPainterPath finalPath;
    if (!includeInternalContours && bestIdx != -1) {
        // On ne garde que le "bestIdx" (le plus gros)
        std::vector<std::vector<cv::Point>> singleContour = {processedContours[bestIdx]};
        finalPath = convertContoursToPath(singleContour);
        qDebug() << "Contour externe lissé généré.";
    } else {
        // On ajoute tout
        finalPath = convertContoursToPath(processedContours);
        qDebug() << "Contours complets (avec internes) lissés générés.";
    }

    // Simplification finale par Qt pour s'assurer que le chemin est propre
    return finalPath.simplified();
}
