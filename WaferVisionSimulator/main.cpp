#include <cmath>
#include <iostream>
#include <opencv2/opencv.hpp>

int main()
{
    constexpr int width = 500;
    constexpr int height = 500;

    cv::Mat wafer = cv::Mat::zeros(height, width, CV_8UC1);
    cv::circle(wafer, cv::Point(150, 100), 2, cv::Scalar(255), cv::FILLED);
    cv::circle(wafer, cv::Point(300, 250), 3, cv::Scalar(255), cv::FILLED);
    cv::line(wafer, cv::Point(420, 380), cv::Point(460, 410), cv::Scalar(255), 2);

    cv::Mat blurred;
    cv::GaussianBlur(wafer, blurred, cv::Size(3, 3), 0);

    cv::Mat defectMask;
    cv::threshold(blurred, defectMask, 25, 255, cv::THRESH_BINARY);

    cv::Mat labels;
    cv::Mat stats;
    cv::Mat centroids;
    const int labelCount = cv::connectedComponentsWithStats(defectMask, labels, stats, centroids, 8, CV_32S);

    std::cout << "Detected defects: " << labelCount - 1 << '\n';
    for (int label = 1; label < labelCount; ++label)
    {
        std::cout << " defect " << label
            << " center=(" << static_cast<int>(std::round(centroids.at<double>(label, 0)))
            << ", " << static_cast<int>(std::round(centroids.at<double>(label, 1)))
            << ") area=" << stats.at<int>(label, cv::CC_STAT_AREA)
            << '\n';
    }

    return 0;
}
