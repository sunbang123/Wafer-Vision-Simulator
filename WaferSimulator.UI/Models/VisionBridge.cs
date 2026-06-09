using System;
using System.Runtime.InteropServices;

namespace WaferSimulator.UI.Models
{
    public static class VisionBridge
    {
        private const string DllName = "WaferSimulator.Core.dll";

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int DetectWaferFaults(
            IntPtr imageData,
            int width,
            int height,
            [Out] int[] outXArray,
            [Out] int[] outYArray,
            int maxFaults,
            int roiX,
            int roiY,
            int roiWidth,
            int roiHeight,
            out int outRoiFaultCount);
    }
}
