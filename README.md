# 웨이퍼 결함 탐지 및 모니터링 시뮬레이터

가상의 반도체 웨이퍼 이미지를 생성하고, C++ OpenCV 비전 코어로 결함 좌표를 검출한 뒤 C# WPF 대시보드에 표시하는 시뮬레이터입니다.

## 구성

- `WaferVisionSimulator`: 1단계 검증용 C++ 콘솔 프로그램
- `WaferSimulator.Core`: C#에서 호출하는 C++ OpenCV DLL
- `WaferSimulator.UI`: WPF MVVM 대시보드

## 핵심 흐름

1. WPF ViewModel이 500 x 500 Gray8 가상 웨이퍼 데이터를 생성합니다.
2. `VisionBridge`가 `WaferSimulator.Core.dll`의 `DetectWaferFaults`를 P/Invoke로 호출합니다.
3. C++ 코어가 Gaussian blur, threshold, connected component labeling으로 결함 중심 좌표를 계산합니다.
4. C++ 코어가 결함 중심점 맵의 integral image를 만들어 ROI 내부 결함 수를 빠르게 카운트합니다.
5. WPF 화면이 웨이퍼 맵, ROI, 결함 마커, 좌표 리스트를 바인딩으로 갱신합니다.

## 빌드

Visual Studio 2022와 OpenCV가 아래 경로에 설치되어 있다고 가정합니다.

```powershell
C:\OpenCV\build\install
```

솔루션 빌드:

```powershell
msbuild WaferVisionSimulator\WaferVisionSimulator.sln /p:Configuration=Debug /p:Platform=x64
```

콘솔 검증 실행:

```powershell
WaferVisionSimulator\x64\Debug\WaferVisionSimulator.exe
```

WPF 실행:

```powershell
WaferSimulator.UI\bin\Debug\net8.0-windows\WaferSimulator.UI.exe
```

## 현재 구현 상태

- C++ 콘솔에서 단순 웨이퍼 이미지의 결함 개수와 좌표 출력
- C++ DLL에서 결함 좌표 배열과 ROI 결함 수 반환
- C# P/Invoke marshaling 및 pinned byte buffer 사용
- WPF MVVM 기반 대시보드 UI, 웨이퍼 이미지, ROI, 결함 마커, 좌표 리스트 표시
