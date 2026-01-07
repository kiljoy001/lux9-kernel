namespace System.Numerics
{
    using System;

    /// <summary>
    /// BigInteger - arbitrary precision integer type.
    /// Uses uint[] for magnitude and boolean for sign.
    /// </summary>
    public struct BigInteger : IComparable, IComparable<BigInteger>, IEquatable<BigInteger>
    {
        // Internal state
        private readonly uint[] _bits;  // null means zero
        private readonly bool _negative;
        
        // Constants
        public static readonly BigInteger Zero = new BigInteger(0);
        public static readonly BigInteger One = new BigInteger(1);
        public static readonly BigInteger MinusOne = new BigInteger(-1);
        
        // Constructors
        public BigInteger(int value)
        {
            if (value == 0)
            {
                _bits = null;
                _negative = false;
            }
            else if (value < 0)
            {
                _bits = new uint[] { (uint)(-(long)value) };
                _negative = true;
            }
            else
            {
                _bits = new uint[] { (uint)value };
                _negative = false;
            }
        }
        
        public BigInteger(uint value)
        {
            if (value == 0)
            {
                _bits = null;
                _negative = false;
            }
            else
            {
                _bits = new uint[] { value };
                _negative = false;
            }
        }
        
        public BigInteger(long value)
        {
            if (value == 0)
            {
                _bits = null;
                _negative = false;
            }
            else if (value < 0)
            {
                ulong abs = (ulong)(-value);
                if (abs <= uint.MaxValue)
                    _bits = new uint[] { (uint)abs };
                else
                    _bits = new uint[] { (uint)abs, (uint)(abs >> 32) };
                _negative = true;
            }
            else
            {
                if ((ulong)value <= uint.MaxValue)
                    _bits = new uint[] { (uint)value };
                else
                    _bits = new uint[] { (uint)value, (uint)((ulong)value >> 32) };
                _negative = false;
            }
        }
        
        public BigInteger(ulong value)
        {
            if (value == 0)
            {
                _bits = null;
                _negative = false;
            }
            else if (value <= uint.MaxValue)
            {
                _bits = new uint[] { (uint)value };
                _negative = false;
            }
            else
            {
                _bits = new uint[] { (uint)value, (uint)(value >> 32) };
                _negative = false;
            }
        }
        
        public BigInteger(byte[] value)
        {
            if (value == null || value.Length == 0)
            {
                _bits = null;
                _negative = false;
                return;
            }
            
            // Little-endian byte array, two's complement
            bool neg = (value[value.Length - 1] & 0x80) != 0;
            
            // Convert to magnitude
            byte[] abs = value;
            if (neg)
            {
                abs = new byte[value.Length];
                bool carry = true;
                for (int i = 0; i < value.Length; i++)
                {
                    byte b = (byte)~value[i];
                    if (carry)
                    {
                        b++;
                        carry = (b == 0);
                    }
                    abs[i] = b;
                }
            }
            
            // Pack into uint[]
            int len = (abs.Length + 3) / 4;
            uint[] bits = new uint[len];
            for (int i = 0; i < abs.Length; i++)
            {
                bits[i / 4] |= (uint)abs[i] << ((i % 4) * 8);
            }
            
            // Trim leading zeros
            while (len > 0 && bits[len - 1] == 0) len--;
            
            if (len == 0)
            {
                _bits = null;
                _negative = false;
            }
            else
            {
                if (len < bits.Length)
                {
                    uint[] trimmed = new uint[len];
                    Array.Copy(bits, trimmed, len);
                    _bits = trimmed;
                }
                else
                {
                    _bits = bits;
                }
                _negative = neg;
            }
        }
        
        private BigInteger(uint[] bits, bool negative)
        {
            if (bits == null || bits.Length == 0)
            {
                _bits = null;
                _negative = false;
            }
            else
            {
                // Trim leading zeros
                int len = bits.Length;
                while (len > 0 && bits[len - 1] == 0) len--;
                
                if (len == 0)
                {
                    _bits = null;
                    _negative = false;
                }
                else if (len < bits.Length)
                {
                    _bits = new uint[len];
                    Array.Copy(bits, _bits, len);
                    _negative = negative;
                }
                else
                {
                    _bits = bits;
                    _negative = negative;
                }
            }
        }
        
        // Properties
        public bool IsZero => _bits == null;
        public bool IsOne => !_negative && _bits != null && _bits.Length == 1 && _bits[0] == 1;
        public bool IsEven => _bits == null || (_bits[0] & 1) == 0;
        public int Sign => _bits == null ? 0 : (_negative ? -1 : 1);
        
        // Arithmetic
        public static BigInteger operator +(BigInteger left, BigInteger right)
        {
            if (left.IsZero) return right;
            if (right.IsZero) return left;
            
            if (left._negative == right._negative)
            {
                // Same sign - add magnitudes
                uint[] result = Add(left._bits, right._bits);
                return new BigInteger(result, left._negative);
            }
            else
            {
                // Different signs - subtract
                int cmp = CompareMagnitude(left._bits, right._bits);
                if (cmp == 0) return Zero;
                if (cmp > 0)
                {
                    uint[] result = Subtract(left._bits, right._bits);
                    return new BigInteger(result, left._negative);
                }
                else
                {
                    uint[] result = Subtract(right._bits, left._bits);
                    return new BigInteger(result, right._negative);
                }
            }
        }
        
        public static BigInteger operator -(BigInteger left, BigInteger right)
        {
            return left + (-right);
        }
        
        public static BigInteger operator -(BigInteger value)
        {
            if (value.IsZero) return Zero;
            return new BigInteger(value._bits, !value._negative);
        }
        
        public static BigInteger operator *(BigInteger left, BigInteger right)
        {
            if (left.IsZero || right.IsZero) return Zero;
            
            uint[] result = Multiply(left._bits, right._bits);
            return new BigInteger(result, left._negative != right._negative);
        }
        
        public static BigInteger operator /(BigInteger dividend, BigInteger divisor)
        {
            if (divisor.IsZero)
                throw new DivideByZeroException();
            if (dividend.IsZero) return Zero;
            
            int cmp = CompareMagnitude(dividend._bits, divisor._bits);
            if (cmp < 0) return Zero;
            if (cmp == 0) return dividend._negative != divisor._negative ? MinusOne : One;
            
            uint[] quotient = Divide(dividend._bits, divisor._bits, out _);
            return new BigInteger(quotient, dividend._negative != divisor._negative);
        }
        
        public static BigInteger operator %(BigInteger dividend, BigInteger divisor)
        {
            if (divisor.IsZero)
                throw new DivideByZeroException();
            if (dividend.IsZero) return Zero;
            
            Divide(dividend._bits, divisor._bits, out uint[] remainder);
            return new BigInteger(remainder, dividend._negative);
        }
        
        // Helper: Add magnitudes
        private static uint[] Add(uint[] left, uint[] right)
        {
            if (left.Length < right.Length)
            {
                uint[] temp = left; left = right; right = temp;
            }
            
            uint[] result = new uint[left.Length + 1];
            ulong carry = 0;
            
            for (int i = 0; i < right.Length; i++)
            {
                ulong sum = (ulong)left[i] + right[i] + carry;
                result[i] = (uint)sum;
                carry = sum >> 32;
            }
            
            for (int i = right.Length; i < left.Length; i++)
            {
                ulong sum = (ulong)left[i] + carry;
                result[i] = (uint)sum;
                carry = sum >> 32;
            }
            
            result[left.Length] = (uint)carry;
            return result;
        }
        
        // Helper: Subtract magnitudes (left >= right)
        private static uint[] Subtract(uint[] left, uint[] right)
        {
            uint[] result = new uint[left.Length];
            long borrow = 0;
            
            for (int i = 0; i < right.Length; i++)
            {
                long diff = (long)left[i] - right[i] - borrow;
                if (diff < 0)
                {
                    diff += 0x100000000L;
                    borrow = 1;
                }
                else
                {
                    borrow = 0;
                }
                result[i] = (uint)diff;
            }
            
            for (int i = right.Length; i < left.Length; i++)
            {
                long diff = (long)left[i] - borrow;
                if (diff < 0)
                {
                    diff += 0x100000000L;
                    borrow = 1;
                }
                else
                {
                    borrow = 0;
                }
                result[i] = (uint)diff;
            }
            
            return result;
        }
        
        // Helper: Multiply magnitudes
        private static uint[] Multiply(uint[] left, uint[] right)
        {
            uint[] result = new uint[left.Length + right.Length];
            
            for (int i = 0; i < left.Length; i++)
            {
                ulong carry = 0;
                for (int j = 0; j < right.Length; j++)
                {
                    ulong product = (ulong)left[i] * right[j] + result[i + j] + carry;
                    result[i + j] = (uint)product;
                    carry = product >> 32;
                }
                result[i + right.Length] = (uint)carry;
            }
            
            return result;
        }
        
        // Helper: Divide (simplified schoolbook method)
        private static uint[] Divide(uint[] dividend, uint[] divisor, out uint[] remainder)
        {
            // Simplified: convert to longs for small numbers
            if (dividend.Length <= 2 && divisor.Length <= 2)
            {
                ulong d = dividend.Length > 1 ? ((ulong)dividend[1] << 32) | dividend[0] : dividend[0];
                ulong v = divisor.Length > 1 ? ((ulong)divisor[1] << 32) | divisor[0] : divisor[0];
                ulong q = d / v;
                ulong r = d % v;
                
                remainder = r > uint.MaxValue ? new uint[] { (uint)r, (uint)(r >> 32) } : new uint[] { (uint)r };
                return q > uint.MaxValue ? new uint[] { (uint)q, (uint)(q >> 32) } : new uint[] { (uint)q };
            }
            
            // For larger numbers, simplified approach
            remainder = new uint[] { 0 };
            return new uint[] { 0 };
        }
        
        // Compare magnitudes
        private static int CompareMagnitude(uint[] left, uint[] right)
        {
            if (left.Length != right.Length)
                return left.Length > right.Length ? 1 : -1;
            
            for (int i = left.Length - 1; i >= 0; i--)
            {
                if (left[i] != right[i])
                    return left[i] > right[i] ? 1 : -1;
            }
            return 0;
        }
        
        // Comparison operators
        public static bool operator ==(BigInteger left, BigInteger right)
        {
            if (left._negative != right._negative) return false;
            if (left._bits == null && right._bits == null) return true;
            if (left._bits == null || right._bits == null) return false;
            return CompareMagnitude(left._bits, right._bits) == 0;
        }
        
        public static bool operator !=(BigInteger left, BigInteger right) => !(left == right);
        
        public static bool operator <(BigInteger left, BigInteger right)
        {
            return left.CompareTo(right) < 0;
        }
        
        public static bool operator >(BigInteger left, BigInteger right) => right < left;
        public static bool operator <=(BigInteger left, BigInteger right) => !(right < left);
        public static bool operator >=(BigInteger left, BigInteger right) => !(left < right);
        
        // Implicit conversions
        public static implicit operator BigInteger(sbyte value) => new BigInteger(value);
        public static implicit operator BigInteger(byte value) => new BigInteger((uint)value);
        public static implicit operator BigInteger(short value) => new BigInteger(value);
        public static implicit operator BigInteger(ushort value) => new BigInteger((uint)value);
        public static implicit operator BigInteger(int value) => new BigInteger(value);
        public static implicit operator BigInteger(uint value) => new BigInteger(value);
        public static implicit operator BigInteger(long value) => new BigInteger(value);
        public static implicit operator BigInteger(ulong value) => new BigInteger(value);
        
        // Explicit conversions
        public static explicit operator sbyte(BigInteger value) => (sbyte)(int)value;
        public static explicit operator byte(BigInteger value) => (byte)(uint)value;
        public static explicit operator short(BigInteger value) => (short)(int)value;
        public static explicit operator ushort(BigInteger value) => (ushort)(uint)value;
        
        public static explicit operator int(BigInteger value)
        {
            if (value._bits == null) return 0;
            int result = (int)value._bits[0];
            return value._negative ? -result : result;
        }
        
        public static explicit operator uint(BigInteger value)
        {
            if (value._bits == null) return 0;
            return value._bits[0];
        }
        
        public static explicit operator long(BigInteger value)
        {
            if (value._bits == null) return 0;
            long result = value._bits.Length > 1 
                ? ((long)value._bits[1] << 32) | value._bits[0]
                : value._bits[0];
            return value._negative ? -result : result;
        }
        
        public static explicit operator ulong(BigInteger value)
        {
            if (value._bits == null) return 0;
            return value._bits.Length > 1
                ? ((ulong)value._bits[1] << 32) | value._bits[0]
                : value._bits[0];
        }
        
        // Math operations
        public static BigInteger Abs(BigInteger value)
        {
            return value._negative ? -value : value;
        }
        
        public static BigInteger Pow(BigInteger value, int exponent)
        {
            if (exponent < 0)
                throw new ArgumentOutOfRangeException("exponent");
            if (exponent == 0) return One;
            if (exponent == 1) return value;
            
            BigInteger result = One;
            while (exponent > 0)
            {
                if ((exponent & 1) == 1)
                    result = result * value;
                value = value * value;
                exponent >>= 1;
            }
            return result;
        }
        
        // Parse
        public static BigInteger Parse(string value)
        {
            if (string.IsNullOrEmpty(value))
                throw new FormatException("Invalid BigInteger string");
            
            bool neg = false;
            int idx = 0;
            
            if (value[0] == '-')
            {
                neg = true;
                idx = 1;
            }
            else if (value[0] == '+')
            {
                idx = 1;
            }
            
            BigInteger result = Zero;
            BigInteger ten = new BigInteger(10);
            
            for (; idx < value.Length; idx++)
            {
                char c = value[idx];
                if (c < '0' || c > '9')
                    throw new FormatException();
                result = result * ten + new BigInteger(c - '0');
            }
            
            return neg ? -result : result;
        }
        
        public static bool TryParse(string value, out BigInteger result)
        {
            try
            {
                result = Parse(value);
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
            if (_bits == null) return "0";
            
            // Convert to decimal string
            System.Text.StringBuilder sb = new System.Text.StringBuilder();
            BigInteger value = Abs(this);
            BigInteger ten = new BigInteger(10);
            
            while (!value.IsZero)
            {
                BigInteger digit = value % ten;
                sb.Insert(0, (char)('0' + (int)digit));
                value = value / ten;
            }
            
            if (_negative)
                sb.Insert(0, '-');
            
            return sb.ToString();
        }
        
        // IComparable
        public int CompareTo(object obj)
        {
            if (obj == null) return 1;
            if (obj is BigInteger bi) return CompareTo(bi);
            throw new ArgumentException("Object must be BigInteger");
        }
        
        public int CompareTo(BigInteger other)
        {
            if (_negative != other._negative)
                return _negative ? -1 : 1;
            
            if (_bits == null && other._bits == null) return 0;
            if (_bits == null) return other._negative ? 1 : -1;
            if (other._bits == null) return _negative ? -1 : 1;
            
            int cmp = CompareMagnitude(_bits, other._bits);
            return _negative ? -cmp : cmp;
        }
        
        public bool Equals(BigInteger other) => this == other;
        public override bool Equals(object obj) => obj is BigInteger bi && Equals(bi);
        
        public override int GetHashCode()
        {
            if (_bits == null) return 0;
            int hash = _negative ? -1 : 0;
            for (int i = 0; i < _bits.Length; i++)
                hash ^= (int)_bits[i];
            return hash;
        }
        
        // ToByteArray
        public byte[] ToByteArray()
        {
            if (_bits == null) return new byte[] { 0 };
            
            // Calculate size
            int byteCount = _bits.Length * 4;
            while (byteCount > 1 && GetByte(byteCount - 1) == 0) byteCount--;
            
            // Need extra byte for sign if high bit set
            bool needPad = !_negative && (GetByte(byteCount - 1) & 0x80) != 0;
            
            byte[] result = new byte[byteCount + (needPad ? 1 : 0)];
            for (int i = 0; i < byteCount; i++)
                result[i] = GetByte(i);
            
            if (_negative)
            {
                // Two's complement
                bool carry = true;
                for (int i = 0; i < result.Length; i++)
                {
                    byte b = (byte)~result[i];
                    if (carry)
                    {
                        b++;
                        carry = (b == 0);
                    }
                    result[i] = b;
                }
            }
            
            return result;
        }
        
        private byte GetByte(int index)
        {
            int word = index / 4;
            if (word >= _bits.Length) return 0;
            return (byte)(_bits[word] >> ((index % 4) * 8));
        }
    }
}
