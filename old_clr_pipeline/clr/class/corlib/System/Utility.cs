/*
 * ECMA-335 Core Utility Types - Lux9 BCL
 * 
 * Math, GC, Console, Version, TimeSpan, DateTime, etc.
 */
namespace System
{
    // Math (ECMA-335)
    public static class Math
    {
        public const double E = 2.7182818284590452;
        public const double PI = 3.1415926535897932;

        public static int Abs(int value) => value >= 0 ? value : -value;
        public static long Abs(long value) => value >= 0 ? value : -value;
        public static double Abs(double value) => value >= 0 ? value : -value;
        public static float Abs(float value) => value >= 0 ? value : -value;

        public static int Max(int val1, int val2) => val1 >= val2 ? val1 : val2;
        public static long Max(long val1, long val2) => val1 >= val2 ? val1 : val2;
        public static double Max(double val1, double val2) => val1 >= val2 ? val1 : val2;
        public static float Max(float val1, float val2) => val1 >= val2 ? val1 : val2;

        public static int Min(int val1, int val2) => val1 <= val2 ? val1 : val2;
        public static long Min(long val1, long val2) => val1 <= val2 ? val1 : val2;
        public static double Min(double val1, double val2) => val1 <= val2 ? val1 : val2;
        public static float Min(float val1, float val2) => val1 <= val2 ? val1 : val2;

        public static int Sign(int value) => value < 0 ? -1 : (value > 0 ? 1 : 0);
        public static int Sign(long value) => value < 0 ? -1 : (value > 0 ? 1 : 0);
        public static int Sign(double value) => value < 0 ? -1 : (value > 0 ? 1 : 0);

        public static double Ceiling(double a) => (int)a == a ? a : (a > 0 ? (int)a + 1 : (int)a);
        public static double Floor(double d) => d >= 0 ? (int)d : ((int)d == d ? d : (int)d - 1);
        public static double Round(double a) => Floor(a + 0.5);
        public static double Round(double value, int digits) => Round(value * Pow(10, digits)) / Pow(10, digits);
        public static double Truncate(double d) => (int)d;

        public static double Sqrt(double d) => Pow(d, 0.5);
        public static double Pow(double x, double y)
        {
            if (y == 0) return 1;
            if (y == 1) return x;
            double result = 1;
            for (int i = 0; i < (int)y; i++) result *= x;
            return result;
        }
        public static double Log(double d) => 0; // Simplified
        public static double Log10(double d) => 0; // Simplified
        public static double Exp(double d) => Pow(E, d);

        public static double Sin(double a) => 0; // Simplified
        public static double Cos(double d) => 0; // Simplified
        public static double Tan(double a) => 0; // Simplified
        public static double Asin(double d) => 0;
        public static double Acos(double d) => 0;
        public static double Atan(double d) => 0;
        public static double Atan2(double y, double x) => 0;

        public static int Clamp(int value, int min, int max) => value < min ? min : (value > max ? max : value);
        public static long Clamp(long value, long min, long max) => value < min ? min : (value > max ? max : value);
        public static double Clamp(double value, double min, double max) => value < min ? min : (value > max ? max : value);

        public static int DivRem(int a, int b, out int result) { result = a % b; return a / b; }
        public static long DivRem(long a, long b, out long result) { result = a % b; return a / b; }
        public static long BigMul(int a, int b) => (long)a * b;
    }

    // GC (ECMA-335)
    public static class GC
    {
        public static int MaxGeneration => 2;
        public static void Collect() { }
        public static void Collect(int generation) { }
        public static void Collect(int generation, GCCollectionMode mode) { }
        public static int CollectionCount(int generation) => 0;
        public static long GetTotalMemory(bool forceFullCollection) => 0;
        public static void WaitForPendingFinalizers() { }
        public static void SuppressFinalize(Object obj) { }
        public static void ReRegisterForFinalize(Object obj) { }
        public static void KeepAlive(Object obj) { }
        public static void AddMemoryPressure(long bytesAllocated) { }
        public static void RemoveMemoryPressure(long bytesAllocated) { }
    }

    public enum GCCollectionMode { Default = 0, Forced = 1, Optimized = 2 }

    // Version (ECMA-335)
    public sealed class Version : IComparable, IComparable<Version>, IEquatable<Version>, ICloneable
    {
        public int Major { get; }
        public int Minor { get; }
        public int Build { get; }
        public int Revision { get; }

        public Version() { }
        public Version(int major, int minor) { Major = major; Minor = minor; Build = -1; Revision = -1; }
        public Version(int major, int minor, int build) { Major = major; Minor = minor; Build = build; Revision = -1; }
        public Version(int major, int minor, int build, int revision) { Major = major; Minor = minor; Build = build; Revision = revision; }
        public Version(String version) { /* Parse */ }

        public Object Clone() => new Version(Major, Minor, Build, Revision);
        public int CompareTo(Object version) => version is Version v ? CompareTo(v) : throw new ArgumentException();
        public int CompareTo(Version value)
        {
            if (value == null) return 1;
            if (Major != value.Major) return Major > value.Major ? 1 : -1;
            if (Minor != value.Minor) return Minor > value.Minor ? 1 : -1;
            if (Build != value.Build) return Build > value.Build ? 1 : -1;
            if (Revision != value.Revision) return Revision > value.Revision ? 1 : -1;
            return 0;
        }
        public override bool Equals(Object obj) => obj is Version v && Equals(v);
        public bool Equals(Version obj) => obj != null && Major == obj.Major && Minor == obj.Minor && Build == obj.Build && Revision == obj.Revision;
        public override int GetHashCode() => (Major << 24) | (Minor << 16) | (Build << 8) | Revision;
        public override String ToString() => Build < 0 ? $"{Major}.{Minor}" : Revision < 0 ? $"{Major}.{Minor}.{Build}" : $"{Major}.{Minor}.{Build}.{Revision}";
    }

    // TimeSpan (ECMA-335)
    public struct TimeSpan : IComparable, IComparable<TimeSpan>, IEquatable<TimeSpan>
    {
        private long _ticks;
        public const long TicksPerMillisecond = 10000;
        public const long TicksPerSecond = TicksPerMillisecond * 1000;
        public const long TicksPerMinute = TicksPerSecond * 60;
        public const long TicksPerHour = TicksPerMinute * 60;
        public const long TicksPerDay = TicksPerHour * 24;

        public static readonly TimeSpan Zero = new TimeSpan(0);
        public static readonly TimeSpan MinValue = new TimeSpan(Int64.MinValue);
        public static readonly TimeSpan MaxValue = new TimeSpan(Int64.MaxValue);

        public TimeSpan(long ticks) { _ticks = ticks; }
        public TimeSpan(int hours, int minutes, int seconds) : this(0, hours, minutes, seconds, 0) { }
        public TimeSpan(int days, int hours, int minutes, int seconds) : this(days, hours, minutes, seconds, 0) { }
        public TimeSpan(int days, int hours, int minutes, int seconds, int milliseconds)
        {
            _ticks = days * TicksPerDay + hours * TicksPerHour + minutes * TicksPerMinute + seconds * TicksPerSecond + milliseconds * TicksPerMillisecond;
        }

        public long Ticks => _ticks;
        public int Days => (int)(_ticks / TicksPerDay);
        public int Hours => (int)((_ticks / TicksPerHour) % 24);
        public int Minutes => (int)((_ticks / TicksPerMinute) % 60);
        public int Seconds => (int)((_ticks / TicksPerSecond) % 60);
        public int Milliseconds => (int)((_ticks / TicksPerMillisecond) % 1000);
        public double TotalDays => (double)_ticks / TicksPerDay;
        public double TotalHours => (double)_ticks / TicksPerHour;
        public double TotalMinutes => (double)_ticks / TicksPerMinute;
        public double TotalSeconds => (double)_ticks / TicksPerSecond;
        public double TotalMilliseconds => (double)_ticks / TicksPerMillisecond;

        public TimeSpan Add(TimeSpan ts) => new TimeSpan(_ticks + ts._ticks);
        public TimeSpan Subtract(TimeSpan ts) => new TimeSpan(_ticks - ts._ticks);
        public TimeSpan Negate() => new TimeSpan(-_ticks);
        public TimeSpan Duration() => new TimeSpan(_ticks >= 0 ? _ticks : -_ticks);

        public static TimeSpan FromDays(double value) => new TimeSpan((long)(value * TicksPerDay));
        public static TimeSpan FromHours(double value) => new TimeSpan((long)(value * TicksPerHour));
        public static TimeSpan FromMinutes(double value) => new TimeSpan((long)(value * TicksPerMinute));
        public static TimeSpan FromSeconds(double value) => new TimeSpan((long)(value * TicksPerSecond));
        public static TimeSpan FromMilliseconds(double value) => new TimeSpan((long)(value * TicksPerMillisecond));
        public static TimeSpan FromTicks(long value) => new TimeSpan(value);

        public int CompareTo(Object value) => value is TimeSpan ts ? CompareTo(ts) : throw new ArgumentException();
        public int CompareTo(TimeSpan value) => _ticks < value._ticks ? -1 : (_ticks > value._ticks ? 1 : 0);
        public override bool Equals(Object value) => value is TimeSpan ts && Equals(ts);
        public bool Equals(TimeSpan obj) => _ticks == obj._ticks;
        public override int GetHashCode() => (int)_ticks ^ (int)(_ticks >> 32);
        public override String ToString() => $"{Days}.{Hours:D2}:{Minutes:D2}:{Seconds:D2}.{Milliseconds:D3}";

        public static TimeSpan operator +(TimeSpan t1, TimeSpan t2) => t1.Add(t2);
        public static TimeSpan operator -(TimeSpan t1, TimeSpan t2) => t1.Subtract(t2);
        public static TimeSpan operator -(TimeSpan t) => t.Negate();
        public static bool operator ==(TimeSpan t1, TimeSpan t2) => t1._ticks == t2._ticks;
        public static bool operator !=(TimeSpan t1, TimeSpan t2) => t1._ticks != t2._ticks;
        public static bool operator <(TimeSpan t1, TimeSpan t2) => t1._ticks < t2._ticks;
        public static bool operator >(TimeSpan t1, TimeSpan t2) => t1._ticks > t2._ticks;
        public static bool operator <=(TimeSpan t1, TimeSpan t2) => t1._ticks <= t2._ticks;
        public static bool operator >=(TimeSpan t1, TimeSpan t2) => t1._ticks >= t2._ticks;
    }

    // DateTime - simplified (ECMA-335)
    public struct DateTime : IComparable, IComparable<DateTime>, IEquatable<DateTime>
    {
        private ulong _dateData;
        public static readonly DateTime MinValue = new DateTime(0);
        public static readonly DateTime MaxValue = new DateTime(3155378975999999999);

        public DateTime(long ticks) { _dateData = (ulong)ticks; }
        public DateTime(int year, int month, int day) : this(DateToTicks(year, month, day)) { }
        public DateTime(int year, int month, int day, int hour, int minute, int second)
            : this(DateToTicks(year, month, day) + TimeToTicks(hour, minute, second)) { }

        public long Ticks => (long)(_dateData & 0x3FFFFFFFFFFFFFFF);
        public DateTime Date => new DateTime((long)(_dateData - (ulong)(Ticks % TimeSpan.TicksPerDay)));
        public int Year => GetDatePart(0);
        public int Month => GetDatePart(2);
        public int Day => GetDatePart(3);
        public int Hour => (int)((Ticks / TimeSpan.TicksPerHour) % 24);
        public int Minute => (int)((Ticks / TimeSpan.TicksPerMinute) % 60);
        public int Second => (int)((Ticks / TimeSpan.TicksPerSecond) % 60);
        public int Millisecond => (int)((Ticks / TimeSpan.TicksPerMillisecond) % 1000);
        public DayOfWeek DayOfWeek => (DayOfWeek)(((Ticks / TimeSpan.TicksPerDay) + 1) % 7);
        public int DayOfYear => GetDatePart(1);
        public TimeSpan TimeOfDay => new TimeSpan(Ticks % TimeSpan.TicksPerDay);

        public static DateTime Now => new DateTime(Environment.TickCount64 * TimeSpan.TicksPerMillisecond);
        public static DateTime UtcNow => Now;
        public static DateTime Today => Now.Date;

        public DateTime Add(TimeSpan value) => new DateTime(Ticks + value.Ticks);
        public DateTime AddDays(double value) => Add(TimeSpan.FromDays(value));
        public DateTime AddHours(double value) => Add(TimeSpan.FromHours(value));
        public DateTime AddMinutes(double value) => Add(TimeSpan.FromMinutes(value));
        public DateTime AddSeconds(double value) => Add(TimeSpan.FromSeconds(value));
        public DateTime AddMilliseconds(double value) => Add(TimeSpan.FromMilliseconds(value));
        public DateTime AddMonths(int months) => this; // Simplified
        public DateTime AddYears(int value) => AddMonths(value * 12);
        public TimeSpan Subtract(DateTime value) => new TimeSpan(Ticks - value.Ticks);
        public DateTime Subtract(TimeSpan value) => new DateTime(Ticks - value.Ticks);

        private static long DateToTicks(int year, int month, int day) => 0; // Simplified
        private static long TimeToTicks(int hour, int minute, int second) =>
            hour * TimeSpan.TicksPerHour + minute * TimeSpan.TicksPerMinute + second * TimeSpan.TicksPerSecond;
        private int GetDatePart(int part) => 1; // Simplified

        public int CompareTo(Object value) => value is DateTime dt ? CompareTo(dt) : throw new ArgumentException();
        public int CompareTo(DateTime value) => Ticks < value.Ticks ? -1 : (Ticks > value.Ticks ? 1 : 0);
        public override bool Equals(Object value) => value is DateTime dt && Equals(dt);
        public bool Equals(DateTime value) => Ticks == value.Ticks;
        public override int GetHashCode() => (int)Ticks ^ (int)(Ticks >> 32);
        public override String ToString() => $"{Year:D4}-{Month:D2}-{Day:D2} {Hour:D2}:{Minute:D2}:{Second:D2}";

        public static DateTime operator +(DateTime d, TimeSpan t) => d.Add(t);
        public static DateTime operator -(DateTime d, TimeSpan t) => d.Subtract(t);
        public static TimeSpan operator -(DateTime d1, DateTime d2) => d1.Subtract(d2);
        public static bool operator ==(DateTime d1, DateTime d2) => d1.Ticks == d2.Ticks;
        public static bool operator !=(DateTime d1, DateTime d2) => d1.Ticks != d2.Ticks;
        public static bool operator <(DateTime t1, DateTime t2) => t1.Ticks < t2.Ticks;
        public static bool operator >(DateTime t1, DateTime t2) => t1.Ticks > t2.Ticks;
    }

    public enum DayOfWeek { Sunday = 0, Monday = 1, Tuesday = 2, Wednesday = 3, Thursday = 4, Friday = 5, Saturday = 6 }

    // Guid (ECMA-335)
    public struct Guid : IComparable, IComparable<Guid>, IEquatable<Guid>
    {
        private int _a; private short _b; private short _c;
        private byte _d; private byte _e; private byte _f; private byte _g;
        private byte _h; private byte _i; private byte _j; private byte _k;

        public static readonly Guid Empty = new Guid();

        public Guid(byte[] b)
        {
            if (b == null || b.Length != 16) throw new ArgumentException();
            _a = (b[3] << 24) | (b[2] << 16) | (b[1] << 8) | b[0];
            _b = (short)((b[5] << 8) | b[4]);
            _c = (short)((b[7] << 8) | b[6]);
            _d = b[8]; _e = b[9]; _f = b[10]; _g = b[11];
            _h = b[12]; _i = b[13]; _j = b[14]; _k = b[15];
        }

        public Guid(String g) : this() { /* Parse */ }

        public static Guid NewGuid()
        {
            var bytes = new byte[16];
            new Random().NextBytes(bytes);
            return new Guid(bytes);
        }

        public byte[] ToByteArray()
        {
            return new byte[] {
                (byte)_a, (byte)(_a >> 8), (byte)(_a >> 16), (byte)(_a >> 24),
                (byte)_b, (byte)(_b >> 8), (byte)_c, (byte)(_c >> 8),
                _d, _e, _f, _g, _h, _i, _j, _k
            };
        }

        public int CompareTo(Object value) => value is Guid g ? CompareTo(g) : throw new ArgumentException();
        public int CompareTo(Guid value) => _a != value._a ? _a.CompareTo(value._a) : 0;
        public override bool Equals(Object o) => o is Guid g && Equals(g);
        public bool Equals(Guid g) => _a == g._a && _b == g._b && _c == g._c && _d == g._d && _e == g._e && _f == g._f && _g == g._g && _h == g._h && _i == g._i && _j == g._j && _k == g._k;
        public override int GetHashCode() => _a ^ ((_b << 16) | (ushort)_c) ^ ((_f << 24) | _k);
        public override String ToString() => $"{_a:x8}-{_b:x4}-{_c:x4}-{_d:x2}{_e:x2}-{_f:x2}{_g:x2}{_h:x2}{_i:x2}{_j:x2}{_k:x2}";

        public static bool operator ==(Guid a, Guid b) => a.Equals(b);
        public static bool operator !=(Guid a, Guid b) => !a.Equals(b);
    }

    // Random (ECMA-335)
    public class Random
    {
        private int _seed;
        public Random() : this(Environment.TickCount) { }
        public Random(int seed) { _seed = seed; }
        public virtual int Next()
        {
            _seed = (_seed * 1103515245 + 12345) & 0x7FFFFFFF;
            return _seed;
        }
        public virtual int Next(int maxValue) => maxValue <= 0 ? 0 : Next() % maxValue;
        public virtual int Next(int minValue, int maxValue) => minValue + Next(maxValue - minValue);
        public virtual double NextDouble() => Next() / (double)Int32.MaxValue;
        public virtual void NextBytes(byte[] buffer)
        {
            if (buffer == null) throw new ArgumentNullException(nameof(buffer));
            for (int i = 0; i < buffer.Length; i++) buffer[i] = (byte)Next(256);
        }
    }

    // Environment (ECMA-335)
    public static class Environment
    {
        public static String NewLine => "\n";
        public static int TickCount => (int)(TickCount64 & 0x7FFFFFFF);
        public static long TickCount64 => 0; // Runtime-implemented
        public static String MachineName => "Lux9";
        public static int ProcessorCount => 1;
        public static String UserName => "kernel";
        public static String UserDomainName => "local";
        public static int CurrentManagedThreadId => Threading.Thread.CurrentThread.ManagedThreadId;
        public static bool Is64BitProcess => IntPtr.Size == 8;
        public static bool Is64BitOperatingSystem => true;
        public static String CurrentDirectory { get; set; } = "/";
        public static void Exit(int exitCode) { }
        public static void FailFast(String message) { }
        public static void FailFast(String message, Exception exception) { }
        public static String GetEnvironmentVariable(String variable) => null;
        public static void SetEnvironmentVariable(String variable, String value) { }
        public static String[] GetCommandLineArgs() => Array.Empty<String>();
    }

    // IAsyncResult (ECMA-335)
    public interface IAsyncResult
    {
        Object AsyncState { get; }
        Threading.WaitHandle AsyncWaitHandle { get; }
        bool CompletedSynchronously { get; }
        bool IsCompleted { get; }
    }

    // DBNull
    public sealed class DBNull
    {
        public static readonly DBNull Value = new DBNull();
        private DBNull() { }
        public override String ToString() => String.Empty;
    }

    // Convert - simplified (ECMA-335)
    public static class Convert
    {
        public static bool ToBoolean(Object value) => value is bool b ? b : false;
        public static int ToInt32(Object value) => value is int i ? i : 0;
        public static long ToInt64(Object value) => value is long l ? l : 0;
        public static double ToDouble(Object value) => value is double d ? d : 0;
        public static String ToString(Object value) => value?.ToString() ?? String.Empty;
        public static byte[] FromBase64String(String s) => Array.Empty<byte>(); // Simplified
        public static String ToBase64String(byte[] inArray) => String.Empty; // Simplified
    }

    // BitConverter (ECMA-335)
    public static class BitConverter
    {
        public static bool IsLittleEndian => true;
        public static unsafe int SingleToInt32Bits(float value)
        {
            return *((int*)&value);
        }
        public static unsafe float Int32BitsToSingle(int value)
        {
            return *((float*)&value);
        }
        public static unsafe long DoubleToInt64Bits(double value)
        {
            return *((long*)&value);
        }
        public static unsafe double Int64BitsToDouble(long value)
        {
            return *((double*)&value);
        }
        public static byte[] GetBytes(int value) => new byte[] { (byte)value, (byte)(value >> 8), (byte)(value >> 16), (byte)(value >> 24) };
        public static byte[] GetBytes(long value) => new byte[] { (byte)value, (byte)(value >> 8), (byte)(value >> 16), (byte)(value >> 24), (byte)(value >> 32), (byte)(value >> 40), (byte)(value >> 48), (byte)(value >> 56) };
        public static int ToInt32(byte[] value, int startIndex) => value[startIndex] | (value[startIndex + 1] << 8) | (value[startIndex + 2] << 16) | (value[startIndex + 3] << 24);
        public static long ToInt64(byte[] value, int startIndex) => (long)ToInt32(value, startIndex) | ((long)ToInt32(value, startIndex + 4) << 32);
    }

    // Number formatting helpers - internal
    internal static class Number
    {
        public static String FormatInt32(int value, String format, IFormatProvider provider)
        {
            if (value == 0) return "0";
            bool negative = value < 0;
            if (negative) value = -value;
            char[] buf = new char[11];
            int i = 10;
            while (value > 0) { buf[i--] = (char)('0' + value % 10); value /= 10; }
            if (negative) buf[i--] = '-';
            return new String(buf, i + 1, 10 - i);
        }
        public static String FormatUInt32(uint value, String format, IFormatProvider provider)
        {
            if (value == 0) return "0";
            char[] buf = new char[10];
            int i = 9;
            while (value > 0) { buf[i--] = (char)('0' + value % 10); value /= 10; }
            return new String(buf, i + 1, 9 - i);
        }
        public static String FormatInt64(long value, String format, IFormatProvider provider) => value.ToString();
        public static String FormatUInt64(ulong value, String format, IFormatProvider provider) => value.ToString();
        public static String FormatSingle(float value, String format, IFormatProvider provider) => value.ToString();
        public static String FormatDouble(double value, String format, IFormatProvider provider) => value.ToString();
        public static int ParseInt32(String s) => 0; // Simplified
    }
}
