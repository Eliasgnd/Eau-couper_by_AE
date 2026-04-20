#include "ShapeValidationService.h"
#include "GeometryUtils.h"
#include <cmath>
#include <algorithm> // Requis pour std::max

ShapeValidationResult ShapeValidationService::validate(const QList<QPainterPath> &paths,
                                                       const QRectF &bounds,
                                                       qreal minIntersectionSize)
{
    ShapeValidationResult result;
    result.allValid = true;

    // 1. Test de dépassement des limites du plateau (Out of Bounds)
    for (int i = 0; i < paths.size(); ++i) {
        const QRectF rect = paths[i].boundingRect();
        // BONUS : On agrandit virtuellement le plateau de 0.5mm pour absorber
        // les erreurs d'arrondi si une pièce est posée exactement sur le trait du bord.
        if (!bounds.adjusted(-0.5, -0.5, 0.5, 0.5).contains(rect)) {
            result.allValid = false;
            result.outOfBoundsIndices.insert(i);
        }
    }

    // 2. Test de collision réelle (Basé sur l'ÉPAISSEUR de l'intersection)
    for (int i = 0; i < paths.size(); ++i) {
        for (int j = i + 1; j < paths.size(); ++j) {

            const QPainterPath inter = paths[i].intersected(paths[j]);
            if (inter.isEmpty()) continue;

            // Calcul de la surface (Aire) de l'intersection
            double totalArea = 0.0;
            const QList<QPolygonF> polys = inter.toFillPolygons();

            for (const QPolygonF &poly : polys) {
                double polyArea = 0.0;
                for (int k = 0; k < poly.size() - 1; ++k) {
                    polyArea += poly[k].x() * poly[k+1].y() - poly[k+1].x() * poly[k].y();
                }
                totalArea += std::abs(polyArea) / 2.0;
            }

            // Si l'aire est absolument microscopique, c'est du bruit mathématique pur
            if (totalArea < 0.5) continue;

            // --- LA MAGIE EST ICI : Calcul de l'épaisseur ---
            // On prend la plus grande dimension (largeur ou hauteur) de l'intersection
            QRectF interBounds = inter.boundingRect();
            double maxDimension = std::max(interBounds.width(), interBounds.height());

            if (maxDimension <= 0.0) continue;

            // L'épaisseur correspond à l'Aire divisée par la Longueur
            double averageThickness = totalArea / maxDimension;

            // TOLÉRANCE D'ÉPAISSEUR : On autorise les lignes à se superposer (s'empiler)
            // jusqu'à 1.5 millimètres de "pénétration" l'une dans l'autre.
            const double MAX_ALLOWED_THICKNESS = 1.5;

            // Si le chevauchement est plus épais que 1.5 mm, c'est une vraie collision !
            if (averageThickness > MAX_ALLOWED_THICKNESS) {
                result.allValid = false;
                result.collisionIndices.insert(i);
                result.collisionIndices.insert(j);
            }
        }
    }

    return result;
}
