namespace System
{
    using System.Runtime.CompilerServices;
    using System.Runtime.InteropServices;

    public interface IDisposable
    {
        void Dispose();
    }

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

    public class Object
    {
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

        public static bool ReferenceEquals(object objA, object objB)
        {
            return objA == objB;
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public virtual extern int GetHashCode();

        public virtual string ToString()
        {
            return GetType().FullName;
        }
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern Type GetType();

        public extern void Finalize();
    }

    public abstract class ValueType : Object {}

    public abstract class Enum : ValueType {}

    public struct Void {}

    [StructLayout(LayoutKind.Sequential, Size = 1)]
    public unsafe struct Boolean
    {
        internal fixed byte m_value[1];
        public override string ToString() => "Boolean";
        
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator ==(bool left, bool right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator !=(bool left, bool right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator !(bool value);
    }

    [StructLayout(LayoutKind.Sequential, Size = 2, Pack = 2)]
    public unsafe partial struct Char
    {
        internal fixed byte m_value[2];
        public override string ToString() => "Char";
        
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator ==(char left, char right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator !=(char left, char right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator <(char left, char right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator >(char left, char right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator <=(char left, char right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator >=(char left, char right);
        
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern implicit operator int(char c);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern explicit operator char(int i);
    }

    [StructLayout(LayoutKind.Sequential, Size = 1)]
    public unsafe struct SByte
    {
        internal fixed byte m_value[1];
        public override string ToString() => "SByte";
        
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern implicit operator int(sbyte b);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern explicit operator sbyte(int i);
    }

    [StructLayout(LayoutKind.Sequential, Size = 1)]
    public unsafe struct Byte
    {
        internal fixed byte m_value[1];
        public override string ToString() => "Byte";
        
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern implicit operator int(byte b);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern explicit operator byte(int i);
    }

    [StructLayout(LayoutKind.Sequential, Size = 2, Pack = 2)]
    public unsafe struct Int16
    {
        internal fixed byte m_value[2];
        public override string ToString() => "Int16";
        
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern implicit operator int(short s);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern explicit operator short(int i);
    }

    [StructLayout(LayoutKind.Sequential, Size = 2, Pack = 2)]
    public unsafe struct UInt16
    {
        internal fixed byte m_value[2];
        public override string ToString() => "UInt16";
        
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern implicit operator int(ushort s);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern explicit operator ushort(int i);
    }

    [StructLayout(LayoutKind.Sequential, Size = 4, Pack = 4)]
    public unsafe struct Int32 : IComparable, IComparable<int>, IEquatable<int>
    {
        internal fixed byte m_value[4];
        
        public const int MinValue = -2147483648;
        public const int MaxValue = 2147483647;

        public int CompareTo(object value)
        {
            if (value == null) return 1;
            if (value is int i)
            {
                int val;
                fixed (byte* p = m_value) val = *(int*)p;
                if (val < i) return -1;
                if (val > i) return 1;
                return 0;
            }
            throw new ArgumentException("Argument must be Int32");
        }

        public int CompareTo(int value)
        {
            int val;
            fixed (byte* p = m_value) val = *(int*)p;
            if (val < value) return -1;
            if (val > value) return 1;
            return 0;
        }

        public override bool Equals(object obj)
        {
            if (obj is int i)
            {
                return Equals(i);
            }
            return false;
        }

        public bool Equals(int obj)
        {
            int val;
            fixed (byte* p = m_value) val = *(int*)p;
            return val == obj;
        }

        public override int GetHashCode()
        {
            int val;
            fixed (byte* p = m_value) val = *(int*)p;
            return val;
        }
        
        public override string ToString() => "Int32";
        
        public static int Parse(string s)
        {
            if (s == null) throw new ArgumentNullException("s");
            int result = 0;
            int sign = 1;
            int i = 0;
            
            // Skip leading whitespace
            while (i < s.Length && s[i] == ' ') i++;
            
            // Handle sign
            if (i < s.Length && s[i] == '-') { sign = -1; i++; }
            else if (i < s.Length && s[i] == '+') { i++; }
            
            // Parse digits
            while (i < s.Length && s[i] >= '0' && s[i] <= '9')
            {
                result = result * 10 + (s[i] - '0');
                i++;
            }
            
            return result * sign;
        }
        
        public static bool TryParse(string s, out int result)
        {
            try { result = Parse(s); return true; }
            catch { result = 0; return false; }
        }
        
        // Arithmetic
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern int operator +(int left, int right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern int operator -(int left, int right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern int operator *(int left, int right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern int operator /(int left, int right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern int operator %(int left, int right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern int operator -(int value);
        
        // Bitwise
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern int operator &(int left, int right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern int operator |(int left, int right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern int operator ^(int left, int right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern int operator ~(int value);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern int operator <<(int value, int shift);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern int operator >>(int value, int shift);
        
        // Comparison
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator ==(int left, int right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator !=(int left, int right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator <(int left, int right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator >(int left, int right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator <=(int left, int right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator >=(int left, int right);
        
        // Conversions
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern implicit operator long(int i);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern implicit operator float(int i);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern implicit operator double(int i);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern explicit operator int(long i);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern explicit operator int(float f);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern explicit operator int(double d);
    }

    [StructLayout(LayoutKind.Sequential, Size = 4, Pack = 4)]
    public unsafe struct UInt32
    {
        internal fixed byte m_value[4];
        public override string ToString() => "UInt32";
        public const uint MaxValue = 4294967295;
        public const uint MinValue = 0;
        
        // Partial set for BitOperations
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator ==(uint left, uint right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator !=(uint left, uint right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern uint operator &(uint left, uint right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern uint operator |(uint left, uint right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern uint operator <<(uint value, int shift);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern uint operator >>(uint value, int shift);
        
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern explicit operator uint(int i);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern explicit operator int(uint u);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern implicit operator ulong(uint u);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern explicit operator uint(ulong u);
    }

    [StructLayout(LayoutKind.Sequential, Size = 8, Pack = 8)]
    public unsafe struct Int64
    {
        internal fixed byte m_value[8];
        
        public const long MinValue = -9223372036854775808L;
        public const long MaxValue = 9223372036854775807L;
        
        public override string ToString() => "Int64";
        
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern long operator -(long value);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator <(long left, long right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator >(long left, long right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator <=(long left, long right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator >=(long left, long right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator ==(long left, long right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator !=(long left, long right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern long operator *(long left, long right);
        
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern long operator &(long left, long right);
        
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern implicit operator float(long i);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern implicit operator double(long i);
    }

    [StructLayout(LayoutKind.Sequential, Size = 8, Pack = 8)]
    public unsafe struct UInt64
    {
        internal fixed byte m_value[8];
        public override string ToString() => "UInt64";
        
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator ==(ulong left, ulong right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator !=(ulong left, ulong right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern ulong operator &(ulong left, ulong right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern ulong operator |(ulong left, ulong right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern ulong operator <<(ulong value, int shift);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern ulong operator >>(ulong value, int shift);
    }

    [StructLayout(LayoutKind.Sequential, Size = 8, Pack = 8)]
    public unsafe struct IntPtr
    {
        private void* m_value;
        public static readonly IntPtr Zero = default;
        public override string ToString() => ((nint)m_value).ToString();
        
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator ==(IntPtr left, IntPtr right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator !=(IntPtr left, IntPtr right);
    }

    [StructLayout(LayoutKind.Sequential, Size = 8, Pack = 8)]
    public unsafe struct UIntPtr
    {
        private void* m_value;
        public static readonly UIntPtr Zero = default;
        public override string ToString() => ((nuint)m_value).ToString();
    }

    [StructLayout(LayoutKind.Sequential, Size = 8, Pack = 8)]
    public unsafe struct Double
    {
        internal fixed byte m_value[8];
        
        public const double NaN = 0.0 / 0.0;
        public const double PositiveInfinity = 1.0 / 0.0;
        public const double NegativeInfinity = -1.0 / 0.0;
        public const double MinValue = -1.7976931348623157E+308;
        public const double MaxValue = 1.7976931348623157E+308;
        public const double Epsilon = 4.9406564584124654E-324;
        
        public static bool IsNaN(double d) => d != d;
        public static bool IsInfinity(double d) => d == PositiveInfinity || d == NegativeInfinity;
        public static bool IsPositiveInfinity(double d) => d == PositiveInfinity;
        public static bool IsNegativeInfinity(double d) => d == NegativeInfinity;
        
        public override string ToString() => "Double";
        
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator <(double left, double right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator >(double left, double right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator <=(double left, double right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator >=(double left, double right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator ==(double left, double right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator !=(double left, double right);
        
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern double operator +(double left, double right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern double operator -(double left, double right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern double operator *(double left, double right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern double operator /(double left, double right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern double operator -(double value);
        
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern explicit operator float(double d);
    }

    [StructLayout(LayoutKind.Sequential, Size = 4, Pack = 4)]
    public unsafe struct Single
    {
        internal fixed byte m_value[4];
        public override string ToString() => "Single";
        
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator <(float left, float right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator >(float left, float right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator <=(float left, float right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator >=(float left, float right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator ==(float left, float right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern bool operator !=(float left, float right);
        
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern float operator +(float left, float right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern float operator -(float left, float right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern float operator *(float left, float right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern float operator /(float left, float right);
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern float operator -(float value);
        
        [MethodImpl(MethodImplOptions.InternalCall)] public static extern implicit operator double(float f);
    }
    
    [AttributeUsage(AttributeTargets.Enum)]
    public class FlagsAttribute : Attribute { }

    public struct RuntimeTypeHandle { }
    public struct RuntimeFieldHandle { } 
        

}
