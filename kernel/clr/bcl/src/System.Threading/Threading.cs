/*
 * System.Threading - Basic threading primitives
 * Lux9 CLR Base Class Library
 */
namespace System.Threading
{
    using System;
    using System.Runtime.CompilerServices;

    /// <summary>
    /// Provides atomic operations for variables shared by multiple threads.
    /// </summary>
    public static class Interlocked
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern int Increment(ref int location);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern int Decrement(ref int location);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern int Exchange(ref int location1, int value);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern int CompareExchange(ref int location1, int value, int comparand);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern long Increment(ref long location);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern long Decrement(ref long location);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern object Exchange(ref object location1, object value);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern object CompareExchange(ref object location1, object value, object comparand);
    }

    /// <summary>
    /// Provides a mechanism that synchronizes access to objects.
    /// </summary>
    public static class Monitor
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Enter(object obj);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Exit(object obj);
        
        public static bool TryEnter(object obj)
        {
            return TryEnter(obj, 0);
        }
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool TryEnter(object obj, int millisecondsTimeout);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Pulse(object obj);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void PulseAll(object obj);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool Wait(object obj, int millisecondsTimeout);
        
        public static bool Wait(object obj)
        {
            return Wait(obj, -1);
        }
    }

    /// <summary>
    /// Represents a thread of execution.
    /// </summary>
    public sealed class Thread
    {
        private int _managedThreadId;
        private ThreadState _threadState;
        private string _name;
        private static int _nextThreadId = 1;
        
        private ThreadStart _start;
        private ParameterizedThreadStart _paramStart;
        
        public Thread(ThreadStart start)
        {
            if (start == null) throw new ArgumentNullException(nameof(start));
            _start = start;
            _managedThreadId = Interlocked.Increment(ref _nextThreadId);
            _threadState = ThreadState.Unstarted;
        }
        
        public Thread(ParameterizedThreadStart start)
        {
            if (start == null) throw new ArgumentNullException(nameof(start));
            _paramStart = start;
            _managedThreadId = Interlocked.Increment(ref _nextThreadId);
            _threadState = ThreadState.Unstarted;
        }
        
        public int ManagedThreadId => _managedThreadId;
        
        public string Name
        {
            get => _name;
            set => _name = value;
        }
        
        public ThreadState ThreadState => _threadState;
        
        public bool IsAlive => (_threadState & ThreadState.Stopped) == 0 && 
                               (_threadState & ThreadState.Unstarted) == 0;
        
        public bool IsBackground { get; set; }
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern void Start();
        
        public void Start(object parameter)
        {
            // Store parameter and call internal start
            StartInternal(parameter);
        }
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern void StartInternal(object parameter);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern void Join();
        
        public bool Join(int millisecondsTimeout)
        {
            return JoinInternal(millisecondsTimeout);
        }
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern bool JoinInternal(int millisecondsTimeout);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern void Abort();
        
        public static Thread CurrentThread
        {
            [MethodImpl(MethodImplOptions.InternalCall)]
            get;
        }
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Sleep(int millisecondsTimeout);
        
        public static void Sleep(TimeSpan timeout)
        {
            Sleep((int)timeout.TotalMilliseconds);
        }
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SpinWait(int iterations);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool Yield();
    }
    
    public delegate void ThreadStart();
    public delegate void ParameterizedThreadStart(object obj);
    
    [Flags]
    public enum ThreadState
    {
        Running = 0,
        StopRequested = 1,
        SuspendRequested = 2,
        Background = 4,
        Unstarted = 8,
        Stopped = 16,
        WaitSleepJoin = 32,
        Suspended = 64,
        AbortRequested = 128,
        Aborted = 256
    }

    /// <summary>
    /// A lightweight synchronization primitive.
    /// </summary>
    public struct SpinLock
    {
        private volatile int _owner;
        
        public void Enter(ref bool lockTaken)
        {
            if (lockTaken) throw new ArgumentException("Lock already taken", nameof(lockTaken));
            
            while (Interlocked.CompareExchange(ref _owner, 1, 0) != 0)
            {
                Thread.SpinWait(1);
            }
            lockTaken = true;
        }
        
        public void Exit()
        {
            _owner = 0;
        }
        
        public bool IsHeld => _owner != 0;
    }

    /// <summary>
    /// A synchronization primitive that signals when its count reaches zero.
    /// </summary>
    public class CountdownEvent : IDisposable
    {
        private int _currentCount;
        private readonly int _initialCount;
        private volatile bool _disposed;
        
        public CountdownEvent(int initialCount)
        {
            if (initialCount < 0) throw new ArgumentOutOfRangeException(nameof(initialCount));
            _currentCount = initialCount;
            _initialCount = initialCount;
        }
        
        public int CurrentCount => _currentCount;
        public int InitialCount => _initialCount;
        public bool IsSet => _currentCount == 0;
        
        public bool Signal()
        {
            int newCount = Interlocked.Decrement(ref _currentCount);
            if (newCount < 0) throw new InvalidOperationException("Count already zero");
            return newCount == 0;
        }
        
        public bool Signal(int signalCount)
        {
            for (int i = 0; i < signalCount; i++) Signal();
            return IsSet;
        }
        
        public void AddCount() => AddCount(1);
        
        public void AddCount(int signalCount)
        {
            if (signalCount <= 0) throw new ArgumentOutOfRangeException(nameof(signalCount));
            Interlocked.Increment(ref _currentCount);
        }
        
        public void Reset() => Reset(_initialCount);
        
        public void Reset(int count)
        {
            if (count < 0) throw new ArgumentOutOfRangeException(nameof(count));
            _currentCount = count;
        }
        
        public void Wait()
        {
            while (_currentCount > 0)
            {
                Thread.SpinWait(1);
            }
        }
        
        public void Dispose()
        {
            _disposed = true;
        }
    }

    /// <summary>
    /// Represents a lock that is used to manage read/write access.
    /// </summary>
    public class ReaderWriterLockSlim : IDisposable
    {
        private int _readers;
        private int _writers;
        private int _writeRequests;
        
        public void EnterReadLock()
        {
            while (true)
            {
                while (_writers > 0 || _writeRequests > 0)
                    Thread.SpinWait(1);
                
                Interlocked.Increment(ref _readers);
                
                if (_writers == 0) break;
                
                Interlocked.Decrement(ref _readers);
            }
        }
        
        public void ExitReadLock()
        {
            Interlocked.Decrement(ref _readers);
        }
        
        public void EnterWriteLock()
        {
            Interlocked.Increment(ref _writeRequests);
            
            while (Interlocked.CompareExchange(ref _writers, 1, 0) != 0)
                Thread.SpinWait(1);
            
            while (_readers > 0)
                Thread.SpinWait(1);
            
            Interlocked.Decrement(ref _writeRequests);
        }
        
        public void ExitWriteLock()
        {
            _writers = 0;
        }
        
        public bool IsReadLockHeld => _readers > 0;
        public bool IsWriteLockHeld => _writers > 0;
        
        public void Dispose() { }
    }

    /// <summary>
    /// Provides lazy initialization.
    /// </summary>
    public sealed class LazyInitializer
    {
        public static T EnsureInitialized<T>(ref T target) where T : class, new()
        {
            if (target == null)
            {
                T newValue = new T();
                Interlocked.CompareExchange(ref target, newValue, null);
            }
            return target;
        }
        
        public static T EnsureInitialized<T>(ref T target, Func<T> valueFactory) where T : class
        {
            if (target == null)
            {
                T newValue = valueFactory();
                Interlocked.CompareExchange(ref target, newValue, null);
            }
            return target;
        }
    }

    /// <summary>
    /// Signals across threads that an event has occurred.
    /// </summary>
    public class ManualResetEvent : WaitHandle
    {
        private volatile bool _signaled;
        
        public ManualResetEvent(bool initialState)
        {
            _signaled = initialState;
        }
        
        public bool Set()
        {
            _signaled = true;
            return true;
        }
        
        public bool Reset()
        {
            _signaled = false;
            return true;
        }
        
        public override bool WaitOne()
        {
            while (!_signaled)
                Thread.SpinWait(1);
            return true;
        }
        
        public override bool WaitOne(int millisecondsTimeout)
        {
            // Simplified: doesn't actually timeout
            return WaitOne();
        }
    }

    /// <summary>
    /// Signals across threads that an event has occurred (auto-reset).
    /// </summary>
    public class AutoResetEvent : WaitHandle
    {
        private volatile bool _signaled;
        
        public AutoResetEvent(bool initialState)
        {
            _signaled = initialState;
        }
        
        public bool Set()
        {
            _signaled = true;
            return true;
        }
        
        public override bool WaitOne()
        {
            while (!_signaled)
                Thread.SpinWait(1);
            _signaled = false;
            return true;
        }
        
        public override bool WaitOne(int millisecondsTimeout)
        {
            return WaitOne();
        }
    }

    /// <summary>
    /// Base class for wait handles.
    /// </summary>
    public abstract class WaitHandle : IDisposable
    {
        public abstract bool WaitOne();
        public abstract bool WaitOne(int millisecondsTimeout);
        
        public virtual void Close() { }
        public void Dispose() => Close();
    }

    /// <summary>
    /// Limits the number of threads that can access a resource.
    /// </summary>
    public class SemaphoreSlim : IDisposable
    {
        private volatile int _currentCount;
        private readonly int _maxCount;
        
        public SemaphoreSlim(int initialCount) : this(initialCount, int.MaxValue) { }
        
        public SemaphoreSlim(int initialCount, int maxCount)
        {
            if (initialCount < 0) throw new ArgumentOutOfRangeException(nameof(initialCount));
            if (maxCount <= 0) throw new ArgumentOutOfRangeException(nameof(maxCount));
            if (initialCount > maxCount) throw new ArgumentOutOfRangeException(nameof(initialCount));
            
            _currentCount = initialCount;
            _maxCount = maxCount;
        }
        
        public int CurrentCount => _currentCount;
        
        public void Wait()
        {
            while (true)
            {
                int current = _currentCount;
                if (current > 0 && Interlocked.CompareExchange(ref _currentCount, current - 1, current) == current)
                    return;
                Thread.SpinWait(1);
            }
        }
        
        public int Release() => Release(1);
        
        public int Release(int releaseCount)
        {
            if (releaseCount < 1) throw new ArgumentOutOfRangeException(nameof(releaseCount));
            
            int oldCount = _currentCount;
            Interlocked.Increment(ref _currentCount);
            return oldCount;
        }
        
        public void Dispose() { }
    }
}

namespace System
{
    /// <summary>
    /// Represents a time interval.
    /// </summary>
    public readonly struct TimeSpan : IComparable<TimeSpan>, IEquatable<TimeSpan>
    {
        public const long TicksPerMillisecond = 10000;
        public const long TicksPerSecond = TicksPerMillisecond * 1000;
        public const long TicksPerMinute = TicksPerSecond * 60;
        public const long TicksPerHour = TicksPerMinute * 60;
        public const long TicksPerDay = TicksPerHour * 24;
        
        public static readonly TimeSpan Zero = new TimeSpan(0);
        public static readonly TimeSpan MinValue = new TimeSpan(long.MinValue);
        public static readonly TimeSpan MaxValue = new TimeSpan(long.MaxValue);
        
        private readonly long _ticks;
        
        public TimeSpan(long ticks)
        {
            _ticks = ticks;
        }
        
        public TimeSpan(int hours, int minutes, int seconds)
        {
            _ticks = hours * TicksPerHour + minutes * TicksPerMinute + seconds * TicksPerSecond;
        }
        
        public TimeSpan(int days, int hours, int minutes, int seconds)
            : this(days, hours, minutes, seconds, 0)
        {
        }
        
        public TimeSpan(int days, int hours, int minutes, int seconds, int milliseconds)
        {
            _ticks = days * TicksPerDay + hours * TicksPerHour + 
                     minutes * TicksPerMinute + seconds * TicksPerSecond +
                     milliseconds * TicksPerMillisecond;
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
        
        public static TimeSpan operator +(TimeSpan t1, TimeSpan t2) => t1.Add(t2);
        public static TimeSpan operator -(TimeSpan t1, TimeSpan t2) => t1.Subtract(t2);
        public static TimeSpan operator -(TimeSpan t) => t.Negate();
        public static bool operator ==(TimeSpan t1, TimeSpan t2) => t1._ticks == t2._ticks;
        public static bool operator !=(TimeSpan t1, TimeSpan t2) => t1._ticks != t2._ticks;
        public static bool operator <(TimeSpan t1, TimeSpan t2) => t1._ticks < t2._ticks;
        public static bool operator <=(TimeSpan t1, TimeSpan t2) => t1._ticks <= t2._ticks;
        public static bool operator >(TimeSpan t1, TimeSpan t2) => t1._ticks > t2._ticks;
        public static bool operator >=(TimeSpan t1, TimeSpan t2) => t1._ticks >= t2._ticks;
        
        public int CompareTo(TimeSpan other) => _ticks.CompareTo(other._ticks);
        public bool Equals(TimeSpan other) => _ticks == other._ticks;
        public override bool Equals(object obj) => obj is TimeSpan ts && Equals(ts);
        public override int GetHashCode() => _ticks.GetHashCode();
        
        public override string ToString()
        {
            // Simplified format: d.hh:mm:ss
            if (Days != 0)
                return $"{Days}.{Hours:D2}:{Minutes:D2}:{Seconds:D2}";
            return $"{Hours:D2}:{Minutes:D2}:{Seconds:D2}";
        }
    }
}
