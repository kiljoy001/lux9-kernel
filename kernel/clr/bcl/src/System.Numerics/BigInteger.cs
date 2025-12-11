namespace System.Numerics
{
    using System;

    public struct BigInteger : IComparable, IEquatable<BigInteger>
    {
        // Minimal implementation using long. Correct one needs int[] array.
        internal long m_value;

        public BigInteger(int value) { m_value = value; }
        public BigInteger(long value) { m_value = value; }
        
        public static BigInteger Parse(string value)
        {
            // Stub parse
            return new BigInteger(0);
        }

        public static BigInteger operator +(BigInteger left, BigInteger right) => new BigInteger(left.m_value + right.m_value);
        public static BigInteger operator -(BigInteger left, BigInteger right) => new BigInteger(left.m_value - right.m_value);
        public static BigInteger operator *(BigInteger left, BigInteger right) => new BigInteger(left.m_value * right.m_value);
        public static BigInteger operator /(BigInteger left, BigInteger right) => new BigInteger(left.m_value / right.m_value);
        
        public override string ToString() => "BigIntStub";

        public int CompareTo(object obj) => 0; 
        public bool Equals(BigInteger other) => m_value == other.m_value;
    }

}
