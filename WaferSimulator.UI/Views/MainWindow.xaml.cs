using System;
using System.Runtime.InteropServices;
using System.Windows;
using WaferSimulator.UI.Models; // VisionBridge를 쓰기 위해 필요합니다.

namespace WaferSimulator.UI
{
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
        }

        // "OpenCV 비전 엔진 가동" 버튼을 누르면 작동하는 핵심 이벤트
        private void BtnCalculate_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                TxtStatusResult.Text = "엔진 상태 코드: OpenCV 알고리즘 연산 중...";

                // 1. 가상의 500x500 웨이퍼 이미지 생성 (C# 1차원 byte 배열)
                int width = 500;
                int height = 500;
                byte[] rawImageData = new byte[width * height];

                // 2. 가상의 불량 데이터 심기 (픽셀값 255 = 흰색 결함 점)
                rawImageData[100 * width + 150] = 255; // 결함 1 (150, 100)
                rawImageData[250 * width + 300] = 255; // 결함 2 (300, 250)
                rawImageData[400 * width + 450] = 255; // 결함 3 (450, 400)

                // 3. C++ DLL에게 받아올 빈 좌표 공책(배열) 준비
                int maxFaults = 100;
                int[] outXArray = new int[maxFaults];
                int[] outYArray = new int[maxFaults];

                // 4. 가비지 컬렉터(GC)로부터 메모리를 안전하게 보호하기 위해 고정 (Pinning)
                GCHandle handle = GCHandle.Alloc(rawImageData, GCHandleType.Pinned);
                IntPtr imagePtr = handle.AddrOfPinnedObject();

                try
                {
                    // 5. [핵심] VisionBridge를 통해 C++ OpenCV 함수를 직접 호출!
                    int detectedCount = VisionBridge.DetectWaferFaults(imagePtr, width, height, outXArray, outYArray, maxFaults);

                    // 6. 결과를 UI 텍스트 박스에 실시간 출력
                    if (detectedCount > 0)
                    {
                        string resultText = $"검출 성공! 총 {detectedCount}개 결함 발견\n";
                        for (int i = 0; i < detectedCount; i++)
                        {
                            resultText += $"▶ [결함 {i + 1}] 좌표: ({outXArray[i]}, {outYArray[i]})\n";
                        }
                        TxtSumResult.Text = resultText;
                    }
                    else
                    {
                        TxtSumResult.Text = "연산 결과: 결함이 발견되지 않았습니다.";
                    }

                    TxtStatusResult.Text = "엔진 상태 코드: SUCCESS (OpenCV 정상 반환)";
                }
                finally
                {
                    // 7. 메모리 고정 해제 (메모리 누수 원천 차단)
                    handle.Free();
                }
            }
            catch (Exception ex)
            {
                TxtSumResult.Text = $"오류 발생: {ex.Message}";
                TxtStatusResult.Text = "엔진 상태 코드: ERROR (연동 실패)";
            }
        }
    }
}