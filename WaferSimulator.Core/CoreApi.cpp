#include "pch.h"
#include "CoreApi.h"

#include <algorithm>
#include <cmath>
#include <opencv2/opencv.hpp>

int Add(int a, int b)
{
    return a + b;
}

int GetStatusCode()
{
    return 2026;
}

namespace
{
    int ClampInt(int value, int minValue, int maxValue)
    {
        return std::min(std::max(value, minValue), maxValue);
    }

    cv::Rect ClampRoi(int roiX, int roiY, int roiWidth, int roiHeight, int width, int height)
    {
        const int x1 = ClampInt(roiX, 0, width);
        const int y1 = ClampInt(roiY, 0, height);
        const int x2 = ClampInt(roiX + roiWidth, 0, width);
        const int y2 = ClampInt(roiY + roiHeight, 0, height);

        if (x2 <= x1 || y2 <= y1)
        {
            return cv::Rect();
        }

        return cv::Rect(x1, y1, x2 - x1, y2 - y1);
    }

    int CountDefectsInRoi(const cv::Mat& defectPointMap, const cv::Rect& roi)
    {
        if (roi.empty())
        {
            return 0;
        }

        cv::Mat integralMap;
        cv::integral(defectPointMap, integralMap, CV_32S);

        const int x1 = roi.x;
        const int y1 = roi.y;
        const int x2 = roi.x + roi.width;
        const int y2 = roi.y + roi.height;

        return integralMap.at<int>(y2, x2)
            - integralMap.at<int>(y1, x2)
            - integralMap.at<int>(y2, x1)
            + integralMap.at<int>(y1, x1);
    }
}

int DetectWaferFaults(
    unsigned char* imageData,
    int width,
    int height,
    int* outXArray,
    int* outYArray,
    int maxFaults,
    int roiX,
    int roiY,
    int roiWidth,
    int roiHeight,
    int* outRoiFaultCount)
{
    if (imageData == nullptr || outXArray == nullptr || outYArray == nullptr
        || width <= 0 || height <= 0 || maxFaults <= 0)
    {
        if (outRoiFaultCount != nullptr)
        {
            *outRoiFaultCount = 0;
        }

        return 0;
    }

    cv::Mat wafer(height, width, CV_8UC1, imageData);

    cv::Mat blurred;
    cv::GaussianBlur(wafer, blurred, cv::Size(3, 3), 0);

    cv::Mat defectMask;
    cv::threshold(blurred, defectMask, 25, 255, cv::THRESH_BINARY);

    cv::Mat labels;
    cv::Mat stats;
    cv::Mat centroids;
    const int numLabels = cv::connectedComponentsWithStats(defectMask, labels, stats, centroids, 8, CV_32S);

    cv::Mat defectPointMap = cv::Mat::zeros(height, width, CV_8UC1);
    int faultCount = 0;

    for (int label = 1; label < numLabels && faultCount < maxFaults; ++label)
    {
        const int area = stats.at<int>(label, cv::CC_STAT_AREA);
        if (area < 1)
        {
            continue;
        }

        const auto centerX = static_cast<int>(std::round(centroids.at<double>(label, 0)));
        const auto centerY = static_cast<int>(std::round(centroids.at<double>(label, 1)));

        outXArray[faultCount] = centerX;
        outYArray[faultCount] = centerY;

        if (0 <= centerX && centerX < width && 0 <= centerY && centerY < height)
        {
            defectPointMap.at<unsigned char>(centerY, centerX) = 1;
        }

        ++faultCount;
    }

    if (outRoiFaultCount != nullptr)
    {
        const cv::Rect roi = ClampRoi(roiX, roiY, roiWidth, roiHeight, width, height);
        *outRoiFaultCount = CountDefectsInRoi(defectPointMap, roi);
    }

    return faultCount;
}
