/*
 * ECMA-335 Primitive Types - Lux9 BCL
 * 
 * These are the fundamental types required by the CLI specification.
 * All CLI implementations must provide these types.
 */
namespace System
{
    using System.Runtime.CompilerServices;
    // Object - the root of the type hierarchy (ECMA-335 IV.5.2)
    public class Object
    {
        public Object() { }
        ~Object() { }
        public virtual bool Equals(Object obj) => ReferenceEquals(this, obj);
        public static bool Equals(Object objA, Object objB)
        {
            if (ReferenceEquals(objA, objB)) return true;
            if (objA is null || objB is null) return false;
            return objA.Equals(objB);
        }
        public virtual int GetHashCode() => RuntimeHelpers.GetHashCode(this);
        public Type GetType() => RuntimeHelpers.GetType(this);
        protected Object MemberwiseClone() => RuntimeHelpers.MemberwiseClone(this);
        public static bool ReferenceEquals(Object objA, Object objB) => RuntimeHelpers.ReferenceEquals(objA, objB);
        public virtual string ToString() => GetType().FullName;
    }

    // ValueType - base for all value types (ECMA-335 IV.5.3)
    public abstract class ValueType
    {
        public override bool Equals(Object obj)
        {
            if (obj == null || GetType() != obj.GetType()) return false;
            return RuntimeHelpers.ValueTypeEquals(this, obj);
        }
        public override int GetHashCode() => RuntimeHelpers.GetHashCode(this);
        public override string ToString() => GetType().FullName;
    }

    // Void - represents no return value (ECMA-335 IV.5.4)
    public struct Void { }

    // Boolean (ECMA-335 IV.5.5)
    public struct Boolean : IComparable, IComparable<Boolean>, IEquatable<Boolean>
    {
        internal bool m_value;
        public static readonly string TrueString = "True";
        public static readonly string FalseString = "False";
        public int CompareTo(Object obj) => obj is Boolean b ? CompareTo(b) : throw new ArgumentException();
        public int CompareTo(Boolean value) => m_value == value.m_value ? 0 : (m_value ? 1 : -1);
        public override bool Equals(Object obj) => obj is Boolean b && m_value == b.m_value;
        public bool Equals(Boolean obj) => m_value == obj.m_value;
        public override int GetHashCode() => m_value ? 1 : 0;
        public static Boolean Parse(String value) => value?.Trim().Equals(TrueString, StringComparison.OrdinalIgnoreCase) == true;
        public override string ToString() => m_value ? TrueString : FalseString;
    }

    // Char (ECMA-335 IV.5.6)
    public struct Char : IComparable, IComparable<Char>, IEquatable<Char>
    {
        internal char m_value;
        public const char MaxValue = '\uffff';
        public const char MinValue = '\0';
        public int CompareTo(Object obj) => obj is Char c ? CompareTo(c) : throw new ArgumentException();
        public int CompareTo(Char value) => m_value - value.m_value;
        public override bool Equals(Object obj) => obj is Char c && m_value == c.m_value;
        public bool Equals(Char obj) => m_value == obj.m_value;
        public override int GetHashCode() => m_value;
        public static bool IsDigit(Char c) => c >= '0' && c <= '9';
        public static bool IsLetter(Char c) => (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
        public static bool IsWhiteSpace(Char c) => c == ' ' || c == '\t' || c == '\n' || c == '\r';
        public static bool IsUpper(Char c) => c >= 'A' && c <= 'Z';
        public static bool IsLower(Char c) => c >= 'a' && c <= 'z';
        public static Char ToLower(Char c) => IsUpper(c) ? (Char)(c + 32) : c;
        public static Char ToUpper(Char c) => IsLower(c) ? (Char)(c - 32) : c;
        public override string ToString() => new String(m_value, 1);
    }

    // SByte (ECMA-335 IV.5.7)
    public struct SByte : IComparable, IComparable<SByte>, IEquatable<SByte>
    {
        internal sbyte m_value;
        public const sbyte MaxValue = 127;
        public const sbyte MinValue = -128;
        public int CompareTo(Object obj) => obj is SByte s ? CompareTo(s) : throw new ArgumentException();
        public int CompareTo(SByte value) => m_value - value.m_value;
        public override bool Equals(Object obj) => obj is SByte s && m_value == s.m_value;
        public bool Equals(SByte obj) => m_value == obj.m_value;
        public override int GetHashCode() => m_value;
        public override string ToString() => Number.FormatInt32(m_value, null, null);
    }

    // Byte (ECMA-335 IV.5.8)
    public struct Byte : IComparable, IComparable<Byte>, IEquatable<Byte>
    {
        internal byte m_value;
        public const byte MaxValue = 255;
        public const byte MinValue = 0;
        public int CompareTo(Object obj) => obj is Byte b ? CompareTo(b) : throw new ArgumentException();
        public int CompareTo(Byte value) => m_value - value.m_value;
        public override bool Equals(Object obj) => obj is Byte b && m_value == b.m_value;
        public bool Equals(Byte obj) => m_value == obj.m_value;
        public override int GetHashCode() => m_value;
        public override string ToString() => Number.FormatInt32(m_value, null, null);
    }

    // Int16 (ECMA-335 IV.5.9)
    public struct Int16 : IComparable, IComparable<Int16>, IEquatable<Int16>
    {
        internal short m_value;
        public const short MaxValue = 32767;
        public const short MinValue = -32768;
        public int CompareTo(Object obj) => obj is Int16 i ? CompareTo(i) : throw new ArgumentException();
        public int CompareTo(Int16 value) => m_value - value.m_value;
        public override bool Equals(Object obj) => obj is Int16 i && m_value == i.m_value;
        public bool Equals(Int16 obj) => m_value == obj.m_value;
        public override int GetHashCode() => m_value;
        public override string ToString() => Number.FormatInt32(m_value, null, null);
    }

    // UInt16 (ECMA-335 IV.5.10)
    [CLSCompliant(false)]
    public struct UInt16 : IComparable, IComparable<UInt16>, IEquatable<UInt16>
    {
        internal ushort m_value;
        public const ushort MaxValue = 65535;
        public const ushort MinValue = 0;
        public int CompareTo(Object obj) => obj is UInt16 u ? CompareTo(u) : throw new ArgumentException();
        public int CompareTo(UInt16 value) => m_value - value.m_value;
        public override bool Equals(Object obj) => obj is UInt16 u && m_value == u.m_value;
        public bool Equals(UInt16 obj) => m_value == obj.m_value;
        public override int GetHashCode() => m_value;
        public override string ToString() => Number.FormatUInt32(m_value, null, null);
    }

    // Int32 (ECMA-335 IV.5.11)
    public struct Int32 : IComparable, IComparable<Int32>, IEquatable<Int32>
    {
        internal int m_value;
        public const int MaxValue = 2147483647;
        public const int MinValue = -2147483648;
        public int CompareTo(Object obj) => obj is Int32 i ? CompareTo(i) : throw new ArgumentException();
        public int CompareTo(Int32 value) => m_value < value.m_value ? -1 : (m_value > value.m_value ? 1 : 0);
        public override bool Equals(Object obj) => obj is Int32 i && m_value == i.m_value;
        public bool Equals(Int32 obj) => m_value == obj.m_value;
        public override int GetHashCode() => m_value;
        public static Int32 Parse(String s) => Number.ParseInt32(s);
        public override string ToString() => Number.FormatInt32(m_value, null, null);
    }

    // UInt32 (ECMA-335 IV.5.12)
    [CLSCompliant(false)]
    public struct UInt32 : IComparable, IComparable<UInt32>, IEquatable<UInt32>
    {
        internal uint m_value;
        public const uint MaxValue = 4294967295;
        public const uint MinValue = 0;
        public int CompareTo(Object obj) => obj is UInt32 u ? CompareTo(u) : throw new ArgumentException();
        public int CompareTo(UInt32 value) => m_value < value.m_value ? -1 : (m_value > value.m_value ? 1 : 0);
        public override bool Equals(Object obj) => obj is UInt32 u && m_value == u.m_value;
        public bool Equals(UInt32 obj) => m_value == obj.m_value;
        public override int GetHashCode() => (int)m_value;
        public override string ToString() => Number.FormatUInt32(m_value, null, null);
    }

    // Int64 (ECMA-335 IV.5.13)
    public struct Int64 : IComparable, IComparable<Int64>, IEquatable<Int64>
    {
        internal long m_value;
        public const long MaxValue = 9223372036854775807;
        public const long MinValue = -9223372036854775808;
        public int CompareTo(Object obj) => obj is Int64 i ? CompareTo(i) : throw new ArgumentException();
        public int CompareTo(Int64 value) => m_value < value.m_value ? -1 : (m_value > value.m_value ? 1 : 0);
        public override bool Equals(Object obj) => obj is Int64 i && m_value == i.m_value;
        public bool Equals(Int64 obj) => m_value == obj.m_value;
        public override int GetHashCode() => (int)m_value ^ (int)(m_value >> 32);
        public override string ToString() => Number.FormatInt64(m_value, null, null);
    }

    // UInt64 (ECMA-335 IV.5.14)
    [CLSCompliant(false)]
    public struct UInt64 : IComparable, IComparable<UInt64>, IEquatable<UInt64>
    {
        internal ulong m_value;
        public const ulong MaxValue = 18446744073709551615;
        public const ulong MinValue = 0;
        public int CompareTo(Object obj) => obj is UInt64 u ? CompareTo(u) : throw new ArgumentException();
        public int CompareTo(UInt64 value) => m_value < value.m_value ? -1 : (m_value > value.m_value ? 1 : 0);
        public override bool Equals(Object obj) => obj is UInt64 u && m_value == u.m_value;
        public bool Equals(UInt64 obj) => m_value == obj.m_value;
        public override int GetHashCode() => (int)m_value ^ (int)(m_value >> 32);
        public override string ToString() => Number.FormatUInt64(m_value, null, null);
    }

    // Single (ECMA-335 IV.5.15)
    public struct Single : IComparable, IComparable<Single>, IEquatable<Single>
    {
        internal float m_value;
        public const float MaxValue = 3.40282347E+38f;
        public const float MinValue = -3.40282347E+38f;
        public const float Epsilon = 1.401298E-45f;
        public const float NaN = 0.0f / 0.0f;
        public const float PositiveInfinity = 1.0f / 0.0f;
        public const float NegativeInfinity = -1.0f / 0.0f;
        public int CompareTo(Object obj) => obj is Single s ? CompareTo(s) : throw new ArgumentException();
        public int CompareTo(Single value)
        {
            if (m_value < value.m_value) return -1;
            if (m_value > value.m_value) return 1;
            if (m_value == value.m_value) return 0;
            if (IsNaN(m_value)) return IsNaN(value.m_value) ? 0 : -1;
            return 1;
        }
        public override bool Equals(Object obj) => obj is Single s && Equals(s);
        public bool Equals(Single obj) => m_value == obj.m_value || (IsNaN(m_value) && IsNaN(obj.m_value));
        public override int GetHashCode() => BitConverter.SingleToInt32Bits(m_value);
        public static bool IsNaN(Single f) => f != f;
        public static bool IsInfinity(Single f) => f == PositiveInfinity || f == NegativeInfinity;
        public override string ToString() => Number.FormatSingle(m_value, null, null);
    }

    // Double (ECMA-335 IV.5.16)
    public struct Double : IComparable, IComparable<Double>, IEquatable<Double>
    {
        internal double m_value;
        public const double MaxValue = 1.7976931348623157E+308;
        public const double MinValue = -1.7976931348623157E+308;
        public const double Epsilon = 4.94065645841247E-324;
        public const double NaN = 0.0 / 0.0;
        public const double PositiveInfinity = 1.0 / 0.0;
        public const double NegativeInfinity = -1.0 / 0.0;
        public int CompareTo(Object obj) => obj is Double d ? CompareTo(d) : throw new ArgumentException();
        public int CompareTo(Double value)
        {
            if (m_value < value.m_value) return -1;
            if (m_value > value.m_value) return 1;
            if (m_value == value.m_value) return 0;
            if (IsNaN(m_value)) return IsNaN(value.m_value) ? 0 : -1;
            return 1;
        }
        public override bool Equals(Object obj) => obj is Double d && Equals(d);
        public bool Equals(Double obj) => m_value == obj.m_value || (IsNaN(m_value) && IsNaN(obj.m_value));
        public override int GetHashCode()
        {
            long bits = BitConverter.DoubleToInt64Bits(m_value);
            return (int)bits ^ (int)(bits >> 32);
        }
        public static bool IsNaN(Double d) => d != d;
        public static bool IsInfinity(Double d) => d == PositiveInfinity || d == NegativeInfinity;
        public override string ToString() => Number.FormatDouble(m_value, null, null);
    }

    // IntPtr (ECMA-335 IV.5.19)
    public struct IntPtr : IEquatable<IntPtr>
    {
        private unsafe void* m_value;
        public static readonly IntPtr Zero = new IntPtr(0);
        public unsafe IntPtr(int value) { m_value = (void*)value; }
        public unsafe IntPtr(long value) { m_value = (void*)value; }
        public unsafe IntPtr(void* value) { m_value = value; }
        public override unsafe bool Equals(Object obj) => obj is IntPtr p && m_value == p.m_value;
        public unsafe bool Equals(IntPtr other) => m_value == other.m_value;
        public override unsafe int GetHashCode() => (int)m_value;
        public unsafe int ToInt32() => (int)m_value;
        public unsafe long ToInt64() => (long)m_value;
        public unsafe void* ToPointer() => m_value;
        public override unsafe string ToString() => ((long)m_value).ToString();
        public static unsafe int Size => sizeof(void*);
        public static unsafe bool operator ==(IntPtr a, IntPtr b) => a.m_value == b.m_value;
        public static unsafe bool operator !=(IntPtr a, IntPtr b) => a.m_value != b.m_value;
        public static explicit operator IntPtr(int value) => new IntPtr(value);
        public static explicit operator IntPtr(long value) => new IntPtr(value);
        public static unsafe explicit operator int(IntPtr value) => (int)value.m_value;
        public static unsafe explicit operator long(IntPtr value) => (long)value.m_value;
        public static unsafe explicit operator void*(IntPtr value) => value.m_value;
        public static unsafe explicit operator IntPtr(void* value) => new IntPtr(value);
    }

    // UIntPtr (ECMA-335 IV.5.20)
    [CLSCompliant(false)]
    public struct UIntPtr : IEquatable<UIntPtr>
    {
        private unsafe void* m_value;
        public static readonly UIntPtr Zero = new UIntPtr(0);
        public unsafe UIntPtr(uint value) { m_value = (void*)value; }
        public unsafe UIntPtr(ulong value) { m_value = (void*)value; }
        public unsafe UIntPtr(void* value) { m_value = value; }
        public override unsafe bool Equals(Object obj) => obj is UIntPtr p && m_value == p.m_value;
        public unsafe bool Equals(UIntPtr other) => m_value == other.m_value;
        public override unsafe int GetHashCode() => (int)m_value;
        public unsafe uint ToUInt32() => (uint)m_value;
        public unsafe ulong ToUInt64() => (ulong)m_value;
        public unsafe void* ToPointer() => m_value;
        public override unsafe string ToString() => ((ulong)m_value).ToString();
        public static unsafe int Size => sizeof(void*);
        public static unsafe bool operator ==(UIntPtr a, UIntPtr b) => a.m_value == b.m_value;
        public static unsafe bool operator !=(UIntPtr a, UIntPtr b) => a.m_value != b.m_value;
        public static explicit operator UIntPtr(uint value) => new UIntPtr(value);
        public static explicit operator UIntPtr(ulong value) => new UIntPtr(value);
        public static unsafe explicit operator uint(UIntPtr value) => (uint)value.m_value;
        public static unsafe explicit operator ulong(UIntPtr value) => (ulong)value.m_value;
        public static unsafe explicit operator void*(UIntPtr value) => value.m_value;
        public static unsafe explicit operator UIntPtr(void* value) => new UIntPtr(value);
    }
}
