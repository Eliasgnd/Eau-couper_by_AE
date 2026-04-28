#include "ImageEdgeImporter.h"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <algorithm> // std::max_element

bool ImageEdgeImporter::loadAndProcess(const QString& filePath,
                                       QPainterPath& edgePath)
{
    /* 0) — lecture de l’image ------------------------------------------------ */
    cv::Mat src = cv::imread(filePath.toStdString());
    if (src.empty()) return false;

    /* 0b) — OPTIMISATION VITESSE : Redimensionnement agressif ---------------- */
    // Pour une silhouette, traiter plus de 1000 pixels de large est inutile
    // et ralentit tout le pipeline. On force une taille raisonnable.
    int max_width = (m_p.resize_max_w > 0) ? m_p.resize_max_w : 1000;
    if (src.cols > max_width) {
        double scale = static_cast<double>(max_width) / src.cols;
        cv::resize(src, src, cv::Size(), scale, scale, cv::INTER_AREA);
    }

    /* 1) — OPTIMISATION VITESSE : Flou Rapide -------------------------------- */
    // On remplace bilateralFilter et edgePreservingFilter (très lents)
    // par un GaussianBlur (ultra rapide) pour enlever le bruit de l'image.
    cv::Mat smooth;
    cv::GaussianBlur(src, smooth, cv::Size(5, 5), 0);

    /* 2) — Lab‑L + CLAHE → gray (Garde un bon contraste) --------------------- */
    cv::Mat lab;
    cv::cvtColor(smooth, lab, cv::COLOR_BGR2Lab);
    std::vector<cv::Mat> ch;
    cv::split(lab, ch);
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(
        static_cast<double>(m_p.clahe_clip), cv::Size(8,8));
    cv::Mat gray;
    clahe->apply(ch[0], gray);

    /* 3) — Création du masque binaire --------------------------------------- */
    cv::Mat mask;
    if (m_p.useCanny) {
        cv::Canny(gray, mask, m_p.canny_low, m_p.canny_high);
        // Fermeture des trous pour avoir une forme pleine
        cv::morphologyEx(mask, mask, cv::MORPH_CLOSE,
                         cv::getStructuringElement(cv::MORPH_ELLIPSE, {5,5}));
    } else {
        cv::adaptiveThreshold(gray, mask, 255,
                              cv::ADAPTIVE_THRESH_MEAN_C,
                              cv::THRESH_BINARY_INV,
                              m_p.adapt_block | 1, m_p.adapt_C);

        if (m_p.morph_close) {
            cv::morphologyEx(mask, mask, cv::MORPH_CLOSE,
                             cv::getStructuringElement(cv::MORPH_ELLIPSE,
                                                       {m_p.morph_kernel, m_p.morph_kernel}));
        }
    }

    /* 3b) — OPTIMISATION RENDU : Le "Masque Liquide" ------------------------ */
    // Cette étape magique floute les pixels en escalier du masque binaire
    // et les re-solidifie. Les micro-détails tremblants disparaissent,
    // laissant une silhouette lisse et continue, idéale pour la découpe.
    cv::GaussianBlur(mask, mask, cv::Size(7, 7), 0);
    cv::threshold(mask, mask, 127, 255, cv::THRESH_BINARY);

    /* 4) — Détection des contours externes ---------------------------------- */
    std::vector<std::vector<cv::Point>> contours;
    // RETR_EXTERNAL pour ne prendre QUE la silhouette extérieure
    // CHAIN_APPROX_TC89_L1 pour avoir des courbes intelligentes
    cv::findContours(mask, contours,
                     cv::RETR_EXTERNAL,
                     cv::CHAIN_APPROX_TC89_L1);

    if (contours.empty()) return false;

    /* 5) — On garde directement le plus grand contour ----------------------- */
    const auto& best = *std::max_element(contours.begin(), contours.end(),
                                         [](const auto& a, const auto& b){
                                             return cv::contourArea(a) < cv::contourArea(b);
                                         });

    /* 6) — Simplification légère (approxPolyDP) ----------------------------- */
    const std::vector<cv::Point>* ptsPtr = &best;
    std::vector<cv::Point> approx;

    if (m_p.epsilon_percent > 0.0) {
        // Comme on a appliqué le Masque Liquide, on peut réduire epsilon
        // pour ne pas trop "casser" les arrondis que l'on vient de créer.
        double eps = m_p.epsilon_percent * cv::arcLength(best, true);
        cv::approxPolyDP(best, approx, eps, true);
        for (int i = 0; i < 8 && m_p.max_output_points > 0
             && static_cast<int>(approx.size()) > m_p.max_output_points; ++i) {
            eps *= 1.6;
            cv::approxPolyDP(best, approx, eps, true);
        }
        if (approx.size() >= 3) ptsPtr = &approx;
    }

    const auto& pts = *ptsPtr;
    if (pts.size() < 3) return false;

    /* 7) — Conversion OpenCV → QPainterPath --------------------------------- */
    QPainterPath path;
    path.moveTo(pts[0].x, pts[0].y);
    for (size_t i = 1; i < pts.size(); ++i) {
        path.lineTo(pts[i].x, pts[i].y);
    }
    path.closeSubpath();

    edgePath = (m_p.final_simplify
                && (m_p.max_output_points <= 0 || path.elementCount() <= m_p.max_output_points * 2))
        ? path.simplified()
        : path;
    return true;
}
