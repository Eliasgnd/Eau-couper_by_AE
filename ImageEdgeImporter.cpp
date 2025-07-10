#include "ImageEdgeImporter.h"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>

/*  ──────────────────────────────────────────────
    Étapes :
    1. Bilatéral (lisse couleur, garde bords)
    2. Luminance Lab + CLAHE (contraste local)
    3. AdaptiveThreshold local
    4. Morphologie légère pour combler pointillés
    5. findContours (RETR_EXTERNAL + CHAIN_NONE)
    ────────────────────────────────────────────── */
bool ImageEdgeImporter::loadAndProcess(const QString &filePath,
                                       QPainterPath &edgePath)
{
    cv::Mat src = cv::imread(filePath.toStdString());
    if (src.empty()) return false;

    /* 1) ­— filtre bilatéral */
    cv::Mat smooth;
    cv::bilateralFilter(src, smooth,
                        m_p.bilateral_d,
                        m_p.bilateral_sigma,
                        m_p.bilateral_sigma);

    /* 2) ­— Lab + CLAHE sur L */
    cv::Mat lab; cv::cvtColor(smooth, lab, cv::COLOR_BGR2Lab);
    std::vector<cv::Mat> ch; cv::split(lab, ch);      // ch[0] = L
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(
        static_cast<double>(m_p.clahe_clip), cv::Size(8,8));
    clahe->apply(ch[0], ch[0]);                      // renforce contraste
    cv::Mat gray = ch[0];

    /* 3) ­— seuillage adaptatif */
    cv::Mat bin;
    cv::adaptiveThreshold(gray, bin, 255,
                          cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY_INV,
                          m_p.adapt_block | 1 /*force impair*/, m_p.adapt_C);

    /* 4) ­— fermeture 3×3 (facultatif) */
    if (m_p.morph_close) {
        cv::morphologyEx(bin, bin, cv::MORPH_CLOSE,
                         cv::getStructuringElement(cv::MORPH_ELLIPSE,
                                                   {m_p.morph_kernel, m_p.morph_kernel}));
    }

    /* 5) ­— contours externes complets */
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(bin, contours,
                     cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
    if (contours.empty()) return false;

    auto best = *std::max_element(contours.begin(), contours.end(),
                                  [](auto &a, auto &b){ return cv::contourArea(a) < cv::contourArea(b); });

    /* 6) ­— simplification douce (optionnel) */
    const std::vector<cv::Point> *ptsPtr = &best;
    std::vector<cv::Point> approx;
    if (m_p.epsilon_percent > 0.0) {
        double eps = m_p.epsilon_percent *
                     cv::arcLength(best, /*closed=*/true);
        cv::approxPolyDP(best, approx, eps, true);
        if (!approx.empty()) ptsPtr = &approx;
    }
    const std::vector<cv::Point> &pts = *ptsPtr;
    if (pts.size() < 8) return false;

    /* 7) ­— OpenCV → QPainterPath */
    QPainterPath path;
    path.moveTo(pts[0].x, pts[0].y);
    for (size_t i = 1; i < pts.size(); ++i)
        path.lineTo(pts[i].x, pts[i].y);
    path.closeSubpath();

    edgePath = m_p.final_simplify ? path.simplified() : path;
    return true;
}
