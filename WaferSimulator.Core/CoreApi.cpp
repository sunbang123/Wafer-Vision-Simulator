#include "pch.h"
#include "CoreApi.h"

#include <algorithm>
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
    enum AlgorithmType
    {
        AlgorithmAutoDetect = 0,
        AlgorithmErosion = 1,
        AlgorithmDilation = 2,
        AlgorithmHistogram = 3,
        AlgorithmGaussian = 4,
        AlgorithmLaplacian = 5,
        AlgorithmThreshold = 6,
        AlgorithmTemplateMatching = 7
    };

    cv::Mat BuildInputMat(unsigned char* imageData, int width, int height, int channels)
    {
        if (imageData == nullptr || width <= 0 || height <= 0) {
            return cv::Mat();
        }

        if (channels == 1) {
            return cv::Mat(height, width, CV_8UC1, imageData);
        }

        if (channels == 3) {
            return cv::Mat(height, width, CV_8UC3, imageData);
        }

        if (channels == 4) {
            return cv::Mat(height, width, CV_8UC4, imageData);
        }

        return cv::Mat();
    }

    cv::Mat ToGray(const cv::Mat& source, int channels)
    {
        cv::Mat gray;

        if (channels == 1) {
            gray = source.clone();
        }
        else if (channels == 3) {
            cv::cvtColor(source, gray, cv::COLOR_BGR2GRAY);
        }
        else {
            cv::cvtColor(source, gray, cv::COLOR_BGRA2GRAY);
        }

        return gray;
    }

    cv::Mat BuildRedDefectMask(const cv::Mat& source, const cv::Mat& waferMask)
    {
        cv::Mat redMask = cv::Mat::zeros(source.size(), CV_8UC1);

        if (source.empty() || waferMask.empty() || source.channels() < 3) {
            return redMask;
        }

        for (int y = 0; y < source.rows; y++) {
            const unsigned char* waferRow = waferMask.ptr<unsigned char>(y);
            unsigned char* maskRow = redMask.ptr<unsigned char>(y);

            if (source.channels() == 4) {
                const cv::Vec4b* sourceRow = source.ptr<cv::Vec4b>(y);
                for (int x = 0; x < source.cols; x++) {
                    const cv::Vec4b& pixel = sourceRow[x];
                    bool redOutlier = pixel[2] > 180 && pixel[2] > pixel[1] + 60 && pixel[2] > pixel[0] + 60;
                    maskRow[x] = waferRow[x] > 0 && redOutlier ? 255 : 0;
                }
            }
            else {
                const cv::Vec3b* sourceRow = source.ptr<cv::Vec3b>(y);
                for (int x = 0; x < source.cols; x++) {
                    const cv::Vec3b& pixel = sourceRow[x];
                    bool redOutlier = pixel[2] > 180 && pixel[2] > pixel[1] + 60 && pixel[2] > pixel[0] + 60;
                    maskRow[x] = waferRow[x] > 0 && redOutlier ? 255 : 0;
                }
            }
        }

        cv::Mat cleanupKernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
        cv::morphologyEx(redMask, redMask, cv::MORPH_OPEN, cleanupKernel);
        cv::morphologyEx(redMask, redMask, cv::MORPH_CLOSE, cleanupKernel);

        return redMask;
    }

    cv::Mat RunErosion(const cv::Mat& gray)
    {
        cv::Mat result = gray.clone();

        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
        cv::Point anchor = cv::Point(-1, -1);
        int iterations = 1;
        cv::erode(gray, result, kernel, anchor, iterations);

        return result;
    }

    cv::Mat RunDilation(const cv::Mat& gray)
    {
        cv::Mat result = gray.clone();

        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
        cv::Point anchor = cv::Point(-1, -1);
        int iterations = 1;
        cv::dilate(gray, result, kernel, anchor, iterations);

        return result;
    }

    cv::Mat RunHistogramEqualization(const cv::Mat& gray)
    {
        cv::Mat result = gray.clone();

        cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2.0, cv::Size(8, 8));
        clahe->apply(gray, result);

        return result;
    }

    cv::Mat RunGaussianBlur(const cv::Mat& gray)
    {
        cv::Mat result = gray.clone();

        int kernelSize = 7;
        double sigma = 1.5;

        cv::GaussianBlur(gray, result, cv::Size(kernelSize, kernelSize), sigma);

        return result;
    }

    cv::Mat RunLaplacian(const cv::Mat& gray)
    {
        cv::Mat result = gray.clone();

        cv::Mat blurred;
        cv::Mat laplacian16;
        cv::GaussianBlur(gray, blurred, cv::Size(3, 3), 0);
        cv::Laplacian(blurred, laplacian16, CV_16S, 3);
        cv::convertScaleAbs(laplacian16, result);

        return result;
    }

    cv::Mat RunThreshold(const cv::Mat& gray)
    {
        cv::Mat result = gray.clone();

        double threshValue = 128; // 임계값을 0-255 사이에서 조절해보세요.
        double maxValue = 255; // 임계값을 넘는 픽셀에 할당할 값입니다.
        cv::threshold(gray, result, threshValue, maxValue, cv::THRESH_BINARY);

        return result;
    }

    cv::Mat RunTemplateMatching(const cv::Mat& gray)
    {
        cv::Mat inverted;
        cv::bitwise_not(gray, inverted);

        cv::Mat defectTemplate = cv::Mat::zeros(13, 13, CV_8UC1);
        cv::circle(defectTemplate, cv::Point(6, 6), 4, cv::Scalar(255), cv::FILLED);

        cv::Mat score;
        cv::matchTemplate(inverted, defectTemplate, score, cv::TM_CCOEFF_NORMED);

        cv::Mat normalizedScore;
        cv::normalize(score, normalizedScore, 0, 255, cv::NORM_MINMAX, CV_8UC1);

        cv::Mat result = cv::Mat::zeros(gray.size(), CV_8UC1);
        cv::Rect roi(
            defectTemplate.cols / 2,
            defectTemplate.rows / 2,
            normalizedScore.cols,
            normalizedScore.rows);
        normalizedScore.copyTo(result(roi));

        return result;
    }

    cv::Mat BuildDefectTemplate()
    {
        cv::Mat defectTemplate = cv::Mat::zeros(13, 13, CV_8UC1);
        cv::circle(defectTemplate, cv::Point(6, 6), 4, cv::Scalar(255), cv::FILLED);
        cv::GaussianBlur(defectTemplate, defectTemplate, cv::Size(3, 3), 0);

        return defectTemplate;
    }

    int FindTemplateMatches(
        const cv::Mat& gray,
        const cv::Mat& waferMask,
        int* outXArray,
        int* outYArray,
        int maxFaults,
        cv::Mat& matchMask,
        cv::Mat& scoreDisplay)
    {
        matchMask = cv::Mat::zeros(gray.size(), CV_8UC1);
        scoreDisplay = cv::Mat::zeros(gray.size(), CV_8UC1);

        if (gray.empty() || waferMask.empty() || outXArray == nullptr || outYArray == nullptr || maxFaults <= 0) {
            return 0;
        }

        cv::Mat inverted;
        cv::bitwise_not(gray, inverted);

        cv::Mat defectTemplate = BuildDefectTemplate();
        cv::Mat score;
        cv::matchTemplate(inverted, defectTemplate, score, cv::TM_CCOEFF_NORMED);

        cv::Mat normalizedScore;
        cv::normalize(score, normalizedScore, 0, 255, cv::NORM_MINMAX, CV_8UC1);

        const int halfWidth = defectTemplate.cols / 2;
        const int halfHeight = defectTemplate.rows / 2;
        cv::Rect scoreRoi(halfWidth, halfHeight, normalizedScore.cols, normalizedScore.rows);
        normalizedScore.copyTo(scoreDisplay(scoreRoi));

        cv::Mat innerWaferMask;
        cv::Mat edgeClearKernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(21, 21));
        cv::erode(waferMask, innerWaferMask, edgeClearKernel);

        const double matchThreshold = 0.45;
        const int maxTemplateMatches = std::min(maxFaults, 20);
        int matchCount = 0;

        while (matchCount < maxTemplateMatches) {
            double maxScore = 0.0;
            cv::Point maxLoc;
            cv::minMaxLoc(score, nullptr, &maxScore, nullptr, &maxLoc);

            if (maxScore < matchThreshold) {
                break;
            }

            cv::Point center(maxLoc.x + halfWidth, maxLoc.y + halfHeight);
            bool insideWafer =
                center.x >= 0 &&
                center.y >= 0 &&
                center.x < innerWaferMask.cols &&
                center.y < innerWaferMask.rows &&
                innerWaferMask.at<unsigned char>(center.y, center.x) > 0;

            if (insideWafer) {
                outXArray[matchCount] = center.x;
                outYArray[matchCount] = center.y;
                cv::rectangle(
                    matchMask,
                    cv::Rect(maxLoc.x, maxLoc.y, defectTemplate.cols, defectTemplate.rows),
                    cv::Scalar(255),
                    2);
                matchCount++;
            }

            cv::Rect suppressRegion(
                std::max(0, maxLoc.x - defectTemplate.cols),
                std::max(0, maxLoc.y - defectTemplate.rows),
                std::min(score.cols - std::max(0, maxLoc.x - defectTemplate.cols), defectTemplate.cols * 3),
                std::min(score.rows - std::max(0, maxLoc.y - defectTemplate.rows), defectTemplate.rows * 3));

            score(suppressRegion).setTo(-1.0f);
        }

        return matchCount;
    }

    cv::Mat RunAlgorithm(const cv::Mat& gray, int algorithmType)
    {
        switch (algorithmType) {
        case AlgorithmErosion:
            return RunErosion(gray);
        case AlgorithmDilation:
            return RunDilation(gray);
        case AlgorithmHistogram:
            return RunHistogramEqualization(gray);
        case AlgorithmGaussian:
            return RunGaussianBlur(gray);
        case AlgorithmLaplacian:
            return RunLaplacian(gray);
        case AlgorithmThreshold:
            return RunThreshold(gray);
        case AlgorithmTemplateMatching:
            return RunTemplateMatching(gray);
        default:
            return gray.clone();
        }
    }

    cv::Mat BuildWaferMask(const cv::Mat& gray)
    {
        cv::Mat mask = cv::Mat::zeros(gray.size(), CV_8UC1);

        cv::threshold(gray, mask, 10, 255, cv::THRESH_BINARY);

        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(9, 9));
        cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);
        cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);

        cv::Mat labels, stats, centroids;
        int labelCount = cv::connectedComponentsWithStats(mask, labels, stats, centroids, 8);
        int largestLabel = 0;
        int largestArea = 0;

        for (int i = 1; i < labelCount; i++) {
            int area = stats.at<int>(i, cv::CC_STAT_AREA);
            if (area > largestArea) {
                largestArea = area;
                largestLabel = i;
            }
        }

        if (largestLabel > 0) {
            mask = labels == largestLabel;
            mask.convertTo(mask, CV_8UC1, 255);
        }

        return mask;
    }

    cv::Mat BuildDefectMask(const cv::Mat& input, const cv::Mat& gray, const cv::Mat& waferMask, cv::Mat* reviewImage = nullptr)
    {
        cv::Mat defectMask = cv::Mat::zeros(gray.size(), CV_8UC1);

        if (gray.empty() || waferMask.empty()) {
            return defectMask;
        }

        cv::Mat redMask = BuildRedDefectMask(input, waferMask);

        cv::Mat background;
        cv::GaussianBlur(gray, background, cv::Size(41, 41), 0);

        cv::Mat localDifference;
        cv::absdiff(gray, background, localDifference);

        if (reviewImage != nullptr) {
            cv::normalize(localDifference, *reviewImage, 0, 255, cv::NORM_MINMAX, CV_8UC1, waferMask);
            cv::bitwise_and(*reviewImage, waferMask, *reviewImage);
        }

        cv::Scalar mean;
        cv::Scalar stddev;
        cv::meanStdDev(localDifference, mean, stddev, waferMask);

        double thresholdValue = std::max(35.0, mean[0] + stddev[0] * 3.5);
        cv::threshold(localDifference, defectMask, thresholdValue, 255, cv::THRESH_BINARY);
        cv::bitwise_or(defectMask, redMask, defectMask);

        cv::Mat innerWaferMask;
        cv::Mat edgeClearKernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(11, 11));
        cv::erode(waferMask, innerWaferMask, edgeClearKernel);
        cv::bitwise_and(defectMask, innerWaferMask, defectMask);

        cv::Mat cleanupKernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
        cv::morphologyEx(defectMask, defectMask, cv::MORPH_OPEN, cleanupKernel);
        cv::morphologyEx(defectMask, defectMask, cv::MORPH_CLOSE, cleanupKernel);

        return defectMask;
    }

    int FillFaults(const cv::Mat& defectMask, int* outXArray, int* outYArray, int maxFaults)
    {
        if (outXArray == nullptr || outYArray == nullptr || maxFaults <= 0) {
            return 0;
        }

        cv::Mat labels, stats, centroids;
        int labelCount = cv::connectedComponentsWithStats(defectMask, labels, stats, centroids, 8);
        int faultCount = 0;

        for (int i = 1; i < labelCount && faultCount < maxFaults; i++) {
            int area = stats.at<int>(i, cv::CC_STAT_AREA);
            int width = stats.at<int>(i, cv::CC_STAT_WIDTH);
            int height = stats.at<int>(i, cv::CC_STAT_HEIGHT);

            if (area < 8 || area > 600 || width > 40 || height > 40) {
                continue;
            }

            outXArray[faultCount] = static_cast<int>(centroids.at<double>(i, 0));
            outYArray[faultCount] = static_cast<int>(centroids.at<double>(i, 1));
            faultCount++;
        }

        return faultCount;
    }

    void WriteProcessedImage(const cv::Mat& source, const cv::Mat& defectMask, unsigned char* processedImageData, int processedBufferLength)
    {
        int requiredLength = source.cols * source.rows * 4;
        if (processedImageData == nullptr || processedBufferLength < requiredLength) {
            return;
        }

        cv::Mat output;
        if (source.channels() == 1) {
            cv::cvtColor(source, output, cv::COLOR_GRAY2BGRA);
        }
        else if (source.channels() == 3) {
            cv::cvtColor(source, output, cv::COLOR_BGR2BGRA);
        }
        else {
            output = source.clone();
        }

        for (int y = 0; y < defectMask.rows; y++) {
            const unsigned char* maskRow = defectMask.ptr<unsigned char>(y);
            cv::Vec4b* outputRow = output.ptr<cv::Vec4b>(y);

            for (int x = 0; x < defectMask.cols; x++) {
                if (maskRow[x] > 0) {
                    outputRow[x] = cv::Vec4b(0, 0, 255, 255);
                }
            }
        }

        std::copy(output.data, output.data + requiredLength, processedImageData);
    }

    void WriteMaskReviewImage(const cv::Mat& baseImage, const cv::Mat& mask, unsigned char* processedImageData, int processedBufferLength)
    {
        int requiredLength = baseImage.cols * baseImage.rows * 4;
        if (processedImageData == nullptr || processedBufferLength < requiredLength) {
            return;
        }

        cv::Mat output;
        if (baseImage.channels() == 1) {
            cv::cvtColor(baseImage, output, cv::COLOR_GRAY2BGRA);
        }
        else if (baseImage.channels() == 3) {
            cv::cvtColor(baseImage, output, cv::COLOR_BGR2BGRA);
        }
        else {
            output = baseImage.clone();
        }

        for (int y = 0; y < mask.rows; y++) {
            const unsigned char* maskRow = mask.ptr<unsigned char>(y);
            cv::Vec4b* outputRow = output.ptr<cv::Vec4b>(y);

            for (int x = 0; x < mask.cols; x++) {
                if (maskRow[x] > 0) {
                    outputRow[x] = cv::Vec4b(0, 0, 255, 255);
                }
            }
        }

        std::copy(output.data, output.data + requiredLength, processedImageData);
    }

    void WriteTemplateReviewImage(const cv::Mat& scoreDisplay, const cv::Mat& matchMask, unsigned char* processedImageData, int processedBufferLength)
    {
        int requiredLength = scoreDisplay.cols * scoreDisplay.rows * 4;
        if (processedImageData == nullptr || processedBufferLength < requiredLength) {
            return;
        }

        cv::Mat output;
        cv::cvtColor(scoreDisplay, output, cv::COLOR_GRAY2BGRA);

        for (int y = 0; y < matchMask.rows; y++) {
            const unsigned char* maskRow = matchMask.ptr<unsigned char>(y);
            cv::Vec4b* outputRow = output.ptr<cv::Vec4b>(y);

            for (int x = 0; x < matchMask.cols; x++) {
                if (maskRow[x] > 0) {
                    outputRow[x] = cv::Vec4b(0, 0, 255, 255);
                }
            }
        }

        std::copy(output.data, output.data + requiredLength, processedImageData);
    }

    void WriteDisplayImage(const cv::Mat& image, unsigned char* processedImageData, int processedBufferLength)
    {
        int requiredLength = image.cols * image.rows * 4;
        if (processedImageData == nullptr || processedBufferLength < requiredLength) {
            return;
        }

        cv::Mat output;
        if (image.channels() == 1) {
            cv::cvtColor(image, output, cv::COLOR_GRAY2BGRA);
        }
        else if (image.channels() == 3) {
            cv::cvtColor(image, output, cv::COLOR_BGR2BGRA);
        }
        else {
            output = image.clone();
        }

        std::copy(output.data, output.data + requiredLength, processedImageData);
    }
}

int DetectWaferFaults(
    unsigned char* imageData,
    int width,
    int height,
    int* outXArray,
    int* outYArray,
    int maxFaults)
{
    return DetectWaferFaultsAdvanced(imageData, width, height, 1, outXArray, outYArray, maxFaults, nullptr, 0);
}

int DetectWaferFaultsAdvanced(
    unsigned char* imageData,
    int width,
    int height,
    int channels,
    int* outXArray,
    int* outYArray,
    int maxFaults,
    unsigned char* processedImageData,
    int processedBufferLength)
{
    cv::Mat input = BuildInputMat(imageData, width, height, channels);
    if (input.empty()) {
        return -1;
    }

    cv::Mat gray = ToGray(input, channels);
    cv::Mat waferMask = BuildWaferMask(gray);
    cv::Mat defectMask = BuildDefectMask(input, gray, waferMask);

    int faultCount = FillFaults(defectMask, outXArray, outYArray, maxFaults);
    WriteProcessedImage(input, defectMask, processedImageData, processedBufferLength);

    return faultCount;
}

int ProcessWaferAlgorithm(
    unsigned char* imageData,
    int width,
    int height,
    int channels,
    int algorithmType,
    int* outXArray,
    int* outYArray,
    int maxFaults,
    unsigned char* processedImageData,
    int processedBufferLength)
{
    cv::Mat input = BuildInputMat(imageData, width, height, channels);
    if (input.empty()) {
        return -1;
    }

    cv::Mat gray = ToGray(input, channels);

    if (algorithmType == AlgorithmAutoDetect) {
        cv::Mat waferMask = BuildWaferMask(gray);
        cv::Mat reviewImage;
        cv::Mat defectMask = BuildDefectMask(input, gray, waferMask, &reviewImage);
        int faultCount = FillFaults(defectMask, outXArray, outYArray, maxFaults);
        WriteMaskReviewImage(reviewImage, defectMask, processedImageData, processedBufferLength);
        return faultCount;
    }

    if (algorithmType == AlgorithmTemplateMatching) {
        cv::Mat waferMask = BuildWaferMask(gray);
        cv::Mat matchMask;
        cv::Mat scoreDisplay;
        int matchCount = FindTemplateMatches(gray, waferMask, outXArray, outYArray, maxFaults, matchMask, scoreDisplay);
        WriteTemplateReviewImage(scoreDisplay, matchMask, processedImageData, processedBufferLength);
        return matchCount;
    }

    cv::Mat result = RunAlgorithm(gray, algorithmType);
    WriteDisplayImage(result, processedImageData, processedBufferLength);

    return 0;
}
