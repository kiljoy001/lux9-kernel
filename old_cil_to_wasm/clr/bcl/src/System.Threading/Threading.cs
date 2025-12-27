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

        public static T CompareExchange<T>(ref T location1, T value, T comparand) where T : class
        {
            // Simple unsafe cast wrapper - assumes layout compatibility (which is true for classes)
            // Ideally should be Intrinsic/InternalCall
            // But we can't easily do ref T -> ref object in C# without Unsafe.
            // Let's declare it as InternalCall for now to satisfy complier.
            return CompareExchangeInternal<T>(ref location1, value, comparand);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern T CompareExchangeInternal<T>(ref T location1, T value, T comparand) where T : class;
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


