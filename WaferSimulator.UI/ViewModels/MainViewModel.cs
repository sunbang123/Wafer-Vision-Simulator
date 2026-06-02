using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Windows.Input;
using WaferSimulator.UI.Models;

namespace WaferSimulator.UI.ViewModels
{
    // 화면에 결함 좌표를 리스트로 보여주기 위한 단순한 데이터 구조
    public class FaultItem
    {
        public int Index { get; set; }
        public int X { get; set; }
        public int Y { get; set; }
    }

    public class MainViewModel : INotifyPropertyChanged
    {
        private string _statusText = "장비 대기 중...";
        private ObservableCollection<FaultItem> _faultList = new ObservableCollection<FaultItem>();

        // UI에 바인딩될 속성 (값이 바뀌면 대시보드 화면이 알아서 갱신됨)
        public string StatusText
        {
            get => _statusText;
            set { _statusText = value; OnPropertyChanged(); }
        }

        public ObservableCollection<FaultItem> FaultList
        {
            get => _faultList;
            set { _faultList = value; OnPropertyChanged(); }
        }

        // 검사 시작 버튼과 연결될 명령(Command)
        public ICommand StartInspectionCommand { get; }

        public MainViewModel()
        {
            StartInspectionCommand = new RelayCommand(ExecuteInspection);
        }

        // 버튼을 누르면 실행되는 핵심 비전 검사 로직
        private void ExecuteInspection()
        {
            StatusText = "웨이퍼 검사 중 (OpenCV 가동)...";
            FaultList.Clear();

            // 1. 가상의 500x500 흑백 이미지 생성 (C# 배열 부하 최소화)
            int width = 500;
            int height = 500;
            byte[] rawImageData = new byte[width * height];

            // 2. 가상의 불량 데이터 심기 (픽셀값 255 = 흰색 결함)
            // 결함 생성을 C#이 주도합니다.
            rawImageData[100 * width + 150] = 255; // (150, 100)
            rawImageData[250 * width + 300] = 255; // (300, 250)
            rawImageData[400 * width + 450] = 255; // (450, 400)

            // 3. C++ DLL에게 채워달라고 보낼 빈 좌표 공책(배열) 준비
            int maxFaults = 100;
            int[] outXArray = new int[maxFaults];
            int[] outYArray = new int[maxFaults];

            // 4. 고정 메모리 핀 고정 (GCHandle) - C++이 연산하는 동안 C# 메모리가 움직이지 않도록 방어
            // 메모리 누수 및 마샬링 예방
            GCHandle handle = GCHandle.Alloc(rawImageData, GCHandleType.Pinned);
            IntPtr imagePtr = handle.AddrOfPinnedObject();

            try
            {
                // 5. C++ OpenCV 함수 호출!
                int detectedCount = VisionBridge.DetectWaferFaults(imagePtr, width, height, outXArray, outYArray, maxFaults);

                // 6. C++이 리스트에 적어준 좌표를 수거해서 UI 리스트에 등록
                for (int i = 0; i < detectedCount; i++)
                {
                    FaultList.Add(new FaultItem { Index = i + 1, X = outXArray[i], Y = outYArray[i] });
                }

                StatusText = $"검사 완료! 총 {detectedCount}개의 결함이 발견되었습니다.";
            }
            catch (Exception ex)
            {
                StatusText = $"비전 코어 호출 오류: {ex.Message}";
            }
            finally
            {
                // 7. 사용한 메모리 핀 해제 (메모리 누수 차단)
                handle.Free();
            }
        }

        // MVVM 패턴의 기본 베이스 구현
        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged([CallerMemberName] string name = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }
    }

    // WPF 버튼 명령 처리를 위한 단순한 헬퍼 클래스
    public class RelayCommand : ICommand
    {
        private readonly Action _execute;
        public RelayCommand(Action execute) => _execute = execute;
        public bool CanExecute(object parameter) => true;
        public void Execute(object parameter) => _execute();
        public event EventHandler CanExecuteChanged { add { } remove { } }
    }
}