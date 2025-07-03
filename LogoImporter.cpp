#include "LogoImporter.h"
#include <QImage>
#include <QBitmap>
#include <QRegion>
#include <QPainterPath>
#include <QFileInfo>
#include <QSvgRenderer>
#include <QPainter>
#include <QDebug>

// -------------------------------------------------------------------
// Fonctions statiques internes
// -------------------------------------------------------------------

// Cette fonction effectue une érosion sur une image binaire (Format_Mono).
// L'érosion consiste à rendre blanc un pixel si un de ses voisins dans un noyau 3x3 est blanc.
// Ainsi, un pixel ne restera noir que s'il est entouré de pixels noirs.
static QImage erodeBinary(const QImage &src)
{
    // Crée une copie de l'image source pour la modifier.
    QImage dst = src.copy();
    // On remplit l'image de destination avec du blanc (bits = 1)
    dst.fill(1);

    // Parcours de l'image en excluant le bord (pour éviter les débordements)
    for (int y = 1; y < src.height() - 1; ++y) {
        uchar *dstLine = dst.scanLine(y);
        // Pour chaque pixel (en excluant les bords)
        for (int x = 1; x < src.width() - 1; ++x) {
            bool keepBlack = true;  // On suppose initialement que le pixel doit rester noir.
            // Parcours du voisinage 3x3 autour du pixel (x, y)
            for (int ny = -1; ny <= 1 && keepBlack; ny++) {
                const uchar *lineN = src.constScanLine(y + ny);
                for (int nx = -1; nx <= 1; nx++) {
                    int xx = x + nx;
                    // Vérifie si le pixel voisin est blanc.
                    bool isWhite = (lineN[xx/8] & (0x80 >> (xx % 8))) != 0;
                    if (isWhite) {
                        // Si un voisin est blanc, le pixel central ne doit plus être noir.
                        keepBlack = false;
                        break;
                    }
                }
            }
            // Si tous les voisins sont noirs, on garde le pixel noir (en laissant le bit à 0).
            if (keepBlack) {
                dstLine[x/8] &= ~(0x80 >> (x % 8));
            }
        }
    }
    return dst;
}

// Cette fonction extrait un trait fin (les bords) à partir d'une image binaire.
// Elle effectue une érosion répétée, puis soustrait l'image érodée de l'image originale.
// Le résultat est une image où l'intérieur du trait a été effacé, ne laissant que le contour.
static QImage extractEdgeLine(const QImage &binaryIn, int erosionIterations = 1)
{
    // On commence par éroder l'image binaire plusieurs fois.
    QImage current = binaryIn;
    for (int i = 0; i < erosionIterations; i++) {
        current = erodeBinary(current);
    }
    // Copie de l'image binaire originale pour en extraire les bords.
    QImage edges = binaryIn.copy();
    // Parcours de chaque pixel de l'image
    for (int y = 0; y < edges.height(); ++y) {
        uchar *edgeLine = edges.scanLine(y);
        const uchar *eroLine = current.constScanLine(y);
        const uchar *oriLine = binaryIn.constScanLine(y);
        for (int x = 0; x < edges.width(); ++x) {
            // Vérifie si le pixel était noir dans l'image originale.
            bool wasBlack   = ((oriLine[x/8] & (0x80 >> (x % 8))) == 0);
            // Vérifie si le pixel reste noir après érosion.
            bool stillBlack = ((eroLine[x/8] & (0x80 >> (x % 8))) == 0);
            if (wasBlack && stillBlack) {
                // Si le pixel était noir et est toujours noir après érosion, il fait partie de l'intérieur.
                // On le met en blanc dans l'image des bords pour ne conserver que le contour.
                edgeLine[x/8] |= (0x80 >> (x % 8));
            }
        }
    }
    return edges;
}

// -------------------------------------------------------------------
// Méthode publique de la classe LogoImporter
// -------------------------------------------------------------------
LogoImporter::LogoImporter() {}

// La méthode importLogo charge une image (ou un SVG), la convertit en tracé vectoriel
// et extrait un contour selon l'option includeInternalContours.
// 'threshold' est utilisé pour le seuillage lors de la conversion en image binaire.
QPainterPath LogoImporter::importLogo(const QString &filePath,
                                      bool includeInternalContours,
                                      int threshold)
{
    // 1) Chargement de l'image via QFileInfo et l'extension du fichier
    QFileInfo fi(filePath);
    QString ext = fi.suffix().toLower();
    QImage image;
    if (ext == "svg") {
        // Si c'est un SVG, on utilise QSvgRenderer pour le charger.
        QSvgRenderer renderer(filePath);
        if (!renderer.isValid()) {
            qDebug() << "SVG invalide";
            return QPainterPath();
        }
        QSize defSize = renderer.defaultSize();
        if (!defSize.isValid() || defSize.isEmpty())
            defSize = QSize(500, 500);  // Taille par défaut si nécessaire.
        image = QImage(defSize, QImage::Format_ARGB32);
        image.fill(Qt::white); // Fond blanc
        QPainter p(&image);
        renderer.render(&p);
        p.end();
        qDebug() << "SVG chargé, taille:" << image.size();
    } else {
        // Pour les images raster (JPG, PNG, BMP)
        if (!image.load(filePath)) {
            qDebug() << "Impossible de charger l'image" << filePath;
            return QPainterPath();
        }
        qDebug() << "Image raster chargée, taille:" << image.size();
    }

    // 2) Conversion en niveaux de gris et seuillage.
    // On convertit l'image en format 8 bits de niveaux de gris.
    QImage gray = image.convertToFormat(QImage::Format_Grayscale8);
    qDebug() << "Conversion en niveaux de gris, taille:" << gray.size();

    // Création d'une image binaire (Format_Mono) de la même taille.
    QImage binary(gray.size(), QImage::Format_Mono);
    binary.fill(1); // Remplit toute l'image en blanc par défaut.
    int adjustedThreshold = threshold; // Utilisation de la valeur passée en paramètre.
    // Pour chaque pixel, si la luminosité est inférieure au seuil,
    // le pixel est considéré comme noir (bit mis à 0).
    for (int y = 0; y < gray.height(); ++y) {
        const uchar *lineGray = gray.constScanLine(y);
        uchar *lineBin = binary.scanLine(y);
        for (int x = 0; x < gray.width(); ++x) {
            if (lineGray[x] < adjustedThreshold) {
                lineBin[x/8] &= ~(0x80 >> (x % 8)); // Définit le pixel comme noir.
            }
        }
    }
    qDebug() << "Seuillage terminé.";
    // Vous pouvez décommenter la ligne suivante pour sauvegarder l'image binaire et vérifier le seuillage.
    // binary.save("debug_binary.png");

    // 3) Traitement spécifique en fonction du type de fichier.
    QImage finalBinary;
    if (ext == "svg") {
        // Pour un SVG, on conserve la silhouette telle quelle.
        finalBinary = binary;
        qDebug() << "SVG => pas d'extraction des bords.";
    } else {
        // Pour un JPG/PNG/BMP, on extrait un trait unique en utilisant l'érosion + soustraction.
        QImage edges = extractEdgeLine(binary, 1);
        finalBinary = edges;
        qDebug() << "Extraction des bords effectuée. edges size:" << edges.size();
    }

    // 4) Conversion de l'image binaire finale en QRegion, puis en QPainterPath.
    QBitmap bmp = QBitmap::fromImage(finalBinary);
    QRegion reg(bmp);

    // Itération sur QRegion grâce aux itérateurs natifs (Qt 6)
    QList<QRect> rects;
    for (QRegion::const_iterator it = reg.begin(); it != reg.end(); ++it) {
        rects.append(*it);
    }
    qDebug() << "Nombre de rectangles dans la région:" << rects.size();

    // On construit un QPainterPath en ajoutant chacun des rectangles.
    QPainterPath rawPath;
    for (const QRect &r : rects) {
        rawPath.addRect(r);
    }
    // Simplifie le chemin pour réduire les irrégularités.
    rawPath = rawPath.simplified();

    // 5) Séparation du chemin en sous-chemins.
    // Chaque "moveTo" indique le début d'un nouveau sous-chemin.
    QList<QPainterPath> subpaths;
    QPainterPath current;
    int count = rawPath.elementCount();
    for (int i = 0; i < count; i++) {
        QPainterPath::Element e = rawPath.elementAt(i);
        if (e.isMoveTo()) {
            if (!current.isEmpty()) {
                subpaths.append(current);
            }
            current = QPainterPath();
            current.moveTo(e.x, e.y);
        } else {
            current.lineTo(e.x, e.y);
        }
    }
    if (!current.isEmpty())
        subpaths.append(current);

    if (subpaths.isEmpty()) {
        qDebug() << "Aucun sous-chemin détecté => chemin vide.";
        return QPainterPath();
    }

    // 6) Option : inclure ou non les contours internes.
    // Si includeInternalContours est vrai, on fusionne tous les sous-chemins.
    // Sinon, on sélectionne seulement le sous-chemin dont la bounding box est la plus grande.
    if (includeInternalContours) {
        QPainterPath unionPath;
        for (const QPainterPath &sp : subpaths) {
            unionPath.addPath(sp);
        }
        unionPath = unionPath.simplified();
        qDebug() << "Contours internes inclus. boundingRect:" << unionPath.boundingRect();
        return unionPath;
    } else {
        double maxArea = 0.0;
        QPainterPath bestPath;
        for (const QPainterPath &sp : subpaths) {
            QRectF br = sp.boundingRect();
            double area = br.width() * br.height();
            if (area > maxArea) {
                maxArea = area;
                bestPath = sp;
            }
        }
        bestPath = bestPath.simplified();
        qDebug() << "Contours internes NON inclus. boundingRect:" << bestPath.boundingRect();
        return bestPath;
    }
}
