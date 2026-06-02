#include <opencv2/opencv.hpp>
#include <vector>

// C#에서 호출할 수 있도록 C 스타일의 인터페이스로 외부에 노출(Export)합니다.
extern "C" __declspec(dllexport) int DetectWaferFaults(
    unsigned char* imageData, 
    int width, 
    int height, 
    int* outXArray, 
    int* outYArray, 
    int maxFaults) 
{
    // 1. C#에서 넘겨받은 원시 이미지 포인터(Raw Pointer)를 OpenCV Mat 객체로 래핑
    cv::Mat wafer(height, width, CV_8UC1, imageData);

    // 2. 가상의 패턴 노이즈가 있다고 가정하고 블러링으로 제거 (기획서 반영)
    cv::Mat blurred;
    cv::GaussianBlur(wafer, blurred, cv::Size(3, 3), 0);

    // 4. 이진화 및 레이블링 알고리즘으로 결함 구역 추출
    cv::Mat labels, stats, centroids;
    int numLabels = cv::connectedComponentsWithStats(blurred, labels, stats, centroids);

    int faultCount = 0;

    // 5. 검출된 결함들의 중심 좌표를 C#이 준비한 배열에 담아주기 (배경인 0번 제외)
    for (int i = 1; i < numLabels && faultCount < maxFaults; i++) {
        double centerX = centroids.at<double>(i, 0);
        double centerY = centroids.at<double>(i, 1);

        // C# 배열 메모리에 직접 좌표 기입 (마샬링 통로)
        outXArray[faultCount] = static_cast<int>(centerX);
        outYArray[faultCount] = static_cast<int>(centerY);
        
        faultCount++;
    }

    // 최종 찾아낸 결함 개수를 C#에게 반환
    return faultCount;
}