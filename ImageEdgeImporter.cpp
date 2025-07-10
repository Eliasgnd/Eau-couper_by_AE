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

    cv::Mat imgRgb;
    cv::cvtColor(img, imgRgb, cv::COLOR_BGR2RGB);
    colorImage = QImage(imgRgb.data, imgRgb.cols, imgRgb.rows,
                        static_cast<int>(imgRgb.step), QImage::Format_RGB888).copy();

    cv::Mat gray;
    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    cv::Mat edges;
    cv::Canny(gray, edges, 100, 200);

    cv::Mat inverted;
    cv::bitwise_not(edges, inverted);
    edgeImage = QImage(inverted.data, inverted.cols, inverted.rows,
                       static_cast<int>(inverted.step), QImage::Format_Grayscale8).copy();

    return true;
}
