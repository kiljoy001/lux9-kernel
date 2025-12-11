namespace System
{
    using System.Runtime.CompilerServices;

    public interface IDisposable
    {
        void Dispose();
    }

    // Attributes required by compiler
    namespace Runtime.InteropServices
    {
        public sealed class StructLayoutAttribute : Attribute
        {
            public StructLayoutAttribute(LayoutKind layoutKind) {}
            public LayoutKind Value { get; }
            public int Pack;
            public int Size;
            public CharSet CharSet;
        }

        public enum LayoutKind { Sequential = 0, Explicit = 2, Auto = 3 }
        public enum CharSet { None = 1, Ansi = 2, Unicode = 3, Auto = 4 }
    }

    [AttributeUsage(AttributeTargets.Class, Inherited = true)]
    public sealed class AttributeUsageAttribute : Attribute 
    {
        public AttributeUsageAttribute(AttributeTargets validOn) 
        {
            ValidOn = validOn;
        }

        public AttributeTargets ValidOn { get; }
        public bool AllowMultiple { get; set; }
        public bool Inherited { get; set; }
    }

    [Flags]
    public enum AttributeTargets
    {
        Assembly = 1,
        Module = 2,
        Class = 4,
        Struct = 8,
        Enum = 16,
        Constructor = 32,
        Method = 64,
        Property = 128,
        Field = 256,
        Event = 512,
        Interface = 1024,
        Parameter = 2048,
        Delegate = 4096,
        ReturnValue = 8192,
        GenericParameter = 16384,
        All = 32767
    }

    public class Attribute : Object { }

    namespace Runtime.CompilerServices
    {
        public class MethodImplAttribute : Attribute
        {
            public MethodImplAttribute(MethodImplOptions methodImplOptions) {}
        }

        public enum MethodImplOptions
        {
            Unmanaged = 4,
            NoInlining = 8,
            ForwardRef = 16,
            Synchronized = 32,
            NoOptimization = 64,
            PreserveSig = 128,
            AggressiveInlining = 256,
            AggressiveOptimization = 512,
            InternalCall = 4096
        }
    }

    // Fundamental Types

    public class Object
    {
        // Layout must match kernel clr_object_t implicitly
        
        public virtual bool Equals(Object obj)
        {
            return (object)this == (object)obj;
        }

        public static bool Equals(object objA, object objB)
        {
            if (objA == objB) return true;
            if (objA == null || objB == null) return false;
            return objA.Equals(objB);
        }

        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)]
        public virtual extern int GetHashCode();

        public virtual string ToString()
        {
            return GetType().FullName;
        }
        
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)]
        public extern Type GetType();

        public extern void Finalize(); // Protected in real C#, public/extern here for kernel hook
    }

    public abstract class ValueType : Object {}

    public abstract class Enum : ValueType {}

    public struct Void {}

    public struct Boolean
    {
        private bool m_value;
        public override string ToString() => m_value ? "True" : "False";
    }

    public partial struct Char
    {
        private char m_value;
        public override string ToString() => "Char";
    }

    public struct SByte
    {
        private sbyte m_value;
        public override string ToString() => "SByte";
    }

    public struct Byte
    {
        private byte m_value;
        public override string ToString() => "Byte";
    }

    public struct Int16
    {
        private short m_value;
        public override string ToString() => "Int16";
    }

    public struct UInt16
    {
        private ushort m_value;
        public override string ToString() => "UInt16";
    }

    public struct Int32
    {
        private int m_value;
        public override string ToString() => "Int32";
    }

    public struct UInt32
    {
        private uint m_value;
        public override string ToString() => "UInt32";
    }

    public struct Int64
    {
        private long m_value;
        public override string ToString() => "Int64";
    }

    public struct UInt64
    {
        private ulong m_value;
        public override string ToString() => "UInt64";
    }

    public unsafe struct IntPtr
    {
        private void* m_value;
        
        public static readonly IntPtr Zero = default;
        public override string ToString() => ((nint)m_value).ToString();
    }

    public unsafe struct UIntPtr
    {
        private void* m_value;
        
        public static readonly UIntPtr Zero = default;
        public override string ToString() => ((nuint)m_value).ToString();
    }

    public struct Double
    {
        private double m_value;
        public override string ToString() => m_value.ToString();
    }

    public struct Single
    {
        private float m_value;
        public override string ToString() => m_value.ToString();
    }



    // Attribute moved up
    

    
    [AttributeUsage(AttributeTargets.Enum)]
    public class FlagsAttribute : Attribute { }



    public struct RuntimeTypeHandle { }
    public struct RuntimeFieldHandle { } 
}

