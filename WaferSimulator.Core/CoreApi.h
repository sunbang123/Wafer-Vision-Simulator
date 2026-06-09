#pragma once

extern "C" __declspec(dllexport) int Add(int a, int b);
extern "C" __declspec(dllexport) int GetStatusCode();
extern "C" __declspec(dllexport) int DetectWaferFaults(
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
    int* outRoiFaultCount);
