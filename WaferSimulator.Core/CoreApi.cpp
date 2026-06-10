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

        // TODO: 침식 알고리즘을 구현하세요.
        // 힌트: cv::getStructuringElement로 구조 요소(kernel)를 만든 뒤,
        // cv::erode(gray, result, kernel, anchor, iterations)를 호출해보세요.
        // 먼저 MORPH_RECT를 써보고, 웨이퍼처럼 둥근 형태에는 MORPH_ELLIPSE도 비교해보세요.
		cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
		cv::Point anchor = cv::Point(-1, -1); // 기본값은 커널 중심입니다.
		int iterations = 1; // 침식 반복 횟수입니다. 1로 시작해서 필요하면 늘려보세요.
        cv::erode(gray, result, kernel, anchor, iterations);

        return result;
    }

    cv::Mat RunDilation(const cv::Mat& gray)
    {
        cv::Mat result = gray.clone();

        // TODO: 팽창 알고리즘을 구현하세요.
        // 힌트: kernel을 만든 뒤 cv::dilate를 호출해보세요.
        // 팽창은 밝은 결함 영역을 두껍게 하거나 끊어진 스크래치 경계를 이어볼 때 유용합니다.

        return result;
    }

    cv::Mat RunHistogramEqualization(const cv::Mat& gray)
    {
        cv::Mat result = gray.clone();

        // TODO: 히스토그램 기반 처리를 구현하세요.
        // 힌트: cv::equalizeHist(gray, result)부터 시작해보세요.
        // 부분 대비를 조절하고 싶다면 cv::createCLAHE를 써서 전체 평활화와 비교해보세요.

        return result;
    }

    cv::Mat RunGaussianBlur(const cv::Mat& gray)
    {
        cv::Mat result = gray.clone();

        // TODO: 가우시안 블러를 구현하세요.
        // 힌트: cv::GaussianBlur(gray, result, cv::Size(odd, odd), sigma)를 호출해보세요.
        // 커널의 가로/세로 크기는 3, 5, 7, 9처럼 홀수여야 합니다.

        return result;
    }

    cv::Mat RunLaplacian(const cv::Mat& gray)
    {
        cv::Mat result = gray.clone();

        // TODO: 라플라시안 엣지 추출을 구현하세요.
        // 힌트: cv::Laplacian 결과를 임시 CV_16S Mat에 받은 뒤,
        // cv::convertScaleAbs로 다시 8비트 이미지로 변환해보세요.
        // 커널 크기 1, 3, 5를 비교해보면 엣지 강도 차이를 보기 좋습니다.

        return result;
    }

    cv::Mat RunThreshold(const cv::Mat& gray)
    {
        cv::Mat result = gray.clone();

        // TODO: 이진화를 구현하세요.
        // 힌트: 고정 임계값, Otsu 임계값, cv::adaptiveThreshold를 비교해보세요.
        // 이진화 결과는 나중에 connectedComponentsWithStats에 넘겨 좌표를 뽑을 수 있습니다.

        return result;
    }

    cv::Mat RunTemplateMatching(const cv::Mat& gray)
    {
        cv::Mat result = gray.clone();

        // TODO: 템플릿 매칭을 구현하세요.
        // 힌트: 비교할 template cv::Mat을 준비한 뒤 cv::matchTemplate을 호출해보세요.
        // 이후 cv::minMaxLoc를 쓰거나 score map을 임계값 처리해서 매칭 위치를 찾을 수 있습니다.
        // 나중에는 C#에서 템플릿 이미지 경로를 넘기거나, C++ 코어 내부에 템플릿을 캐시해볼 수 있습니다.

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

        // TODO: 자동 탐지용 웨이퍼 영역 마스크를 구현하세요.
        // 힌트: 먼저 이진화로 어두운 배경과 웨이퍼를 분리해보세요.
        // 그 다음 morphology close/open으로 작은 구멍이나 고립된 배경 노이즈를 정리해보세요.
        // 예시 흐름:
        //   1. cv::threshold(gray, mask, value, 255, cv::THRESH_BINARY)
        //   2. cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel)
        //   3. cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel)

        return mask;
    }

    cv::Mat BuildDefectMask(const cv::Mat& gray, const cv::Mat& waferMask)
    {
        cv::Mat defectMask = cv::Mat::zeros(gray.size(), CV_8UC1);

        // TODO: 자동 결함 탐지 알고리즘을 구현하세요.
        // 힌트: 여기에서 개별 알고리즘들을 조합할 수 있습니다.
        // 실험해볼 만한 파이프라인:
        //   1. 히스토그램 평활화나 CLAHE로 대비를 개선합니다.
        //   2. 가우시안 블러로 무작위 노이즈를 줄입니다.
        //   3. 큰 블러나 morphology로 배경/패턴을 추정합니다.
        //   4. cv::absdiff로 원본과 배경 추정 이미지를 비교합니다.
        //   5. threshold나 adaptiveThreshold로 차이 이미지를 이진화합니다.
        //   6. 필요하면 Laplacian이나 Canny로 엣지성 결함을 추가합니다.
        //   7. cv::bitwise_and로 waferMask 내부 후보만 남깁니다.
        //   8. 침식/팽창/open/close로 마스크를 정리합니다.
        //
        // CV_8UC1 이진 마스크를 반환하세요:
        //   0   = 정상 영역
        //   255 = 결함 후보
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
