/*
 * ECMA-335 InteropServices Types - Lux9 BCL
 */
namespace System.Runtime.InteropServices
{
    [AttributeUsage(AttributeTargets.Parameter, Inherited = false)]
    public sealed class InAttribute : Attribute { }

    [AttributeUsage(AttributeTargets.Parameter, Inherited = false)]
    public sealed class OutAttribute : Attribute { }

    [AttributeUsage(AttributeTargets.Parameter, Inherited = false)]
    public sealed class OptionalAttribute : Attribute { }

    [AttributeUsage(AttributeTargets.Method, Inherited = false)]
    public sealed class DllImportAttribute : Attribute
    {
        public DllImportAttribute(String dllName) { Value = dllName; }
        public String Value { get; }
        public String EntryPoint;
        public CharSet CharSet;
        public bool SetLastError;
        public bool ExactSpelling;
        public CallingConvention CallingConvention;
        public bool BestFitMapping;
        public bool ThrowOnUnmappableChar;
        public bool PreserveSig = true;
    }

    public enum CharSet { None, Ansi, Unicode, Auto }
    public enum CallingConvention { Winapi = 1, Cdecl, StdCall, ThisCall, FastCall }

    [AttributeUsage(AttributeTargets.Field, Inherited = false)]
    public sealed class FieldOffsetAttribute : Attribute
    {
        public FieldOffsetAttribute(int offset) { Value = offset; }
        public int Value { get; }
    }

    [AttributeUsage(AttributeTargets.Struct | AttributeTargets.Class, Inherited = false)]
    public sealed class StructLayoutAttribute : Attribute
    {
        public StructLayoutAttribute(LayoutKind layoutKind) { Value = layoutKind; }
        public LayoutKind Value { get; }
        public int Size;
        public int Pack;
        public CharSet CharSet;
    }

    public enum LayoutKind { Sequential = 0, Explicit = 2, Auto = 3 }

    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Parameter | AttributeTargets.ReturnValue, Inherited = false)]
    public sealed class MarshalAsAttribute : Attribute
    {
        public MarshalAsAttribute(UnmanagedType unmanagedType) { Value = unmanagedType; }
        public UnmanagedType Value { get; }
        public UnmanagedType ArraySubType;
        public int SizeConst;
        public short SizeParamIndex;
    }

    public enum UnmanagedType
    {
        Bool = 2, I1, U1, I2, U2, I4, U4, I8, U8, R4, R8,
        Currency = 15, BStr = 19, LPStr, LPWStr,
        LPTStr = 22, ByValTStr = 23, IUnknown = 25, IDispatch,
        Struct = 27, Interface, SafeArray, ByValArray,
        SysInt = 31, SysUInt, VBByRefStr = 34, AnsiBStr,
        TBStr = 36, VariantBool, FunctionPtr = 38,
        AsAny = 40, LPArray = 42, LPStruct,
        CustomMarshaler = 44, Error, IInspectable = 46,
        HString = 47, LPUTF8Str = 48
    }

    [AttributeUsage(AttributeTargets.Assembly | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Enum | AttributeTargets.Interface | AttributeTargets.Delegate, Inherited = false)]
    public sealed class GuidAttribute : Attribute
    {
        public GuidAttribute(String guid) { Value = guid; }
        public String Value { get; }
    }

    [AttributeUsage(AttributeTargets.Interface, Inherited = false)]
    public sealed class InterfaceTypeAttribute : Attribute
    {
        public InterfaceTypeAttribute(ComInterfaceType interfaceType) { Value = interfaceType; }
        public ComInterfaceType Value { get; }
    }

    public enum ComInterfaceType { InterfaceIsDual, InterfaceIsIUnknown, InterfaceIsIDispatch, InterfaceIsIInspectable }

    [AttributeUsage(AttributeTargets.Assembly, Inherited = false)]
    public sealed class ComVisibleAttribute : Attribute
    {
        public ComVisibleAttribute(bool visibility) { Value = visibility; }
        public bool Value { get; }
    }

    [AttributeUsage(AttributeTargets.Class, Inherited = true)]
    public sealed class ClassInterfaceAttribute : Attribute
    {
        public ClassInterfaceAttribute(ClassInterfaceType classInterfaceType) { Value = classInterfaceType; }
        public ClassInterfaceType Value { get; }
    }

    public enum ClassInterfaceType { None = 0, AutoDispatch = 1, AutoDual = 2 }

    public static class Marshal
    {
        public static IntPtr AllocHGlobal(int cb) => IntPtr.Zero;
        public static IntPtr AllocHGlobal(IntPtr cb) => IntPtr.Zero;
        public static void FreeHGlobal(IntPtr hglobal) { }
        public static IntPtr AllocCoTaskMem(int cb) => IntPtr.Zero;
        public static void FreeCoTaskMem(IntPtr ptr) { }
        public static int SizeOf<T>() => 0;
        public static int SizeOf(Object structure) => 0;
        public static int SizeOf(Type t) => 0;
        public static void Copy(byte[] source, int startIndex, IntPtr destination, int length) { }
        public static void Copy(IntPtr source, byte[] destination, int startIndex, int length) { }
        public static String PtrToStringAnsi(IntPtr ptr) => null;
        public static String PtrToStringUni(IntPtr ptr) => null;
        public static String PtrToStringUTF8(IntPtr ptr) => null;
        public static IntPtr StringToHGlobalAnsi(String s) => IntPtr.Zero;
        public static IntPtr StringToHGlobalUni(String s) => IntPtr.Zero;
        public static int GetLastWin32Error() => 0;
    }
}

namespace System.Runtime.Versioning
{
    [AttributeUsage(AttributeTargets.Assembly, Inherited = false, AllowMultiple = false)]
    public sealed class TargetFrameworkAttribute : Attribute
    {
        public TargetFrameworkAttribute(String frameworkName) { FrameworkName = frameworkName; }
        public String FrameworkName { get; }
        public String FrameworkDisplayName { get; set; }
    }

    [AttributeUsage(AttributeTargets.All, Inherited = false, AllowMultiple = true)]
    public sealed class SupportedOSPlatformAttribute : Attribute
    {
        public SupportedOSPlatformAttribute(String platformName) { PlatformName = platformName; }
        public String PlatformName { get; }
    }
}
