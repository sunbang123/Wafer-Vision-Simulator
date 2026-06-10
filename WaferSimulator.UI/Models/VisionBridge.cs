using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace WaferSimulator.UI.Models
{
    public static partial class VisionBridge
    {
        private const string DllName = "WaferSimulator.Core.dll";

        [LibraryImport(DllName, EntryPoint = "DetectWaferFaults")]
        [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
        public static partial int DetectWaferFaults(
            byte[] imageData,
            int width,
            int height,
            int[] outXArray,
            int[] outYArray,
            int maxFaults
        );

        [LibraryImport(DllName, EntryPoint = "DetectWaferFaultsAdvanced")]
        [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
        public static partial int DetectWaferFaultsAdvanced(
            byte[] imageData,
            int width,
            int height,
            int channels,
            int[] outXArray,
            int[] outYArray,
            int maxFaults,
            byte[]? processedImageData,
            int processedBufferLength
        );

        [LibraryImport(DllName, EntryPoint = "ProcessWaferAlgorithm")]
        [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
        public static partial int ProcessWaferAlgorithm(
            byte[] imageData,
            int width,
            int height,
            int channels,
            int algorithmType,
            int[] outXArray,
            int[] outYArray,
            int maxFaults,
            byte[] processedImageData,
            int processedBufferLength
        );
    }
}
