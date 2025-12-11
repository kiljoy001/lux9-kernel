namespace System
{
    public struct DateTime : IComparable, IComparable<DateTime>, IEquatable<DateTime>
    {
        private long _ticks;

        public DateTime(long ticks) { _ticks = ticks; }
        public DateTime(int year, int month, int day) { _ticks = 0; } // Stub

        public static DateTime Now => new DateTime(0);
        public static DateTime UtcNow => new DateTime(0);

        public int Year => 2025;
        public int Month => 1;
        public int Day => 1;

        public long Ticks => _ticks;

        public static bool operator ==(DateTime d1, DateTime d2) => d1._ticks == d2._ticks;
        public static bool operator !=(DateTime d1, DateTime d2) => d1._ticks != d2._ticks;
        
        public override string ToString() => "DateTime";
        public int CompareTo(object obj) => 0;
        public int CompareTo(DateTime other) => _ticks < other._ticks ? -1 : (_ticks > other._ticks ? 1 : 0);
        public bool Equals(DateTime other) => _ticks == other._ticks;
        public override bool Equals(object obj) => obj is DateTime dt && Equals(dt);
        public override int GetHashCode() => _ticks.GetHashCode();
    }
}
