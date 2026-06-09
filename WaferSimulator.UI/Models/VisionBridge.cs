using System;
using System.Runtime.InteropServices;

namespace WaferSimulator.UI.Models
{
    public class VisionBridge
    {
        private const string DllName = "WaferSimulator.Core.dll";

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int DetectWaferFaults(
            IntPtr imageData,
            int width,
            int height,
            int[] outXArray,
            int[] outYArray,
            int maxFaults
        );

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int DetectWaferFaultsAdvanced(
            IntPtr imageData,
            int width,
            int height,
            int channels,
            int[] outXArray,
            int[] outYArray,
            int maxFaults,
            IntPtr processedImageData,
            int processedBufferLength
        );

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ProcessWaferAlgorithm(
            IntPtr imageData,
            int width,
            int height,
            int channels,
            int algorithmType,
            int[] outXArray,
            int[] outYArray,
            int maxFaults,
            IntPtr processedImageData,
            int processedBufferLength
        );
    }
}
