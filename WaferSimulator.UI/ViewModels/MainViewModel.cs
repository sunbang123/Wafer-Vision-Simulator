using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows.Input;
using WaferSimulator.UI.Models;

namespace WaferSimulator.UI.ViewModels
{
    public class FaultItem
    {
        public int Index { get; set; }
        public int X { get; set; }
        public int Y { get; set; }
    }

    public class MainViewModel : INotifyPropertyChanged
    {
        private string _statusText = "장비 대기 중...";
        private string _resultText = "[검사 결과] 버튼을 누르면 OpenCV 레이블링 연산이 시작됩니다.";
        private ObservableCollection<FaultItem> _faultList = new ObservableCollection<FaultItem>();

        public string StatusText
        {
            get => _statusText;
            set { _statusText = value; OnPropertyChanged(); }
        }

        // XML의 TextBlock과 연결될 ResultText 속성 추가
        public string ResultText
        {
            get => _resultText;
            set { _resultText = value; OnPropertyChanged(); }
        }

        public ObservableCollection<FaultItem> FaultList
        {
            get => _faultList;
            set { _faultList = value; OnPropertyChanged(); }
        }

        public ICommand StartInspectionCommand { get; }

        public MainViewModel()
        {
            StartInspectionCommand = new RelayCommand(ExecuteInspection);
        }

        private void ExecuteInspection()
        {
            StatusText = "웨이퍼 검사 중 (OpenCV 가동)...";
            ResultText = "비전 코어에서 데이터를 분석하는 중입니다...";
            FaultList.Clear();

            int width = 500;
            int height = 500;
            byte[] rawImageData = new byte[width * height];

            // 가상 불량 데이터 심기
            rawImageData[100 * width + 150] = 255;
            rawImageData[250 * width + 300] = 255;
            rawImageData[400 * width + 450] = 255;

            int maxFaults = 100;
            int[] outXArray = new int[maxFaults];
            int[] outYArray = new int[maxFaults];

            try
            {
                // C++ OpenCV 함수 호출
                int detectedCount = VisionBridge.DetectWaferFaults(rawImageData, width, height, outXArray, outYArray, maxFaults);

                //  C++ 리스트를 가공하여 FaultList 컬렉션에 적재
                for (int i = 0; i < detectedCount; i++)
                {
                    FaultList.Add(new FaultItem { Index = i + 1, X = outXArray[i], Y = outYArray[i] });
                }

                ResultText = $"검사 완료! 총 {detectedCount}개의 결함 무게중심이 무사히 수거되었습니다.";
                StatusText = "엔진 상태 코드: SUCCESS (OpenCV 정상 반환)";
            }
            catch (Exception ex)
            {
                ResultText = "비전 코어 호출 중 심각한 에러가 발생했습니다.";
                StatusText = $"에러 메시지: {ex.Message}";
            }
        }

        public event PropertyChangedEventHandler? PropertyChanged;
        protected void OnPropertyChanged([CallerMemberName] string? name = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }
    }

    public class RelayCommand : ICommand
    {
        private readonly Action _execute;
        public RelayCommand(Action execute) => _execute = execute;
        public bool CanExecute(object? parameter) => true;
        public void Execute(object? parameter) => _execute();
        public event EventHandler? CanExecuteChanged { add { } remove { } }
    }
}
