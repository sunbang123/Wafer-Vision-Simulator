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

    cv::Mat RunErosion(const cv::Mat& gray)
    {
        cv::Mat result = gray.clone();

        // TODO: implement erosion.
        // Hint: create a structuring element with cv::getStructuringElement,
        // then call cv::erode(gray, result, kernel, anchor, iterations).
        // Try MORPH_RECT first, then compare MORPH_ELLIPSE for wafer-like shapes.

        return result;
    }

    cv::Mat RunDilation(const cv::Mat& gray)
    {
        cv::Mat result = gray.clone();

        // TODO: implement dilation.
        // Hint: create a kernel, then call cv::dilate.
        // Dilation can thicken bright defect regions or connect broken scratch edges.

        return result;
    }

    cv::Mat RunHistogramEqualization(const cv::Mat& gray)
    {
        cv::Mat result = gray.clone();

        // TODO: implement histogram processing.
        // Hint: start with cv::equalizeHist(gray, result).
        // For local contrast control, try cv::createCLAHE and compare it with global equalization.

        return result;
    }

    cv::Mat RunGaussianBlur(const cv::Mat& gray)
    {
        cv::Mat result = gray.clone();

        // TODO: implement Gaussian blur.
        // Hint: call cv::GaussianBlur(gray, result, cv::Size(odd, odd), sigma).
        // Kernel width/height must be odd numbers such as 3, 5, 7, or 9.

        return result;
    }

    cv::Mat RunLaplacian(const cv::Mat& gray)
    {
        cv::Mat result = gray.clone();

        // TODO: implement Laplacian edge extraction.
        // Hint: use a temporary CV_16S matrix for cv::Laplacian,
        // then convert it back with cv::convertScaleAbs.
        // Compare kernel sizes 1, 3, and 5.

        return result;
    }

    cv::Mat RunThreshold(const cv::Mat& gray)
    {
        cv::Mat result = gray.clone();

        // TODO: implement binary thresholding.
        // Hint: compare fixed threshold, Otsu threshold, and cv::adaptiveThreshold.
        // A binary result can later be passed to connectedComponentsWithStats for coordinates.

        return result;
    }

    cv::Mat RunTemplateMatching(const cv::Mat& gray)
    {
        cv::Mat result = gray.clone();

        // TODO: implement template matching.
        // Hint: prepare a template cv::Mat, call cv::matchTemplate,
        // then use cv::minMaxLoc or threshold the score map to find matches.
        // Later, expose a template image path from C# or cache known templates here.

        return result;
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

        // TODO: implement wafer-area masking for auto detection.
        // Hint: start with thresholding to separate the wafer from the dark background.
        // Then use morphology close/open to remove tiny holes or isolated background noise.
        // Example direction:
        //   1. cv::threshold(gray, mask, value, 255, cv::THRESH_BINARY)
        //   2. cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel)
        //   3. cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel)

        return mask;
    }

    cv::Mat BuildDefectMask(const cv::Mat& gray, const cv::Mat& waferMask)
    {
        cv::Mat defectMask = cv::Mat::zeros(gray.size(), CV_8UC1);

        // TODO: implement automatic defect detection.
        // Hint: this is where you can combine the individual algorithms.
        // Suggested pipeline to experiment with:
        //   1. Improve contrast with histogram equalization or CLAHE.
        //   2. Suppress random noise with Gaussian blur.
        //   3. Estimate background/pattern using a larger blur or morphology.
        //   4. Compare original vs background with cv::absdiff.
        //   5. Convert the residual to binary using threshold or adaptiveThreshold.
        //   6. Optionally add edge defects using Laplacian or Canny.
        //   7. Limit detections to waferMask with cv::bitwise_and.
        //   8. Clean the mask with erosion/dilation/open/close.
        //
        // Return a CV_8UC1 binary mask:
        //   0   = normal area
        //   255 = defect candidate
        (void)waferMask;

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

            if (area < 3 || area > 5000 || width > 160 || height > 160) {
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
    cv::Mat defectMask = BuildDefectMask(gray, waferMask);

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
        cv::Mat defectMask = BuildDefectMask(gray, waferMask);
        int faultCount = FillFaults(defectMask, outXArray, outYArray, maxFaults);
        WriteProcessedImage(input, defectMask, processedImageData, processedBufferLength);
        return faultCount;
    }

    cv::Mat result = RunAlgorithm(gray, algorithmType);
    WriteDisplayImage(result, processedImageData, processedBufferLength);

    return 0;
}
