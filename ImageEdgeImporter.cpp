#include "ImageEdgeImporter.h"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/photo.hpp>          // ← pour edgePreservingFilter
#include <algorithm>                  // std::max_element

bool ImageEdgeImporter::loadAndProcess(const QString& filePath,
                                       QPainterPath& edgePath)
{
    /* 0) — lecture de l’image ------------------------------------------------ */
    cv::Mat src = cv::imread(filePath.toStdString());
    if (src.empty()) return false;

    /* 0b) — redimensionnement facultatif ------------------------------------- */
    if (m_p.resize_max_w > 0 && src.cols > m_p.resize_max_w) {
        double scale = static_cast<double>(m_p.resize_max_w) / src.cols;
        cv::resize(src, src, {}, scale, scale, cv::INTER_AREA);
    }

    /* 1) — lissage qui préserve les bords ------------------------------------ */
    cv::Mat smooth;
    if (m_p.bilateral_d > 0) {
        cv::bilateralFilter(src, smooth,
                            m_p.bilateral_d,
                            m_p.bilateral_sigma,
                            m_p.bilateral_sigma);
    } else {
        cv::edgePreservingFilter(src, smooth,
                                 cv::RECURS_FILTER, 60, 0.4);
    }

    /* 2) — Lab‑L + CLAHE → gray --------------------------------------------- */
    cv::Mat lab; cv::cvtColor(smooth, lab, cv::COLOR_BGR2Lab);
    std::vector<cv::Mat> ch; cv::split(lab, ch);
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(
        static_cast<double>(m_p.clahe_clip), cv::Size(8,8));
    cv::Mat gray; clahe->apply(ch[0], gray);

    /* 3) — création du masque binaire --------------------------------------- */
    cv::Mat mask;
    if (m_p.useCanny) {
        cv::Canny(gray, mask, m_p.canny_low, m_p.canny_high);
        cv::dilate(mask, mask,
                   cv::getStructuringElement(cv::MORPH_ELLIPSE,{5,5}));
        cv::erode (mask, mask,
                  cv::getStructuringElement(cv::MORPH_ELLIPSE,{5,5}));
    } else {
        cv::adaptiveThreshold(gray, mask, 255,
                              cv::ADAPTIVE_THRESH_MEAN_C,
                              cv::THRESH_BINARY_INV,
                              m_p.adapt_block | 1, m_p.adapt_C);

        if (m_p.morph_close) {
            cv::morphologyEx(mask, mask, cv::MORPH_CLOSE,
                             cv::getStructuringElement(cv::MORPH_ELLIPSE,
                                                       {m_p.morph_kernel,
                                                        m_p.morph_kernel}));
        }
    }

    /* 4) — détection des contours externes ---------------------------------- */
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours,
                     cv::RETR_EXTERNAL,
                     cv::CHAIN_APPROX_SIMPLE);      // ← lissage immédiat

    if (contours.empty()) return false;

    /* 4b) — on enlève les contours trop petits ------------------------------ */
    double maxArea = 0;
    for (const auto& c : contours) maxArea = std::max(maxArea, cv::contourArea(c));
    contours.erase(std::remove_if(contours.begin(), contours.end(),
                                  [maxArea](const std::vector<cv::Point>& c){
                                      return cv::contourArea(c) < 0.30 * maxArea;
                                  }),
                   contours.end());
    if (contours.empty()) return false;

    /* 5) — on garde le plus grand contour ----------------------------------- */
    const auto& best = *std::max_element(contours.begin(), contours.end(),
                                         [](const auto& a, const auto& b){
                                             return cv::contourArea(a) < cv::contourArea(b);
                                         });

    /* 6) — simplification (approxPolyDP) ------------------------------------ */
    const std::vector<cv::Point>* ptsPtr = &best;
    std::vector<cv::Point> approx;
    if (m_p.epsilon_percent > 0.0) {
        double eps = m_p.epsilon_percent * cv::arcLength(best, true);
        cv::approxPolyDP(best, approx, eps, true);
        if (approx.size() >= 4) ptsPtr = &approx;
    }
    const auto& pts = *ptsPtr;
    if (pts.size() < 4) return false;

    /* 7) — conversion OpenCV → QPainterPath --------------------------------- */
    QPainterPath path;
    path.moveTo(pts[0].x, pts[0].y);
    for (size_t i = 1; i < pts.size(); ++i)
        path.lineTo(pts[i].x, pts[i].y);
    path.closeSubpath();

    edgePath = m_p.final_simplify ? path.simplified() : path;
    return true;
}
