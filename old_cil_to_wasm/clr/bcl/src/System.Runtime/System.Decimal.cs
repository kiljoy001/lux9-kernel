namespace System
{
    /// <summary>
    /// Decimal type with 128-bit representation for precise financial calculations.
    /// Layout: 96-bit integer (lo, mid, hi) + 8-bit scale (0-28) + 1-bit sign
    /// </summary>
    public struct Decimal : IComparable, IComparable<Decimal>, IEquatable<Decimal>
    {
        // Internal representation: 96-bit integer with scale and sign
        private int flags;  // bits 0-15: reserved, 16-23: scale (0-28), 24-30: reserved, 31: sign
        private uint lo;    // Low 32 bits
        private uint mid;   // Middle 32 bits  
        private uint hi;    // High 32 bits
        
        private const int ScaleMask = 0x00FF0000;
        private const int ScaleShift = 16;
        private const int SignMask = unchecked((int)0x80000000);
        
        public static readonly Decimal Zero = new Decimal(0);
        public static readonly Decimal One = new Decimal(1);
        public static readonly Decimal MinusOne = new Decimal(-1);
        public static readonly Decimal MaxValue = new Decimal(unchecked((int)0xFFFFFFFF), unchecked((int)0xFFFFFFFF), unchecked((int)0xFFFFFFFF), false, 0);
        public static readonly Decimal MinValue = new Decimal(unchecked((int)0xFFFFFFFF), unchecked((int)0xFFFFFFFF), unchecked((int)0xFFFFFFFF), true, 0);
        
        // Constructors
        public Decimal(int value)
        {
            if (value >= 0)
            {
                flags = 0;
                lo = (uint)value;
            }
            else
            {
                flags = SignMask;
                lo = (uint)(-value);
            }
            mid = 0;
            hi = 0;
        }
        
        public Decimal(uint value)
        {
            flags = 0;
            lo = value;
            mid = 0;
            hi = 0;
        }
        
        public Decimal(long value)
        {
            if (value >= 0)
            {
                flags = 0;
                lo = (uint)value;
                mid = (uint)(value >> 32);
            }
            else
            {
                flags = SignMask;
                ulong abs = (ulong)(-value);
                lo = (uint)abs;
                mid = (uint)(abs >> 32);
            }
            hi = 0;
        }
        
        public Decimal(ulong value)
        {
            flags = 0;
            lo = (uint)value;
            mid = (uint)(value >> 32);
            hi = 0;
        }
        
        public Decimal(double value)
        {
            // Simplified conversion
            bool neg = value < 0;
            if (neg) value = -value;
            
            int scale = 0;
            while (value != Math.Floor(value) && scale < 28)
            {
                value *= 10;
                scale++;
            }
            
            ulong mantissa = (ulong)value;
            flags = (neg ? SignMask : 0) | (scale << ScaleShift);
            lo = (uint)mantissa;
            mid = (uint)(mantissa >> 32);
            hi = 0;
        }
        
        public Decimal(int lo, int mid, int hi, bool isNegative, byte scale)
        {
            if (scale > 28)
                throw new ArgumentOutOfRangeException("scale");
            this.lo = (uint)lo;
            this.mid = (uint)mid;
            this.hi = (uint)hi;
            flags = ((int)scale << ScaleShift) | (isNegative ? SignMask : 0);
        }
        
        // Properties
        public bool IsNegative => (flags & SignMask) != 0;
        public byte Scale => (byte)((flags & ScaleMask) >> ScaleShift);
        
        // Get bits
        public static int[] GetBits(Decimal d)
        {
            return new int[] { (int)d.lo, (int)d.mid, (int)d.hi, d.flags };
        }
        
        // Arithmetic operations
        public static Decimal operator +(Decimal d1, Decimal d2)
        {
            // Normalize scales
            int scale1 = d1.Scale;
            int scale2 = d2.Scale;
            
            // Simplified: convert to double for calculation
            double v1 = ToDouble(d1);
            double v2 = ToDouble(d2);
            return new Decimal(v1 + v2);
        }
        
        public static Decimal operator -(Decimal d1, Decimal d2)
        {
            double v1 = ToDouble(d1);
            double v2 = ToDouble(d2);
            return new Decimal(v1 - v2);
        }
        
        public static Decimal operator *(Decimal d1, Decimal d2)
        {
            double v1 = ToDouble(d1);
            double v2 = ToDouble(d2);
            return new Decimal(v1 * v2);
        }
        
        public static Decimal operator /(Decimal d1, Decimal d2)
        {
            if (d2 == Zero)
                throw new DivideByZeroException();
            double v1 = ToDouble(d1);
            double v2 = ToDouble(d2);
            return new Decimal(v1 / v2);
        }
        
        public static Decimal operator %(Decimal d1, Decimal d2)
        {
            if (d2 == Zero)
                throw new DivideByZeroException();
            double v1 = ToDouble(d1);
            double v2 = ToDouble(d2);
            return new Decimal(v1 % v2);
        }
        
        public static Decimal operator -(Decimal d)
        {
            Decimal result = d;
            result.flags ^= SignMask;
            return result;
        }
        
        public static Decimal operator ++(Decimal d) => d + One;
        public static Decimal operator --(Decimal d) => d - One;
        
        // Comparison
        public static bool operator ==(Decimal d1, Decimal d2)
        {
            return d1.lo == d2.lo && d1.mid == d2.mid && d1.hi == d2.hi && d1.flags == d2.flags;
        }
        
        public static bool operator !=(Decimal d1, Decimal d2) => !(d1 == d2);
        
        public static bool operator <(Decimal d1, Decimal d2)
        {
            return ToDouble(d1) < ToDouble(d2);
        }
        
        public static bool operator >(Decimal d1, Decimal d2) => d2 < d1;
        public static bool operator <=(Decimal d1, Decimal d2) => !(d2 < d1);
        public static bool operator >=(Decimal d1, Decimal d2) => !(d1 < d2);
        
        // Implicit conversions from integral types
        public static implicit operator Decimal(sbyte value) => new Decimal(value);
        public static implicit operator Decimal(byte value) => new Decimal((uint)value);
        public static implicit operator Decimal(short value) => new Decimal(value);
        public static implicit operator Decimal(ushort value) => new Decimal((uint)value);
        public static implicit operator Decimal(int value) => new Decimal(value);
        public static implicit operator Decimal(uint value) => new Decimal(value);
        public static implicit operator Decimal(long value) => new Decimal(value);
        public static implicit operator Decimal(ulong value) => new Decimal(value);
        public static implicit operator Decimal(char value) => new Decimal((uint)value);
        
        // Explicit conversions to integral types
        public static explicit operator sbyte(Decimal d) => (sbyte)ToDouble(d);
        public static explicit operator byte(Decimal d) => (byte)ToDouble(d);
        public static explicit operator short(Decimal d) => (short)ToDouble(d);
        public static explicit operator ushort(Decimal d) => (ushort)ToDouble(d);
        public static explicit operator int(Decimal d) => (int)ToDouble(d);
        public static explicit operator uint(Decimal d) => (uint)ToDouble(d);
        public static explicit operator long(Decimal d) => (long)ToDouble(d);
        public static explicit operator ulong(Decimal d) => (ulong)ToDouble(d);
        public static explicit operator float(Decimal d) => (float)ToDouble(d);
        public static explicit operator double(Decimal d) => ToDouble(d);
        
        // Conversion to double
        public static double ToDouble(Decimal d)
        {
            // Convert 96-bit integer to double
            double value = (double)d.hi * 4294967296.0 * 4294967296.0 
                         + (double)d.mid * 4294967296.0 
                         + (double)d.lo;
            
            // Apply scale
            int scale = d.Scale;
            while (scale > 0)
            {
                value /= 10;
                scale--;
            }
            
            // Apply sign
            if (d.IsNegative) value = -value;
            return value;
        }
        
        // Math operations
        public static Decimal Abs(Decimal d)
        {
            Decimal result = d;
            result.flags &= ~SignMask;
            return result;
        }
        
        public static Decimal Ceiling(Decimal d)
        {
            return new Decimal(Math.Ceiling(ToDouble(d)));
        }
        
        public static Decimal Floor(Decimal d)
        {
            return new Decimal(Math.Floor(ToDouble(d)));
        }
        
        public static Decimal Round(Decimal d)
        {
            return new Decimal(Math.Round(ToDouble(d)));
        }
        
        public static Decimal Round(Decimal d, int decimals)
        {
            double mult = 1;
            for (int i = 0; i < decimals; i++) mult *= 10;
            return new Decimal(Math.Round(ToDouble(d) * mult) / mult);
        }
        
        public static Decimal Truncate(Decimal d)
        {
            return new Decimal(Math.Truncate(ToDouble(d)));
        }
        
        // Parse
        public static Decimal Parse(string s)
        {
            if (string.IsNullOrEmpty(s))
                throw new FormatException("Invalid Decimal string");
            
            bool neg = false;
            int idx = 0;
            
            if (s[0] == '-')
            {
                neg = true;
                idx = 1;
            }
            else if (s[0] == '+')
            {
                idx = 1;
            }
            
            double value = 0;
            bool hasDot = false;
            double divisor = 1;
            
            for (; idx < s.Length; idx++)
            {
                char c = s[idx];
                if (c == '.')
                {
                    if (hasDot) throw new FormatException();
                    hasDot = true;
                }
                else if (c >= '0' && c <= '9')
                {
                    value = value * 10 + (c - '0');
                    if (hasDot) divisor *= 10;
                }
                else
                {
                    throw new FormatException();
                }
            }
            
            value /= divisor;
            if (neg) value = -value;
            
            return new Decimal(value);
        }
        
        public static bool TryParse(string s, out Decimal result)
        {
            try
            {
                result = Parse(s);
                return true;
            }
            catch
            {
                result = Zero;
                return false;
            }
        }
        
        // ToString
        public override string ToString()
        {
            double val = ToDouble(this);
            // Format with fixed decimal places based on scale
            int scale = Scale;
            if (scale == 0)
                return ((long)val).ToString();
            
            // Simple formatting: just return the value as string
            // Full formatting support requires double.ToString(format) which isn't available
            return ((long)val).ToString();
        }
        // IComparable
        public int CompareTo(object obj)
        {
            if (obj == null) return 1;
            if (obj is Decimal d) return CompareTo(d);
            throw new ArgumentException("Object must be Decimal");
        }
        
        public int CompareTo(Decimal other)
        {
            double d1 = ToDouble(this);
            double d2 = ToDouble(other);
            return d1 < d2 ? -1 : (d1 > d2 ? 1 : 0);
        }
        
        public bool Equals(Decimal other) => this == other;
        public override bool Equals(object obj) => obj is Decimal d && Equals(d);
        public override int GetHashCode() => lo.GetHashCode() ^ mid.GetHashCode() ^ hi.GetHashCode() ^ flags;
    }
}

namespace System.Collections
{
    public interface IStructuralComparable
    {
        int CompareTo(object other, System.Collections.IComparer comparer);
    }
    
    public interface IStructuralEquatable
    {
        bool Equals(object other, System.Collections.IEqualityComparer comparer);
        int GetHashCode(System.Collections.IEqualityComparer comparer);
    }
    
    public interface IComparer
    {
        int Compare(object x, object y);
    }
    
    public interface IEqualityComparer
    {
        bool Equals(object x, object y);
        int GetHashCode(object obj);
    }
}
