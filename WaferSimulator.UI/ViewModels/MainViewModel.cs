using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Threading;
using Microsoft.Win32;
using WaferSimulator.UI.Models;

namespace WaferSimulator.UI.ViewModels
{
    public enum WaferAlgorithmType
    {
        AutoDetect = 0,
        Erosion = 1,
        Dilation = 2,
        Histogram = 3,
        Gaussian = 4,
        Laplacian = 5,
        Threshold = 6,
        TemplateMatching = 7
    }

    public class FaultItem
    {
        public int Index { get; set; }
        public int X { get; set; }
        public int Y { get; set; }
    }

    public class MainViewModel : INotifyPropertyChanged
    {
        private const int DefaultWidth = 500;
        private const int DefaultHeight = 500;
        private readonly DispatcherTimer _monitoringTimer;
        private byte[]? _inputPixels;
        private int _imageWidth;
        private int _imageHeight;
        private int _inspectionCount;
        private WaferAlgorithmType _selectedAlgorithm = WaferAlgorithmType.AutoDetect;
        private string _statusText = "장비 대기 중...";
        private string _resultText = "이미지를 로드하거나 샘플 웨이퍼로 알고리즘을 실행하세요.";
        private string _imageSpecText = "입력 없음";
        private string _selectedAlgorithmText = "선택 알고리즘: 자동 결함 탐지";
        private string _monitoringButtonText = "반복 모니터링 시작";
        private ImageSource? _inputImage;
        private ImageSource? _processedImage;
        private bool _isMonitoring;
        private ObservableCollection<FaultItem> _faultList = new ObservableCollection<FaultItem>();

        public string StatusText
        {
            get => _statusText;
            set { _statusText = value; OnPropertyChanged(); }
        }

        public string ResultText
        {
            get => _resultText;
            set { _resultText = value; OnPropertyChanged(); }
        }

        public string ImageSpecText
        {
            get => _imageSpecText;
            set { _imageSpecText = value; OnPropertyChanged(); }
        }

        public string SelectedAlgorithmText
        {
            get => _selectedAlgorithmText;
            set { _selectedAlgorithmText = value; OnPropertyChanged(); }
        }

        public string MonitoringButtonText
        {
            get => _monitoringButtonText;
            set { _monitoringButtonText = value; OnPropertyChanged(); }
        }

        public ImageSource? InputImage
        {
            get => _inputImage;
            set { _inputImage = value; OnPropertyChanged(); }
        }

        public ImageSource? ProcessedImage
        {
            get => _processedImage;
            set { _processedImage = value; OnPropertyChanged(); }
        }

        public ObservableCollection<FaultItem> FaultList
        {
            get => _faultList;
            set { _faultList = value; OnPropertyChanged(); }
        }

        public ICommand LoadImageCommand { get; }
        public ICommand AlgorithmCommand { get; }
        public ICommand StartInspectionCommand { get; }
        public ICommand ToggleMonitoringCommand { get; }

        public MainViewModel()
        {
            LoadImageCommand = new RelayCommand(_ => LoadImage());
            AlgorithmCommand = new RelayCommand(ExecuteAlgorithmFromParameter);
            StartInspectionCommand = new RelayCommand(_ => ExecuteAlgorithm(WaferAlgorithmType.AutoDetect));
            ToggleMonitoringCommand = new RelayCommand(_ => ToggleMonitoring());

            _monitoringTimer = new DispatcherTimer
            {
                Interval = TimeSpan.FromSeconds(2)
            };
            _monitoringTimer.Tick += (_, _) => ExecuteAlgorithm(_selectedAlgorithm);

            LoadSampleWafer();
        }

        private void LoadImage()
        {
            var dialog = new OpenFileDialog
            {
                Title = "웨이퍼 이미지 선택",
                Filter = "Image Files|*.png;*.jpg;*.jpeg;*.bmp;*.tif;*.tiff|All Files|*.*"
            };

            if (dialog.ShowDialog() != true) {
                return;
            }

            try {
                var bitmap = new BitmapImage();
                bitmap.BeginInit();
                bitmap.CacheOption = BitmapCacheOption.OnLoad;
                bitmap.UriSource = new Uri(dialog.FileName);
                bitmap.EndInit();
                bitmap.Freeze();

                LoadBitmapSource(bitmap, "파일 로드");
                ResultText = "이미지 로드 완료. 원하는 OpenCV 알고리즘 버튼을 눌러보세요.";
                StatusText = dialog.FileName;
            }
            catch (Exception ex) {
                ResultText = "이미지 로드 중 오류가 발생했습니다.";
                StatusText = ex.Message;
            }
        }

        private void LoadSampleWafer()
        {
            _imageWidth = DefaultWidth;
            _imageHeight = DefaultHeight;
            _inputPixels = new byte[_imageWidth * _imageHeight * 4];

            int centerX = _imageWidth / 2;
            int centerY = _imageHeight / 2;
            int radius = 220;

            for (int y = 0; y < _imageHeight; y++) {
                for (int x = 0; x < _imageWidth; x++) {
                    int offset = (y * _imageWidth + x) * 4;
                    int dx = x - centerX;
                    int dy = y - centerY;
                    bool insideWafer = dx * dx + dy * dy <= radius * radius;
                    byte baseValue = insideWafer ? (byte)(120 + ((x / 16 + y / 20) % 2) * 38) : (byte)0;

                    _inputPixels[offset] = baseValue;
                    _inputPixels[offset + 1] = insideWafer ? (byte)(baseValue + 20) : (byte)0;
                    _inputPixels[offset + 2] = insideWafer ? (byte)(baseValue + 45) : (byte)0;
                    _inputPixels[offset + 3] = 255;
                }
            }

            PaintDefect(_inputPixels, _imageWidth, 150, 100, 5);
            PaintDefect(_inputPixels, _imageWidth, 300, 250, 7);
            PaintDefect(_inputPixels, _imageWidth, 420, 360, 4);

            InputImage = CreateBitmap(_inputPixels, _imageWidth, _imageHeight);
            ProcessedImage = null;
            ImageSpecText = "샘플 웨이퍼: 500 x 500, BGRA";
        }

        private static void PaintDefect(byte[] pixels, int stridePixels, int centerX, int centerY, int radius)
        {
            for (int y = centerY - radius; y <= centerY + radius; y++) {
                for (int x = centerX - radius; x <= centerX + radius; x++) {
                    int dx = x - centerX;
                    int dy = y - centerY;
                    if (dx * dx + dy * dy > radius * radius) {
                        continue;
                    }

                    int offset = (y * stridePixels + x) * 4;
                    pixels[offset] = 20;
                    pixels[offset + 1] = 20;
                    pixels[offset + 2] = 255;
                    pixels[offset + 3] = 255;
                }
            }
        }

        private void LoadBitmapSource(BitmapSource source, string label)
        {
            var formatted = new FormatConvertedBitmap(source, PixelFormats.Bgra32, null, 0);
            formatted.Freeze();

            _imageWidth = formatted.PixelWidth;
            _imageHeight = formatted.PixelHeight;
            int stride = _imageWidth * 4;
            _inputPixels = new byte[stride * _imageHeight];
            formatted.CopyPixels(_inputPixels, stride, 0);

            InputImage = CreateBitmap(_inputPixels, _imageWidth, _imageHeight);
            ProcessedImage = null;
            ImageSpecText = $"{label}: {_imageWidth} x {_imageHeight}, BGRA";
        }

        private void ExecuteAlgorithmFromParameter(object? parameter)
        {
            if (parameter is string algorithmName && Enum.TryParse(algorithmName, out WaferAlgorithmType algorithm)) {
                ExecuteAlgorithm(algorithm);
            }
        }

        private void ExecuteAlgorithm(WaferAlgorithmType algorithm)
        {
            if (_inputPixels == null || _imageWidth <= 0 || _imageHeight <= 0) {
                LoadSampleWafer();
            }

            byte[]? inputPixels = _inputPixels;
            if (inputPixels == null) {
                ResultText = "입력 이미지가 준비되지 않았습니다.";
                StatusText = "엔진 상태 코드: NO_INPUT";
                return;
            }

            _selectedAlgorithm = algorithm;
            SelectedAlgorithmText = $"선택 알고리즘: {GetAlgorithmDisplayName(algorithm)}";
            StatusText = $"{GetAlgorithmDisplayName(algorithm)} 처리 중...";
            ResultText = "C++ OpenCV 코어에서 선택한 알고리즘을 실행하는 중입니다.";
            FaultList.Clear();

            int maxFaults = 200;
            int[] outXArray = new int[maxFaults];
            int[] outYArray = new int[maxFaults];
            byte[] processedPixels = new byte[_imageWidth * _imageHeight * 4];

            try {
                int detectedCount = VisionBridge.ProcessWaferAlgorithm(
                    inputPixels,
                    _imageWidth,
                    _imageHeight,
                    4,
                    (int)algorithm,
                    outXArray,
                    outYArray,
                    maxFaults,
                    processedPixels,
                    processedPixels.Length);

                if (detectedCount < 0) {
                    ResultText = "비전 코어가 입력 이미지를 처리하지 못했습니다.";
                    StatusText = "엔진 상태 코드: INVALID_INPUT";
                    return;
                }

                for (int i = 0; i < detectedCount; i++) {
                    FaultList.Add(new FaultItem { Index = i + 1, X = outXArray[i], Y = outYArray[i] });
                }

                ProcessedImage = CreateBitmap(processedPixels, _imageWidth, _imageHeight);
                _inspectionCount++;

                ResultText = detectedCount > 0
                    ? $"{GetAlgorithmDisplayName(algorithm)} 완료. 후보 좌표 {detectedCount}개를 표시했습니다."
                    : $"{GetAlgorithmDisplayName(algorithm)} 완료. 이 모드는 처리 이미지 확인 중심입니다.";

                StatusText = _isMonitoring
                    ? $"반복 모니터링 실행 중... {_inspectionCount}회 처리 완료"
                    : "엔진 상태 코드: SUCCESS";
            }
            catch (Exception ex) {
                ResultText = "비전 코어 호출 중 오류가 발생했습니다.";
                StatusText = ex.Message;
            }
        }

        private void ToggleMonitoring()
        {
            if (_isMonitoring) {
                _monitoringTimer.Stop();
                _isMonitoring = false;
                MonitoringButtonText = "반복 모니터링 시작";
                StatusText = "반복 모니터링 중지";
                return;
            }

            _inspectionCount = 0;
            _isMonitoring = true;
            MonitoringButtonText = "반복 모니터링 중지";
            ExecuteAlgorithm(_selectedAlgorithm);
            _monitoringTimer.Start();
        }

        private static string GetAlgorithmDisplayName(WaferAlgorithmType algorithm)
        {
            return algorithm switch
            {
                WaferAlgorithmType.AutoDetect => "자동 결함 탐지",
                WaferAlgorithmType.Erosion => "침식",
                WaferAlgorithmType.Dilation => "팽창",
                WaferAlgorithmType.Histogram => "히스토그램 평활화",
                WaferAlgorithmType.Gaussian => "가우시안 블러",
                WaferAlgorithmType.Laplacian => "라플라시안",
                WaferAlgorithmType.Threshold => "이진화",
                WaferAlgorithmType.TemplateMatching => "템플릿 매칭",
                _ => "알 수 없음"
            };
        }

        private static BitmapSource CreateBitmap(byte[] pixels, int width, int height)
        {
            var bitmap = BitmapSource.Create(
                width,
                height,
                96,
                96,
                PixelFormats.Bgra32,
                null,
                pixels,
                width * 4);

            bitmap.Freeze();
            return bitmap;
        }

        public event PropertyChangedEventHandler? PropertyChanged;

        protected void OnPropertyChanged([CallerMemberName] string? name = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }
    }

    public class RelayCommand : ICommand
    {
        private readonly Action<object?> _execute;

        public RelayCommand(Action<object?> execute) => _execute = execute;

        public bool CanExecute(object? parameter) => true;

        public void Execute(object? parameter) => _execute(parameter);

        public event EventHandler? CanExecuteChanged { add { } remove { } }
    }
}
