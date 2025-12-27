namespace System
{
    public struct DateTime : IComparable, IComparable<DateTime>, IEquatable<DateTime>
    {
        // Ticks = 100-nanosecond intervals since 0001-01-01 00:00:00 UTC
        private long _ticks;
        
        // Constants
        private const long TicksPerMillisecond = 10000;
        private const long TicksPerSecond = TicksPerMillisecond * 1000;
        private const long TicksPerMinute = TicksPerSecond * 60;
        private const long TicksPerHour = TicksPerMinute * 60;
        private const long TicksPerDay = TicksPerHour * 24;
        
        private const int DaysPerYear = 365;
        private const int DaysPer4Years = DaysPerYear * 4 + 1;       // 1461
        private const int DaysPer100Years = DaysPer4Years * 25 - 1; // 36524
        private const int DaysPer400Years = DaysPer100Years * 4 + 1; // 146097
        
        public const long MinTicks = 0;
        public const long MaxTicks = 3155378975999999999L;
        
        private static readonly int[] DaysToMonth365 = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
        private static readonly int[] DaysToMonth366 = { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 };
        
        public static readonly DateTime MinValue = new DateTime(MinTicks);
        public static readonly DateTime MaxValue = new DateTime(MaxTicks);
        
        public DateTime(long ticks)
        {
            if (ticks < MinTicks || ticks > MaxTicks)
                throw new ArgumentOutOfRangeException("ticks");
            _ticks = ticks;
        }
        
        public DateTime(int year, int month, int day)
            : this(year, month, day, 0, 0, 0, 0) { }
        
        public DateTime(int year, int month, int day, int hour, int minute, int second)
            : this(year, month, day, hour, minute, second, 0) { }
        
        public DateTime(int year, int month, int day, int hour, int minute, int second, int millisecond)
        {
            if (year < 1 || year > 9999) throw new ArgumentOutOfRangeException("year");
            if (month < 1 || month > 12) throw new ArgumentOutOfRangeException("month");
            
            int[] days = IsLeapYear(year) ? DaysToMonth366 : DaysToMonth365;
            int maxDay = days[month] - days[month - 1];
            if (day < 1 || day > maxDay) throw new ArgumentOutOfRangeException("day");
            if (hour < 0 || hour > 23) throw new ArgumentOutOfRangeException("hour");
            if (minute < 0 || minute > 59) throw new ArgumentOutOfRangeException("minute");
            if (second < 0 || second > 59) throw new ArgumentOutOfRangeException("second");
            if (millisecond < 0 || millisecond > 999) throw new ArgumentOutOfRangeException("millisecond");
            
            long totalDays = DateToTicks(year, month, day);
            _ticks = totalDays + TimeToTicks(hour, minute, second) + millisecond * TicksPerMillisecond;
        }
        
        private static long DateToTicks(int year, int month, int day)
        {
            int[] days = IsLeapYear(year) ? DaysToMonth366 : DaysToMonth365;
            int y = year - 1;
            int n = y * 365 + y / 4 - y / 100 + y / 400 + days[month - 1] + day - 1;
            return n * TicksPerDay;
        }
        
        private static long TimeToTicks(int hour, int minute, int second)
        {
            return hour * TicksPerHour + minute * TicksPerMinute + second * TicksPerSecond;
        }
        
        public static bool IsLeapYear(int year)
        {
            if (year < 1 || year > 9999) throw new ArgumentOutOfRangeException("year");
            return (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0));
        }
        
        // Properties
        public long Ticks => _ticks;
        
        public int Millisecond => (int)((_ticks / TicksPerMillisecond) % 1000);
        public int Second => (int)((_ticks / TicksPerSecond) % 60);
        public int Minute => (int)((_ticks / TicksPerMinute) % 60);
        public int Hour => (int)((_ticks / TicksPerHour) % 24);
        
        public int Year => GetDatePart(0);
        public int Month => GetDatePart(1);
        public int Day => GetDatePart(2);
        public int DayOfYear => GetDatePart(3);
        public DayOfWeek DayOfWeek => (DayOfWeek)((int)((_ticks / TicksPerDay + 1) % 7));
        
        public DateTime Date => new DateTime(_ticks - _ticks % TicksPerDay);
        public TimeSpan TimeOfDay => new TimeSpan(_ticks % TicksPerDay);
        
        private int GetDatePart(int part)
        {
            int n = (int)(_ticks / TicksPerDay);
            int y400 = n / DaysPer400Years;
            n -= y400 * DaysPer400Years;
            
            int y100 = n / DaysPer100Years;
            if (y100 == 4) y100 = 3;
            n -= y100 * DaysPer100Years;
            
            int y4 = n / DaysPer4Years;
            n -= y4 * DaysPer4Years;
            
            int y1 = n / DaysPerYear;
            if (y1 == 4) y1 = 3;
            
            if (part == 0) // Year
                return y400 * 400 + y100 * 100 + y4 * 4 + y1 + 1;
            
            n -= y1 * DaysPerYear;
            if (part == 3) // DayOfYear
                return n + 1;
            
            int year = y400 * 400 + y100 * 100 + y4 * 4 + y1 + 1;
            int[] days = IsLeapYear(year) ? DaysToMonth366 : DaysToMonth365;
            int m = (n >> 5) + 1;
            while (n >= days[m]) m++;
            
            if (part == 1) // Month
                return m;
            
            return n - days[m - 1] + 1; // Day
        }
        
        // Now - uses kernel fastticks via InternalCall
        public static DateTime Now
        {
            get
            {
                long ticks = Internal_GetNow();
                return new DateTime(ticks);
            }
        }
        
        [System.Runtime.CompilerServices.MethodImpl(
            System.Runtime.CompilerServices.MethodImplOptions.InternalCall)]
        private static extern long Internal_GetNow();
        
        public static DateTime UtcNow => Now; // Same as Now for kernel (no timezone)
        
        public static DateTime Today => Now.Date;
        
        // Add methods
        public DateTime Add(TimeSpan value) => new DateTime(_ticks + value.Ticks);
        public DateTime AddTicks(long value) => new DateTime(_ticks + value);
        public DateTime AddMilliseconds(double value) => new DateTime(_ticks + (long)(value * TicksPerMillisecond));
        public DateTime AddSeconds(double value) => new DateTime(_ticks + (long)(value * TicksPerSecond));
        public DateTime AddMinutes(double value) => new DateTime(_ticks + (long)(value * TicksPerMinute));
        public DateTime AddHours(double value) => new DateTime(_ticks + (long)(value * TicksPerHour));
        public DateTime AddDays(double value) => new DateTime(_ticks + (long)(value * TicksPerDay));
        
        public DateTime AddMonths(int months)
        {
            int y = Year;
            int m = Month;
            int d = Day;
            
            m += months;
            while (m > 12) { m -= 12; y++; }
            while (m < 1) { m += 12; y--; }
            
            int[] days = IsLeapYear(y) ? DaysToMonth366 : DaysToMonth365;
            int maxDay = days[m] - days[m - 1];
            if (d > maxDay) d = maxDay;
            
            return new DateTime(y, m, d, Hour, Minute, Second, Millisecond);
        }
        
        public DateTime AddYears(int value) => AddMonths(value * 12);
        
        public TimeSpan Subtract(DateTime value) => new TimeSpan(_ticks - value._ticks);
        public DateTime Subtract(TimeSpan value) => new DateTime(_ticks - value.Ticks);
        
        // Operators
        public static DateTime operator +(DateTime d, TimeSpan t) => d.Add(t);
        public static DateTime operator -(DateTime d, TimeSpan t) => d.Subtract(t);
        public static TimeSpan operator -(DateTime d1, DateTime d2) => d1.Subtract(d2);
        
        public static bool operator ==(DateTime d1, DateTime d2) => d1._ticks == d2._ticks;
        public static bool operator !=(DateTime d1, DateTime d2) => d1._ticks != d2._ticks;
        public static bool operator <(DateTime d1, DateTime d2) => d1._ticks < d2._ticks;
        public static bool operator >(DateTime d1, DateTime d2) => d1._ticks > d2._ticks;
        public static bool operator <=(DateTime d1, DateTime d2) => d1._ticks <= d2._ticks;
        public static bool operator >=(DateTime d1, DateTime d2) => d1._ticks >= d2._ticks;
        
        // ToString - simplified to avoid String.Concat bootstrap issues
        public override string ToString()
        {
            // Use explicit Concat calls instead of + operator
            return String.Concat(
                Pad(Year, 4), "-", Pad(Month, 2), "-", Pad(Day, 2), " ",
                Pad(Hour, 2), ":", Pad(Minute, 2), ":", Pad(Second, 2)
            );
        }
        
        private static string Pad(int value, int width)
        {
            string s = value.ToString();
            while (s.Length < width) s = String.Concat("0", s);
            return s;
        }
        
        public string ToString(string format)
        {
            // All format handling simplified to avoid + operator
            if (format == null) return ToString();
            if (format == "d") return String.Concat(Month.ToString(), "/", Day.ToString(), "/", Year.ToString());
            if (format == "D") return String.Concat(DayOfWeek.ToString(), ", ", Month.ToString(), " ", Day.ToString(), ", ", Year.ToString());
            if (format == "t") return String.Concat(Pad(Hour, 2), ":", Pad(Minute, 2));
            if (format == "T") return String.Concat(Pad(Hour, 2), ":", Pad(Minute, 2), ":", Pad(Second, 2));
            if (format == "s") return String.Concat(
                Pad(Year, 4), "-", Pad(Month, 2), "-", Pad(Day, 2), "T",
                Pad(Hour, 2), ":", Pad(Minute, 2), ":", Pad(Second, 2)
            );
            return ToString();
        }
        
        // Comparison
        public int CompareTo(object obj)
        {
            if (obj == null) return 1;
            if (obj is DateTime dt) return CompareTo(dt);
            throw new ArgumentException("Object must be DateTime");
        }
        
        public int CompareTo(DateTime other) => _ticks < other._ticks ? -1 : (_ticks > other._ticks ? 1 : 0);
        
        public bool Equals(DateTime other) => _ticks == other._ticks;
        public override bool Equals(object obj) => obj is DateTime dt && Equals(dt);
        public override int GetHashCode() => _ticks.GetHashCode();
        
        // Parse (simplified)
        public static DateTime Parse(string s)
        {
            if (string.IsNullOrEmpty(s))
                throw new FormatException("Invalid DateTime string");
            
            // Try ISO format: YYYY-MM-DD or YYYY-MM-DDTHH:MM:SS
            string[] parts = s.Split('T');
            string[] dateParts = parts[0].Split('-');
            
            if (dateParts.Length != 3)
                throw new FormatException("Invalid date format");
            
            int year = int.Parse(dateParts[0]);
            int month = int.Parse(dateParts[1]);
            int day = int.Parse(dateParts[2]);
            
            if (parts.Length > 1)
            {
                string[] timeParts = parts[1].Split(':');
                int hour = timeParts.Length > 0 ? int.Parse(timeParts[0]) : 0;
                int minute = timeParts.Length > 1 ? int.Parse(timeParts[1]) : 0;
                int second = timeParts.Length > 2 ? int.Parse(timeParts[2].Split('.')[0]) : 0;
                return new DateTime(year, month, day, hour, minute, second);
            }
            
            return new DateTime(year, month, day);
        }
        
        public static bool TryParse(string s, out DateTime result)
        {
            try
            {
                result = Parse(s);
                return true;
            }
            catch
            {
                result = MinValue;
                return false;
            }
        }
    }
    
    public enum DayOfWeek
    {
        Sunday = 0,
        Monday = 1,
        Tuesday = 2,
        Wednesday = 3,
        Thursday = 4,
        Friday = 5,
        Saturday = 6
    }
    
    public struct TimeSpan : IComparable, IComparable<TimeSpan>, IEquatable<TimeSpan>
    {
        private long _ticks;
        
        public const long TicksPerMillisecond = 10000;
        public const long TicksPerSecond = TicksPerMillisecond * 1000;
        public const long TicksPerMinute = TicksPerSecond * 60;
        public const long TicksPerHour = TicksPerMinute * 60;
        public const long TicksPerDay = TicksPerHour * 24;
        
        public static readonly TimeSpan Zero = new TimeSpan(0);
        public static readonly TimeSpan MinValue = new TimeSpan(long.MinValue);
        public static readonly TimeSpan MaxValue = new TimeSpan(long.MaxValue);
        
        public TimeSpan(long ticks) { _ticks = ticks; }
        public TimeSpan(int hours, int minutes, int seconds)
            : this(0, hours, minutes, seconds, 0) { }
        public TimeSpan(int days, int hours, int minutes, int seconds)
            : this(days, hours, minutes, seconds, 0) { }
        public TimeSpan(int days, int hours, int minutes, int seconds, int milliseconds)
        {
            _ticks = days * TicksPerDay + hours * TicksPerHour + minutes * TicksPerMinute
                   + seconds * TicksPerSecond + milliseconds * TicksPerMillisecond;
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
        public TimeSpan Duration() => new TimeSpan(_ticks < 0 ? -_ticks : _ticks);
        
        public static TimeSpan FromDays(double value) => new TimeSpan((long)(value * TicksPerDay));
        public static TimeSpan FromHours(double value) => new TimeSpan((long)(value * TicksPerHour));
        public static TimeSpan FromMinutes(double value) => new TimeSpan((long)(value * TicksPerMinute));
        public static TimeSpan FromSeconds(double value) => new TimeSpan((long)(value * TicksPerSecond));
        public static TimeSpan FromMilliseconds(double value) => new TimeSpan((long)(value * TicksPerMillisecond));
        public static TimeSpan FromTicks(long value) => new TimeSpan(value);
        
        public static TimeSpan operator +(TimeSpan t1, TimeSpan t2) => t1.Add(t2);
        public static TimeSpan operator -(TimeSpan t1, TimeSpan t2) => t1.Subtract(t2);
        public static TimeSpan operator -(TimeSpan t) => t.Negate();
        
        public static bool operator ==(TimeSpan t1, TimeSpan t2) => t1._ticks == t2._ticks;
        public static bool operator !=(TimeSpan t1, TimeSpan t2) => t1._ticks != t2._ticks;
        public static bool operator <(TimeSpan t1, TimeSpan t2) => t1._ticks < t2._ticks;
        public static bool operator >(TimeSpan t1, TimeSpan t2) => t1._ticks > t2._ticks;
        public static bool operator <=(TimeSpan t1, TimeSpan t2) => t1._ticks <= t2._ticks;
        public static bool operator >=(TimeSpan t1, TimeSpan t2) => t1._ticks >= t2._ticks;
        
        public override string ToString()
        {
            string time = String.Concat(Pad(Hours, 2), ":", Pad(Minutes, 2), ":", Pad(Seconds, 2));
            if (Days != 0)
                return String.Concat(Days.ToString(), ".", time);
            return time;
        }
        
        private static string Pad(int value, int width)
        {
            string s = value.ToString();
            while (s.Length < width) s = String.Concat("0", s);
            return s;
        }
        
        public int CompareTo(object obj)
        {
            if (obj == null) return 1;
            if (obj is TimeSpan ts) return CompareTo(ts);
            throw new ArgumentException("Object must be TimeSpan");
        }
        
        public int CompareTo(TimeSpan other) => _ticks < other._ticks ? -1 : (_ticks > other._ticks ? 1 : 0);
        public bool Equals(TimeSpan other) => _ticks == other._ticks;
        public override bool Equals(object obj) => obj is TimeSpan ts && Equals(ts);
        public override int GetHashCode() => _ticks.GetHashCode();
    }
}
