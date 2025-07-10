#include "ImageEdgeImporter.h"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs/legacy/constants_c.h>

ImageEdgeImporter::ImageEdgeImporter() {}

bool ImageEdgeImporter::loadAndProcess(const QString &filePath,
                                       QImage &colorImage,
                                       QImage &edgeImage)
{
    cv::Mat img = cv::imread(filePath.toStdString(), cv::IMREAD_COLOR);
    if (img.empty())
        return false;

    // ---
    // Apply a small Gaussian blur to smooth the image before extracting edges.
    // This reduces noise and results in cleaner contours.
    cv::Mat imgBlurred;
    cv::GaussianBlur(img, imgBlurred, cv::Size(5, 5), 0);

    cv::Mat imgRgb;
    cv::cvtColor(imgBlurred, imgRgb, cv::COLOR_BGR2RGB);
    colorImage = QImage(imgRgb.data, imgRgb.cols, imgRgb.rows,
                        static_cast<int>(imgRgb.step), QImage::Format_RGB888).copy();

    cv::Mat gray;
    cv::cvtColor(imgBlurred, gray, cv::COLOR_BGR2GRAY);
    cv::Mat edges;
    cv::Canny(gray, edges, 100, 200);

    cv::Mat inverted;
    cv::bitwise_not(edges, inverted);
    edgeImage = QImage(inverted.data, inverted.cols, inverted.rows,
                       static_cast<int>(inverted.step), QImage::Format_Grayscale8).copy();

    return true;
}
