#pragma once

extern "C" __declspec(dllexport) int Add(int a, int b);
extern "C" __declspec(dllexport) int GetStatusCode();
extern "C" __declspec(dllexport) int DetectWaferFaults(
    unsigned char* imageData,
    int width,
    int height,
    int* outXArray,
    int* outYArray,
    int maxFaults);
extern "C" __declspec(dllexport) int DetectWaferFaultsAdvanced(
    unsigned char* imageData,
    int width,
    int height,
    int channels,
    int* outXArray,
    int* outYArray,
    int maxFaults,
    unsigned char* processedImageData,
    int processedBufferLength);
extern "C" __declspec(dllexport) int ProcessWaferAlgorithm(
    unsigned char* imageData,
    int width,
    int height,
    int channels,
    int algorithmType,
    int* outXArray,
    int* outYArray,
    int maxFaults,
    unsigned char* processedImageData,
    int processedBufferLength);
