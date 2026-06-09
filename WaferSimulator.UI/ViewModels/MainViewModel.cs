using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using WaferSimulator.UI.Models;

namespace WaferSimulator.UI.ViewModels
{
    public sealed class FaultItem
    {
        public int Index { get; init; }
        public int X { get; init; }
        public int Y { get; init; }
        public double MarkerLeft => X - 5;
        public double MarkerTop => Y - 5;
    }

    public sealed class MainViewModel : INotifyPropertyChanged
    {
        private const int WaferWidth = 500;
        private const int WaferHeight = 500;
        private const int RoiXValue = 240;
        private const int RoiYValue = 190;
        private const int RoiWidthValue = 210;
        private const int RoiHeightValue = 210;

        private string _statusText = "Standby";
        private string _resultText = "OpenCV vision core is ready.";
        private string _roiText = "ROI defects: -";
        private ImageSource? _waferImage;

        public MainViewModel()
        {
            StartInspectionCommand = new RelayCommand(ExecuteInspection);
            LoadSyntheticWafer();
        }

        public event PropertyChangedEventHandler? PropertyChanged;

        public ICommand StartInspectionCommand { get; }
        public ObservableCollection<FaultItem> FaultList { get; } = new();

        public int CanvasWidth => WaferWidth;
        public int CanvasHeight => WaferHeight;
        public int RoiX => RoiXValue;
        public int RoiY => RoiYValue;
        public int RoiWidth => RoiWidthValue;
        public int RoiHeight => RoiHeightValue;

        public string StatusText
        {
            get => _statusText;
            private set { _statusText = value; OnPropertyChanged(); }
        }

        public string ResultText
        {
            get => _resultText;
            private set { _resultText = value; OnPropertyChanged(); }
        }

        public string RoiText
        {
            get => _roiText;
            private set { _roiText = value; OnPropertyChanged(); }
        }

        public ImageSource? WaferImage
        {
            get => _waferImage;
            private set { _waferImage = value; OnPropertyChanged(); }
        }

        private void ExecuteInspection()
        {
            StatusText = "Inspecting";
            ResultText = "Running blur, threshold, connected component labeling.";
            FaultList.Clear();

            byte[] rawImageData = CreateSyntheticWaferImage();
            int[] outXArray = new int[100];
            int[] outYArray = new int[100];

            GCHandle handle = GCHandle.Alloc(rawImageData, GCHandleType.Pinned);
            try
            {
                int detectedCount = VisionBridge.DetectWaferFaults(
                    handle.AddrOfPinnedObject(),
                    WaferWidth,
                    WaferHeight,
                    outXArray,
                    outYArray,
                    outXArray.Length,
                    RoiXValue,
                    RoiYValue,
                    RoiWidthValue,
                    RoiHeightValue,
                    out int roiFaultCount);

                for (int i = 0; i < detectedCount; i++)
                {
                    FaultList.Add(new FaultItem { Index = i + 1, X = outXArray[i], Y = outYArray[i] });
                }

                WaferImage = CreateBitmap(rawImageData);
                ResultText = $"Inspection complete. Total defects: {detectedCount}.";
                RoiText = $"ROI defects: {roiFaultCount}";
                StatusText = $"Core status: SUCCESS / {DateTime.Now:HH:mm:ss}";
            }
            catch (DllNotFoundException ex)
            {
                ResultText = "WaferSimulator.Core.dll was not found in the UI output folder.";
                StatusText = ex.Message;
            }
            catch (Exception ex)
            {
                ResultText = "Vision core call failed.";
                StatusText = ex.Message;
            }
            finally
            {
                handle.Free();
            }
        }

        private void LoadSyntheticWafer()
        {
            WaferImage = CreateBitmap(CreateSyntheticWaferImage());
            RoiText = $"ROI: ({RoiXValue}, {RoiYValue}) {RoiWidthValue}x{RoiHeightValue}";
        }

        private static byte[] CreateSyntheticWaferImage()
        {
            byte[] pixels = new byte[WaferWidth * WaferHeight];
            PlotDisk(pixels, 150, 100, 2);
            PlotDisk(pixels, 300, 250, 3);
            PlotScratch(pixels, 420, 380, 460, 410);
            return pixels;
        }

        private static void PlotDisk(byte[] pixels, int centerX, int centerY, int radius)
        {
            for (int y = centerY - radius; y <= centerY + radius; y++)
            {
                for (int x = centerX - radius; x <= centerX + radius; x++)
                {
                    int dx = x - centerX;
                    int dy = y - centerY;
                    if (dx * dx + dy * dy <= radius * radius)
                    {
                        SetPixel(pixels, x, y);
                    }
                }
            }
        }

        private static void PlotScratch(byte[] pixels, int x1, int y1, int x2, int y2)
        {
            const int steps = 48;
            for (int i = 0; i <= steps; i++)
            {
                double t = i / (double)steps;
                int x = (int)Math.Round(x1 + (x2 - x1) * t);
                int y = (int)Math.Round(y1 + (y2 - y1) * t);
                PlotDisk(pixels, x, y, 1);
            }
        }

        private static void SetPixel(byte[] pixels, int x, int y)
        {
            if (0 <= x && x < WaferWidth && 0 <= y && y < WaferHeight)
            {
                pixels[y * WaferWidth + x] = 255;
            }
        }

        private static BitmapSource CreateBitmap(byte[] pixels)
        {
            var bitmap = BitmapSource.Create(
                WaferWidth,
                WaferHeight,
                96,
                96,
                PixelFormats.Gray8,
                null,
                pixels,
                WaferWidth);
            bitmap.Freeze();
            return bitmap;
        }

        private void OnPropertyChanged([CallerMemberName] string? name = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }
    }

    public sealed class RelayCommand : ICommand
    {
        private readonly Action _execute;

        public RelayCommand(Action execute)
        {
            _execute = execute;
        }

        public bool CanExecute(object? parameter) => true;

        public void Execute(object? parameter)
        {
            _execute();
        }

        public event EventHandler? CanExecuteChanged
        {
            add { }
            remove { }
        }
    }
}
