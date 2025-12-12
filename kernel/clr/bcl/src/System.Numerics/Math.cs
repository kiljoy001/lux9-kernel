/*
 * System.Math - Mathematical functions
 * Lux9 CLR Base Class Library
 */
namespace System
{
    using System.Runtime.CompilerServices;

    /// <summary>
    /// Provides constants and static methods for trigonometric, logarithmic, and other common mathematical functions.
    /// </summary>
    public static class Math
    {
        public const double E = 2.7182818284590452354;
        public const double PI = 3.14159265358979323846;
        public const double Tau = 6.28318530717958647692; // 2 * PI
        
        // Abs
        public static int Abs(int value) => value < 0 ? -value : value;
        public static long Abs(long value) => value < 0 ? -value : value;
        public static float Abs(float value) => value < 0 ? -value : value;
        public static double Abs(double value) => value < 0 ? -value : value;
        public static decimal Abs(decimal value) => value < 0 ? -value : value;
        public static short Abs(short value) => (short)(value < 0 ? -value : value);
        public static sbyte Abs(sbyte value) => (sbyte)(value < 0 ? -value : value);
        
        // Min/Max
        public static int Min(int val1, int val2) => val1 < val2 ? val1 : val2;
        public static int Max(int val1, int val2) => val1 > val2 ? val1 : val2;
        public static long Min(long val1, long val2) => val1 < val2 ? val1 : val2;
        public static long Max(long val1, long val2) => val1 > val2 ? val1 : val2;
        public static float Min(float val1, float val2) => val1 < val2 ? val1 : val2;
        public static float Max(float val1, float val2) => val1 > val2 ? val1 : val2;
        public static double Min(double val1, double val2) => val1 < val2 ? val1 : val2;
        public static double Max(double val1, double val2) => val1 > val2 ? val1 : val2;
        
        // Clamp
        public static int Clamp(int value, int min, int max)
        {
            if (value < min) return min;
            if (value > max) return max;
            return value;
        }
        
        public static double Clamp(double value, double min, double max)
        {
            if (value < min) return min;
            if (value > max) return max;
            return value;
        }
        
        // Sign
        public static int Sign(int value) => value < 0 ? -1 : (value > 0 ? 1 : 0);
        public static int Sign(long value) => value < 0 ? -1 : (value > 0 ? 1 : 0);
        public static int Sign(float value) => value < 0 ? -1 : (value > 0 ? 1 : 0);
        public static int Sign(double value) => value < 0 ? -1 : (value > 0 ? 1 : 0);
        
        // Rounding
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Floor(double d);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Ceiling(double a);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Round(double a);
        
        public static double Round(double value, int digits)
        {
            double scale = Pow(10, digits);
            return Round(value * scale) / scale;
        }
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Truncate(double d);
        
        // Power and roots
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Sqrt(double d);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Pow(double x, double y);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Exp(double d);
        
        public static double Cbrt(double d) => Pow(d, 1.0 / 3.0);
        
        // Logarithms
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Log(double d);
        
        public static double Log(double a, double newBase) => Log(a) / Log(newBase);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Log10(double d);
        
        public static double Log2(double d) => Log(d) / Log(2.0);
        
        // Trigonometry
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Sin(double a);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Cos(double d);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Tan(double a);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Asin(double d);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Acos(double d);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Atan(double d);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Atan2(double y, double x);
        
        // Hyperbolic
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Sinh(double value);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Cosh(double value);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Tanh(double value);
        
        // IEEE remainder
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double IEEERemainder(double x, double y);
        
        // Division
        public static int DivRem(int a, int b, out int result)
        {
            result = a % b;
            return a / b;
        }
        
        public static long DivRem(long a, long b, out long result)
        {
            result = a % b;
            return a / b;
        }
        
        // BigMul
        public static long BigMul(int a, int b) => (long)a * (long)b;
        
        // Fused multiply-add
        public static double FusedMultiplyAdd(double x, double y, double z) => x * y + z;
        
        // Scale
        public static double ScaleB(double x, int n) => x * Pow(2, n);
        
        // Copy sign
        public static double CopySign(double x, double y)
        {
            double absX = Abs(x);
            return y >= 0 ? absX : -absX;
        }
        
        // BitDecrement/Increment (approximate)
        public static double BitDecrement(double x) => x - double.Epsilon;
        public static double BitIncrement(double x) => x + double.Epsilon;
        
        // Reciprocal estimates (simple implementations)
        public static double ReciprocalEstimate(double d) => 1.0 / d;
        public static double ReciprocalSqrtEstimate(double d) => 1.0 / Sqrt(d);
    }
    
    /// <summary>
    /// Provides static methods for bit manipulation.
    /// </summary>
    public static class BitOperations
    {
        public static int LeadingZeroCount(uint value)
        {
            if (value == 0) return 32;
            int count = 0;
            while ((value & 0x80000000) == 0)
            {
                count++;
                value <<= 1;
            }
            return count;
        }
        
        public static int LeadingZeroCount(ulong value)
        {
            if (value == 0) return 64;
            int count = 0;
            while ((value & 0x8000000000000000) == 0)
            {
                count++;
                value <<= 1;
            }
            return count;
        }
        
        public static int TrailingZeroCount(uint value)
        {
            if (value == 0) return 32;
            int count = 0;
            while ((value & 1) == 0)
            {
                count++;
                value >>= 1;
            }
            return count;
        }
        
        public static int TrailingZeroCount(ulong value)
        {
            if (value == 0) return 64;
            int count = 0;
            while ((value & 1) == 0)
            {
                count++;
                value >>= 1;
            }
            return count;
        }
        
        public static int PopCount(uint value)
        {
            int count = 0;
            while (value != 0)
            {
                count += (int)(value & 1);
                value >>= 1;
            }
            return count;
        }
        
        public static int PopCount(ulong value)
        {
            int count = 0;
            while (value != 0)
            {
                count += (int)(value & 1);
                value >>= 1;
            }
            return count;
        }
        
        public static int Log2(uint value)
        {
            return 31 - LeadingZeroCount(value);
        }
        
        public static int Log2(ulong value)
        {
            return 63 - LeadingZeroCount(value);
        }
        
        public static bool IsPow2(int value) => value > 0 && (value & (value - 1)) == 0;
        public static bool IsPow2(uint value) => value != 0 && (value & (value - 1)) == 0;
        public static bool IsPow2(long value) => value > 0 && (value & (value - 1)) == 0;
        public static bool IsPow2(ulong value) => value != 0 && (value & (value - 1)) == 0;
        
        public static uint RoundUpToPowerOf2(uint value)
        {
            --value;
            value |= value >> 1;
            value |= value >> 2;
            value |= value >> 4;
            value |= value >> 8;
            value |= value >> 16;
            return value + 1;
        }
        
        public static uint RotateLeft(uint value, int offset) => 
            (value << offset) | (value >> (32 - offset));
            
        public static uint RotateRight(uint value, int offset) => 
            (value >> offset) | (value << (32 - offset));
            
        public static ulong RotateLeft(ulong value, int offset) => 
            (value << offset) | (value >> (64 - offset));
            
        public static ulong RotateRight(ulong value, int offset) => 
            (value >> offset) | (value << (64 - offset));
    }
}

namespace System.Numerics
{
    using System;

    /// <summary>
    /// Represents a 2D vector.
    /// </summary>
    public struct Vector2 : IEquatable<Vector2>
    {
        public float X;
        public float Y;
        
        public static readonly Vector2 Zero = new Vector2(0, 0);
        public static readonly Vector2 One = new Vector2(1, 1);
        public static readonly Vector2 UnitX = new Vector2(1, 0);
        public static readonly Vector2 UnitY = new Vector2(0, 1);
        
        public Vector2(float value) : this(value, value) { }
        
        public Vector2(float x, float y)
        {
            X = x;
            Y = y;
        }
        
        public float Length() => (float)Math.Sqrt(X * X + Y * Y);
        public float LengthSquared() => X * X + Y * Y;
        
        public static float Distance(Vector2 value1, Vector2 value2)
        {
            float dx = value1.X - value2.X;
            float dy = value1.Y - value2.Y;
            return (float)Math.Sqrt(dx * dx + dy * dy);
        }
        
        public static float Dot(Vector2 value1, Vector2 value2) => 
            value1.X * value2.X + value1.Y * value2.Y;
        
        public static Vector2 Normalize(Vector2 value)
        {
            float len = value.Length();
            return new Vector2(value.X / len, value.Y / len);
        }
        
        public static Vector2 Lerp(Vector2 value1, Vector2 value2, float amount)
        {
            return new Vector2(
                value1.X + (value2.X - value1.X) * amount,
                value1.Y + (value2.Y - value1.Y) * amount);
        }
        
        public static Vector2 operator +(Vector2 left, Vector2 right) => 
            new Vector2(left.X + right.X, left.Y + right.Y);
        public static Vector2 operator -(Vector2 left, Vector2 right) => 
            new Vector2(left.X - right.X, left.Y - right.Y);
        public static Vector2 operator *(Vector2 left, Vector2 right) => 
            new Vector2(left.X * right.X, left.Y * right.Y);
        public static Vector2 operator *(Vector2 left, float right) => 
            new Vector2(left.X * right, left.Y * right);
        public static Vector2 operator *(float left, Vector2 right) => right * left;
        public static Vector2 operator /(Vector2 left, Vector2 right) => 
            new Vector2(left.X / right.X, left.Y / right.Y);
        public static Vector2 operator /(Vector2 left, float right) => 
            new Vector2(left.X / right, left.Y / right);
        public static Vector2 operator -(Vector2 value) => new Vector2(-value.X, -value.Y);
        public static bool operator ==(Vector2 left, Vector2 right) => left.X == right.X && left.Y == right.Y;
        public static bool operator !=(Vector2 left, Vector2 right) => !(left == right);
        
        public bool Equals(Vector2 other) => X == other.X && Y == other.Y;
        public override bool Equals(object obj) => obj is Vector2 v && Equals(v);
        public override int GetHashCode() => X.GetHashCode() ^ Y.GetHashCode();
        public override string ToString() => String.Concat("<", X.ToString(), ", ", Y.ToString(), ">");
    }

    /// <summary>
    /// Represents a 3D vector.
    /// </summary>
    public struct Vector3 : IEquatable<Vector3>
    {
        public float X;
        public float Y;
        public float Z;
        
        public static readonly Vector3 Zero = new Vector3(0, 0, 0);
        public static readonly Vector3 One = new Vector3(1, 1, 1);
        public static readonly Vector3 UnitX = new Vector3(1, 0, 0);
        public static readonly Vector3 UnitY = new Vector3(0, 1, 0);
        public static readonly Vector3 UnitZ = new Vector3(0, 0, 1);
        
        public Vector3(float value) : this(value, value, value) { }
        public Vector3(Vector2 vec, float z) : this(vec.X, vec.Y, z) { }
        
        public Vector3(float x, float y, float z)
        {
            X = x;
            Y = y;
            Z = z;
        }
        
        public float Length() => (float)Math.Sqrt(X * X + Y * Y + Z * Z);
        public float LengthSquared() => X * X + Y * Y + Z * Z;
        
        public static float Dot(Vector3 vector1, Vector3 vector2) => 
            vector1.X * vector2.X + vector1.Y * vector2.Y + vector1.Z * vector2.Z;
        
        public static Vector3 Cross(Vector3 vector1, Vector3 vector2)
        {
            return new Vector3(
                vector1.Y * vector2.Z - vector1.Z * vector2.Y,
                vector1.Z * vector2.X - vector1.X * vector2.Z,
                vector1.X * vector2.Y - vector1.Y * vector2.X);
        }
        
        public static Vector3 Normalize(Vector3 value)
        {
            float len = value.Length();
            return new Vector3(value.X / len, value.Y / len, value.Z / len);
        }
        
        public static Vector3 Lerp(Vector3 value1, Vector3 value2, float amount)
        {
            return new Vector3(
                value1.X + (value2.X - value1.X) * amount,
                value1.Y + (value2.Y - value1.Y) * amount,
                value1.Z + (value2.Z - value1.Z) * amount);
        }
        
        public static Vector3 operator +(Vector3 left, Vector3 right) => 
            new Vector3(left.X + right.X, left.Y + right.Y, left.Z + right.Z);
        public static Vector3 operator -(Vector3 left, Vector3 right) => 
            new Vector3(left.X - right.X, left.Y - right.Y, left.Z - right.Z);
        public static Vector3 operator *(Vector3 left, float right) => 
            new Vector3(left.X * right, left.Y * right, left.Z * right);
        public static Vector3 operator *(float left, Vector3 right) => right * left;
        public static Vector3 operator /(Vector3 left, float right) => 
            new Vector3(left.X / right, left.Y / right, left.Z / right);
        public static Vector3 operator -(Vector3 value) => new Vector3(-value.X, -value.Y, -value.Z);
        public static bool operator ==(Vector3 left, Vector3 right) => 
            left.X == right.X && left.Y == right.Y && left.Z == right.Z;
        public static bool operator !=(Vector3 left, Vector3 right) => !(left == right);
        
        public bool Equals(Vector3 other) => X == other.X && Y == other.Y && Z == other.Z;
        public override bool Equals(object obj) => obj is Vector3 v && Equals(v);
        public override int GetHashCode() => X.GetHashCode() ^ Y.GetHashCode() ^ Z.GetHashCode();
        public override string ToString() => String.Concat("<", X.ToString(), ", ", Y.ToString(), ", ", Z.ToString(), ">");
    }

    /// <summary>
    /// Represents a 4D vector.
    /// </summary>
    public struct Vector4 : IEquatable<Vector4>
    {
        public float X;
        public float Y;
        public float Z;
        public float W;
        
        public static readonly Vector4 Zero = new Vector4(0, 0, 0, 0);
        public static readonly Vector4 One = new Vector4(1, 1, 1, 1);
        public static readonly Vector4 UnitX = new Vector4(1, 0, 0, 0);
        public static readonly Vector4 UnitY = new Vector4(0, 1, 0, 0);
        public static readonly Vector4 UnitZ = new Vector4(0, 0, 1, 0);
        public static readonly Vector4 UnitW = new Vector4(0, 0, 0, 1);
        
        public Vector4(float value) : this(value, value, value, value) { }
        public Vector4(Vector3 vec, float w) : this(vec.X, vec.Y, vec.Z, w) { }
        public Vector4(Vector2 vec, float z, float w) : this(vec.X, vec.Y, z, w) { }
        
        public Vector4(float x, float y, float z, float w)
        {
            X = x;
            Y = y;
            Z = z;
            W = w;
        }
        
        public float Length() => (float)Math.Sqrt(X * X + Y * Y + Z * Z + W * W);
        public float LengthSquared() => X * X + Y * Y + Z * Z + W * W;
        
        public static float Dot(Vector4 vector1, Vector4 vector2) => 
            vector1.X * vector2.X + vector1.Y * vector2.Y + 
            vector1.Z * vector2.Z + vector1.W * vector2.W;
        
        public static Vector4 Normalize(Vector4 value)
        {
            float len = value.Length();
            return new Vector4(value.X / len, value.Y / len, value.Z / len, value.W / len);
        }
        
        public static Vector4 operator +(Vector4 left, Vector4 right) => 
            new Vector4(left.X + right.X, left.Y + right.Y, left.Z + right.Z, left.W + right.W);
        public static Vector4 operator -(Vector4 left, Vector4 right) => 
            new Vector4(left.X - right.X, left.Y - right.Y, left.Z - right.Z, left.W - right.W);
        public static Vector4 operator *(Vector4 left, float right) => 
            new Vector4(left.X * right, left.Y * right, left.Z * right, left.W * right);
        public static Vector4 operator -(Vector4 value) => 
            new Vector4(-value.X, -value.Y, -value.Z, -value.W);
        public static bool operator ==(Vector4 left, Vector4 right) => 
            left.X == right.X && left.Y == right.Y && left.Z == right.Z && left.W == right.W;
        public static bool operator !=(Vector4 left, Vector4 right) => !(left == right);
        
        public bool Equals(Vector4 other) => X == other.X && Y == other.Y && Z == other.Z && W == other.W;
        public override bool Equals(object obj) => obj is Vector4 v && Equals(v);
        public override int GetHashCode() => X.GetHashCode() ^ Y.GetHashCode() ^ Z.GetHashCode() ^ W.GetHashCode();
        public override string ToString() => String.Concat("<", X.ToString(), ", ", Y.ToString(), ", ", Z.ToString(), ", ", W.ToString(), ">");
    }

    /// <summary>
    /// Represents a 4x4 matrix.
    /// </summary>
    public struct Matrix4x4 : IEquatable<Matrix4x4>
    {
        public float M11, M12, M13, M14;
        public float M21, M22, M23, M24;
        public float M31, M32, M33, M34;
        public float M41, M42, M43, M44;
        
        public static readonly Matrix4x4 Identity = new Matrix4x4(
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1);
        
        public Matrix4x4(
            float m11, float m12, float m13, float m14,
            float m21, float m22, float m23, float m24,
            float m31, float m32, float m33, float m34,
            float m41, float m42, float m43, float m44)
        {
            M11 = m11; M12 = m12; M13 = m13; M14 = m14;
            M21 = m21; M22 = m22; M23 = m23; M24 = m24;
            M31 = m31; M32 = m32; M33 = m33; M34 = m34;
            M41 = m41; M42 = m42; M43 = m43; M44 = m44;
        }
        
        public bool IsIdentity => this == Identity;
        
        public Vector3 Translation
        {
            get => new Vector3(M41, M42, M43);
            set { M41 = value.X; M42 = value.Y; M43 = value.Z; }
        }
        
        public static Matrix4x4 CreateTranslation(Vector3 position) =>
            CreateTranslation(position.X, position.Y, position.Z);
        
        public static Matrix4x4 CreateTranslation(float x, float y, float z)
        {
            var result = Identity;
            result.M41 = x; result.M42 = y; result.M43 = z;
            return result;
        }
        
        public static Matrix4x4 CreateScale(float scale) => 
            CreateScale(scale, scale, scale);
        
        public static Matrix4x4 CreateScale(float x, float y, float z)
        {
            var result = Identity;
            result.M11 = x; result.M22 = y; result.M33 = z;
            return result;
        }
        
        public static Matrix4x4 operator *(Matrix4x4 value1, Matrix4x4 value2)
        {
            return new Matrix4x4(
                value1.M11 * value2.M11 + value1.M12 * value2.M21 + value1.M13 * value2.M31 + value1.M14 * value2.M41,
                value1.M11 * value2.M12 + value1.M12 * value2.M22 + value1.M13 * value2.M32 + value1.M14 * value2.M42,
                value1.M11 * value2.M13 + value1.M12 * value2.M23 + value1.M13 * value2.M33 + value1.M14 * value2.M43,
                value1.M11 * value2.M14 + value1.M12 * value2.M24 + value1.M13 * value2.M34 + value1.M14 * value2.M44,
                value1.M21 * value2.M11 + value1.M22 * value2.M21 + value1.M23 * value2.M31 + value1.M24 * value2.M41,
                value1.M21 * value2.M12 + value1.M22 * value2.M22 + value1.M23 * value2.M32 + value1.M24 * value2.M42,
                value1.M21 * value2.M13 + value1.M22 * value2.M23 + value1.M23 * value2.M33 + value1.M24 * value2.M43,
                value1.M21 * value2.M14 + value1.M22 * value2.M24 + value1.M23 * value2.M34 + value1.M24 * value2.M44,
                value1.M31 * value2.M11 + value1.M32 * value2.M21 + value1.M33 * value2.M31 + value1.M34 * value2.M41,
                value1.M31 * value2.M12 + value1.M32 * value2.M22 + value1.M33 * value2.M32 + value1.M34 * value2.M42,
                value1.M31 * value2.M13 + value1.M32 * value2.M23 + value1.M33 * value2.M33 + value1.M34 * value2.M43,
                value1.M31 * value2.M14 + value1.M32 * value2.M24 + value1.M33 * value2.M34 + value1.M34 * value2.M44,
                value1.M41 * value2.M11 + value1.M42 * value2.M21 + value1.M43 * value2.M31 + value1.M44 * value2.M41,
                value1.M41 * value2.M12 + value1.M42 * value2.M22 + value1.M43 * value2.M32 + value1.M44 * value2.M42,
                value1.M41 * value2.M13 + value1.M42 * value2.M23 + value1.M43 * value2.M33 + value1.M44 * value2.M43,
                value1.M41 * value2.M14 + value1.M42 * value2.M24 + value1.M43 * value2.M34 + value1.M44 * value2.M44);
        }
        
        public static bool operator ==(Matrix4x4 value1, Matrix4x4 value2) =>
            value1.M11 == value2.M11 && value1.M12 == value2.M12 && value1.M13 == value2.M13 && value1.M14 == value2.M14 &&
            value1.M21 == value2.M21 && value1.M22 == value2.M22 && value1.M23 == value2.M23 && value1.M24 == value2.M24 &&
            value1.M31 == value2.M31 && value1.M32 == value2.M32 && value1.M33 == value2.M33 && value1.M34 == value2.M34 &&
            value1.M41 == value2.M41 && value1.M42 == value2.M42 && value1.M43 == value2.M43 && value1.M44 == value2.M44;
        
        public static bool operator !=(Matrix4x4 value1, Matrix4x4 value2) => !(value1 == value2);
        
        public bool Equals(Matrix4x4 other) => this == other;
        public override bool Equals(object obj) => obj is Matrix4x4 m && Equals(m);
        public override int GetHashCode() => M11.GetHashCode() ^ M22.GetHashCode() ^ M33.GetHashCode() ^ M44.GetHashCode();
    }

    /// <summary>
    /// Represents a quaternion for rotations.
    /// </summary>
    public struct Quaternion : IEquatable<Quaternion>
    {
        public float X;
        public float Y;
        public float Z;
        public float W;
        
        public static readonly Quaternion Identity = new Quaternion(0, 0, 0, 1);
        
        public Quaternion(float x, float y, float z, float w)
        {
            X = x;
            Y = y;
            Z = z;
            W = w;
        }
        
        public Quaternion(Vector3 vectorPart, float scalarPart)
            : this(vectorPart.X, vectorPart.Y, vectorPart.Z, scalarPart) { }
        
        public bool IsIdentity => X == 0 && Y == 0 && Z == 0 && W == 1;
        
        public float Length() => (float)Math.Sqrt(X * X + Y * Y + Z * Z + W * W);
        public float LengthSquared() => X * X + Y * Y + Z * Z + W * W;
        
        public static Quaternion Normalize(Quaternion value)
        {
            float len = value.Length();
            return new Quaternion(value.X / len, value.Y / len, value.Z / len, value.W / len);
        }
        
        public static Quaternion Conjugate(Quaternion value) =>
            new Quaternion(-value.X, -value.Y, -value.Z, value.W);
        
        public static Quaternion Inverse(Quaternion value)
        {
            float ls = value.LengthSquared();
            return new Quaternion(-value.X / ls, -value.Y / ls, -value.Z / ls, value.W / ls);
        }
        
        public static float Dot(Quaternion quaternion1, Quaternion quaternion2) =>
            quaternion1.X * quaternion2.X + quaternion1.Y * quaternion2.Y +
            quaternion1.Z * quaternion2.Z + quaternion1.W * quaternion2.W;
        
        public static Quaternion operator *(Quaternion value1, Quaternion value2)
        {
            return new Quaternion(
                value1.W * value2.X + value1.X * value2.W + value1.Y * value2.Z - value1.Z * value2.Y,
                value1.W * value2.Y - value1.X * value2.Z + value1.Y * value2.W + value1.Z * value2.X,
                value1.W * value2.Z + value1.X * value2.Y - value1.Y * value2.X + value1.Z * value2.W,
                value1.W * value2.W - value1.X * value2.X - value1.Y * value2.Y - value1.Z * value2.Z);
        }
        
        public static bool operator ==(Quaternion value1, Quaternion value2) =>
            value1.X == value2.X && value1.Y == value2.Y && value1.Z == value2.Z && value1.W == value2.W;
        public static bool operator !=(Quaternion value1, Quaternion value2) => !(value1 == value2);
        
        public bool Equals(Quaternion other) => this == other;
        public override bool Equals(object obj) => obj is Quaternion q && Equals(q);
        public override int GetHashCode() => X.GetHashCode() ^ Y.GetHashCode() ^ Z.GetHashCode() ^ W.GetHashCode();
        public override string ToString() => String.Concat("<", X.ToString(), ", ", Y.ToString(), ", ", Z.ToString(), ", ", W.ToString(), ">");
    }
    
    /// <summary>
    /// Represents a complex number.
    /// </summary>
    public readonly struct Complex : IEquatable<Complex>
    {
        public double Real { get; }
        public double Imaginary { get; }
        
        public static readonly Complex Zero = new Complex(0, 0);
        public static readonly Complex One = new Complex(1, 0);
        public static readonly Complex ImaginaryOne = new Complex(0, 1);
        
        public Complex(double real, double imaginary)
        {
            Real = real;
            Imaginary = imaginary;
        }
        
        public double Magnitude => Math.Sqrt(Real * Real + Imaginary * Imaginary);
        public double Phase => Math.Atan2(Imaginary, Real);
        
        public static Complex Conjugate(Complex value) => new Complex(value.Real, -value.Imaginary);
        
        public static Complex operator +(Complex left, Complex right) =>
            new Complex(left.Real + right.Real, left.Imaginary + right.Imaginary);
        public static Complex operator -(Complex left, Complex right) =>
            new Complex(left.Real - right.Real, left.Imaginary - right.Imaginary);
        public static Complex operator *(Complex left, Complex right) =>
            new Complex(
                left.Real * right.Real - left.Imaginary * right.Imaginary,
                left.Real * right.Imaginary + left.Imaginary * right.Real);
        public static Complex operator /(Complex left, Complex right)
        {
            double denom = right.Real * right.Real + right.Imaginary * right.Imaginary;
            return new Complex(
                (left.Real * right.Real + left.Imaginary * right.Imaginary) / denom,
                (left.Imaginary * right.Real - left.Real * right.Imaginary) / denom);
        }
        public static Complex operator -(Complex value) => new Complex(-value.Real, -value.Imaginary);
        public static bool operator ==(Complex left, Complex right) => 
            left.Real == right.Real && left.Imaginary == right.Imaginary;
        public static bool operator !=(Complex left, Complex right) => !(left == right);
        
        public bool Equals(Complex other) => Real == other.Real && Imaginary == other.Imaginary;
        public override bool Equals(object obj) => obj is Complex c && Equals(c);
        public override int GetHashCode() => Real.GetHashCode() ^ Imaginary.GetHashCode();
        public override string ToString() => String.Concat("(", Real.ToString(), ", ", Imaginary.ToString(), ")");
    }
}
