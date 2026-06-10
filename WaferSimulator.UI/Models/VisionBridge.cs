using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace WaferSimulator.UI.Models;

public static partial class VisionBridge
{
    private const string DllName = "WaferSimulator.Core.dll";

    [LibraryImport(DllName, EntryPoint = "DetectWaferFaults")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    public static partial int DetectWaferFaults(
        [In] byte[] imageData,
        int width,
        int height,
        [Out] int[] outXArray,
        [Out] int[] outYArray,
        int maxFaults);
}
