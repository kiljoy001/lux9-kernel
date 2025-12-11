namespace System
{
public struct Decimal : IComparable, IEquatable<Decimal>
{
    // Minimal stub using long for now
    private long m_value;

    public Decimal(int value) { m_value = value; }
    public Decimal(long value) { m_value = value; }
    public Decimal(double value) { m_value = (long)value; }
    
    public static Decimal Parse(string s) => new Decimal(0);
    
    public static Decimal operator +(Decimal d1, Decimal d2) => new Decimal(d1.m_value + d2.m_value);
    public static Decimal operator -(Decimal d1, Decimal d2) => new Decimal(d1.m_value - d2.m_value);
    public static Decimal operator *(Decimal d1, Decimal d2) => new Decimal(d1.m_value * d2.m_value);
    public static Decimal operator /(Decimal d1, Decimal d2) => new Decimal(d1.m_value / d2.m_value);
    
    public static bool operator ==(Decimal d1, Decimal d2) => d1.m_value == d2.m_value;
    public static bool operator !=(Decimal d1, Decimal d2) => d1.m_value != d2.m_value;
    public static bool operator <(Decimal d1, Decimal d2) => d1.m_value < d2.m_value;
    public static bool operator >(Decimal d1, Decimal d2) => d1.m_value > d2.m_value;
    public static bool operator <=(Decimal d1, Decimal d2) => d1.m_value <= d2.m_value;
    public static bool operator >=(Decimal d1, Decimal d2) => d1.m_value >= d2.m_value;
    public static Decimal operator -(Decimal d) => new Decimal(-d.m_value);
    public static implicit operator Decimal(int value) => new Decimal(value);
    
    public override string ToString() => "Decimal";
    public int CompareTo(object obj) => 0;
    public bool Equals(Decimal other) => m_value == other.m_value;
    public override bool Equals(object obj) => obj is Decimal d && Equals(d);
    public override int GetHashCode() => m_value.GetHashCode();
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
