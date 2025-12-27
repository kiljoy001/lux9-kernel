/*
 * ECMA-335 Threading Types - Lux9 BCL
 * 
 * Threading primitives per CLI specification.
 */
namespace System.Threading
{
    using System;
    using System.Collections.Generic;

    // Thread - simplified for kernel
    public sealed class Thread
    {
        private static Thread _currentThread;
        public static Thread CurrentThread => _currentThread ??= new Thread();
        public int ManagedThreadId => 1;
        public String Name { get; set; }
        public bool IsBackground { get; set; }
        public bool IsAlive => true;
        public bool IsThreadPoolThread => false;
        public static void Sleep(int millisecondsTimeout) { }
        public static void Sleep(TimeSpan timeout) => Sleep((int)timeout.TotalMilliseconds);
        public void Start() { }
        public void Start(Object parameter) { }
        public void Join() { }
        public bool Join(int millisecondsTimeout) => true;
        public void Abort() { }
    }

    // Monitor (ECMA-335)
    public static class Monitor
    {
        public static void Enter(Object obj) { }
        public static void Enter(Object obj, ref bool lockTaken) { lockTaken = true; }
        public static bool TryEnter(Object obj) => true;
        public static bool TryEnter(Object obj, int millisecondsTimeout) => true;
        public static void Exit(Object obj) { }
        public static void Pulse(Object obj) { }
        public static void PulseAll(Object obj) { }
        public static bool Wait(Object obj) => true;
        public static bool Wait(Object obj, int millisecondsTimeout) => true;
    }

    // Interlocked (ECMA-335)
    public static class Interlocked
    {
        public static int Increment(ref int location) => ++location;
        public static long Increment(ref long location) => ++location;
        public static int Decrement(ref int location) => --location;
        public static long Decrement(ref long location) => --location;
        public static int Add(ref int location1, int value) { location1 += value; return location1; }
        public static long Add(ref long location1, long value) { location1 += value; return location1; }
        public static int Exchange(ref int location1, int value) { int orig = location1; location1 = value; return orig; }
        public static long Exchange(ref long location1, long value) { long orig = location1; location1 = value; return orig; }
        public static Object Exchange(ref Object location1, Object value) { Object orig = location1; location1 = value; return orig; }
        public static T Exchange<T>(ref T location1, T value) where T : class { T orig = location1; location1 = value; return orig; }
        public static int CompareExchange(ref int location1, int value, int comparand)
        {
            int orig = location1;
            if (location1 == comparand) location1 = value;
            return orig;
        }
        public static long CompareExchange(ref long location1, long value, long comparand)
        {
            long orig = location1;
            if (location1 == comparand) location1 = value;
            return orig;
        }
        public static Object CompareExchange(ref Object location1, Object value, Object comparand)
        {
            Object orig = location1;
            if (location1 == comparand) location1 = value;
            return orig;
        }
        public static T CompareExchange<T>(ref T location1, T value, T comparand) where T : class
        {
            T orig = location1;
            if (ReferenceEquals(location1, comparand)) location1 = value;
            return orig;
        }
        public static void MemoryBarrier() { }
        public static int Read(ref int location) => location;
        public static long Read(ref long location) => location;
    }

    // WaitHandle (ECMA-335)
    public abstract class WaitHandle : IDisposable
    {
        public const int WaitTimeout = 258;
        protected WaitHandle() { }
        public virtual void Close() => Dispose();
        public virtual void Dispose() { }
        public virtual bool WaitOne() => WaitOne(-1);
        public virtual bool WaitOne(int millisecondsTimeout) => true;
        public virtual bool WaitOne(TimeSpan timeout) => WaitOne((int)timeout.TotalMilliseconds);
        public static bool WaitAll(WaitHandle[] waitHandles) => true;
        public static bool WaitAll(WaitHandle[] waitHandles, int millisecondsTimeout) => true;
        public static int WaitAny(WaitHandle[] waitHandles) => 0;
        public static int WaitAny(WaitHandle[] waitHandles, int millisecondsTimeout) => 0;
        public static bool SignalAndWait(WaitHandle toSignal, WaitHandle toWaitOn) => true;
    }

    // ManualResetEvent (ECMA-335)
    public sealed class ManualResetEvent : WaitHandle
    {
        private bool _state;
        public ManualResetEvent(bool initialState) { _state = initialState; }
        public bool Set() { _state = true; return true; }
        public bool Reset() { _state = false; return true; }
        public override bool WaitOne(int millisecondsTimeout) => _state;
    }

    // AutoResetEvent (ECMA-335)
    public sealed class AutoResetEvent : WaitHandle
    {
        private bool _state;
        public AutoResetEvent(bool initialState) { _state = initialState; }
        public bool Set() { _state = true; return true; }
        public bool Reset() { _state = false; return true; }
        public override bool WaitOne(int millisecondsTimeout)
        {
            bool result = _state;
            _state = false;
            return result;
        }
    }

    // Mutex (ECMA-335)
    public sealed class Mutex : WaitHandle
    {
        public Mutex() { }
        public Mutex(bool initiallyOwned) { }
        public Mutex(bool initiallyOwned, String name) { }
        public void ReleaseMutex() { }
    }

    // Semaphore (ECMA-335)
    public sealed class Semaphore : WaitHandle
    {
        private int _count;
        private int _max;
        public Semaphore(int initialCount, int maximumCount) { _count = initialCount; _max = maximumCount; }
        public Semaphore(int initialCount, int maximumCount, String name) : this(initialCount, maximumCount) { }
        public int Release() => Release(1);
        public int Release(int releaseCount)
        {
            int prev = _count;
            _count = Math.Min(_count + releaseCount, _max);
            return prev;
        }
    }

    // EventResetMode (ECMA-335)
    public enum EventResetMode { AutoReset = 0, ManualReset = 1 }

    // EventWaitHandle (ECMA-335)
    public class EventWaitHandle : WaitHandle
    {
        private bool _state;
        public EventWaitHandle(bool initialState, EventResetMode mode) { _state = initialState; }
        public bool Set() { _state = true; return true; }
        public bool Reset() { _state = false; return true; }
    }

    // ThreadPool (ECMA-335)
    public static class ThreadPool
    {
        public static bool QueueUserWorkItem(WaitCallback callBack) => QueueUserWorkItem(callBack, null);
        public static bool QueueUserWorkItem(WaitCallback callBack, Object state)
        {
            callBack?.Invoke(state);
            return true;
        }
        public static bool SetMinThreads(int workerThreads, int completionPortThreads) => true;
        public static bool SetMaxThreads(int workerThreads, int completionPortThreads) => true;
        public static void GetMinThreads(out int workerThreads, out int completionPortThreads) { workerThreads = 1; completionPortThreads = 1; }
        public static void GetMaxThreads(out int workerThreads, out int completionPortThreads) { workerThreads = 1; completionPortThreads = 1; }
    }

    public delegate void WaitCallback(Object state);
    public delegate void ParameterizedThreadStart(Object obj);
    public delegate void ThreadStart();
    public delegate void TimerCallback(Object state);
    public delegate void ContextCallback(Object state);

    // Volatile
    public static class Volatile
    {
        public static int Read(ref int location) => location;
        public static long Read(ref long location) => location;
        public static T Read<T>(ref T location) where T : class => location;
        public static void Write(ref int location, int value) => location = value;
        public static void Write(ref long location, long value) => location = value;
        public static void Write<T>(ref T location, T value) where T : class => location = value;
    }

    // SpinWait
    public struct SpinWait
    {
        private int _count;
        public int Count => _count;
        public bool NextSpinWillYield => _count >= 10;
        public void SpinOnce() { _count++; }
        public void Reset() { _count = 0; }
        public static void SpinUntil(Func<bool> condition) { while (!condition()) new SpinWait().SpinOnce(); }
    }

    // CancellationToken
    public readonly struct CancellationToken
    {
        public static CancellationToken None => default;
        public bool IsCancellationRequested => false;
        public bool CanBeCanceled => false;
        public void ThrowIfCancellationRequested() { }
    }

    // CancellationTokenSource
    public class CancellationTokenSource : IDisposable
    {
        private bool _cancelled;
        public CancellationTokenSource() { }
        public CancellationToken Token => new CancellationToken();
        public bool IsCancellationRequested => _cancelled;
        public void Cancel() { _cancelled = true; }
        public void Dispose() { }
    }

    // SynchronizationContext
    public class SynchronizationContext
    {
        private static SynchronizationContext _current;
        public static SynchronizationContext Current => _current;
        public static void SetSynchronizationContext(SynchronizationContext syncContext) => _current = syncContext;
        public virtual void Post(SendOrPostCallback d, Object state) => d?.Invoke(state);
        public virtual void Send(SendOrPostCallback d, Object state) => d?.Invoke(state);
    }

    public delegate void SendOrPostCallback(Object state);
}

namespace System.Threading.Tasks
{
    using System.Collections.Generic;
    using System.Runtime.CompilerServices;

    // Task (ECMA-335)
    public class Task
    {
        protected Exception _exception;
        protected bool _completed;

        public Task(Action action) { action?.Invoke(); _completed = true; }
        public Task(Action<Object> action, Object state) { action?.Invoke(state); _completed = true; }
        internal Task() { _completed = true; }

        public bool IsCompleted => _completed;
        public bool IsFaulted => _exception != null;
        public bool IsCanceled => false;
        public TaskStatus Status => _completed ? TaskStatus.RanToCompletion : TaskStatus.Running;
        public Exception Exception => _exception != null ? new AggregateException(_exception) : null;

        public void Wait() { }
        public bool Wait(int millisecondsTimeout) => true;
        public ConfiguredTaskAwaitable ConfigureAwait(bool continueOnCapturedContext) => new ConfiguredTaskAwaitable(this);
        public TaskAwaiter GetAwaiter() => new TaskAwaiter(this);
        public void Start() { _completed = true; }

        public Task ContinueWith(Action<Task> continuationAction)
        {
            continuationAction?.Invoke(this);
            return new Task();
        }

        public static Task Run(Action action)
        {
            action?.Invoke();
            return CompletedTask;
        }

        public static Task<TResult> Run<TResult>(Func<TResult> function) => Task<TResult>.Run(function);

        public static Task Delay(int millisecondsDelay) => CompletedTask;
        public static Task Delay(TimeSpan delay) => CompletedTask;

        public static Task<TResult> FromResult<TResult>(TResult result) => new Task<TResult>(result);
        public static Task CompletedTask { get; } = new Task();

        public static Task WhenAll(params Task[] tasks) => CompletedTask;
        public static Task WhenAll(IEnumerable<Task> tasks) => CompletedTask;
        public static Task<Task> WhenAny(params Task[] tasks) => FromResult(tasks.Length > 0 ? tasks[0] : CompletedTask);
    }

    // Task<TResult>
    public class Task<TResult> : Task
    {
        private TResult _result;

        public Task(Func<TResult> function) : base() { _result = function(); }
        public Task(Func<Object, TResult> function, Object state) : base() { _result = function(state); }
        internal Task(TResult result) : base() { _result = result; }

        public TResult Result => _result;
        public new TaskAwaiter<TResult> GetAwaiter() => new TaskAwaiter<TResult>(this);
        public new ConfiguredTaskAwaitable<TResult> ConfigureAwait(bool continueOnCapturedContext) => new ConfiguredTaskAwaitable<TResult>(this);

        public static new Task<TResult> Run(Func<TResult> function) => new Task<TResult>(function);
    }

    // TaskStatus
    public enum TaskStatus { Created, WaitingForActivation, WaitingToRun, Running, WaitingForChildrenToComplete, RanToCompletion, Canceled, Faulted }

    // AggregateException
    public class AggregateException : Exception
    {
        private Exception[] _innerExceptions;
        public AggregateException(params Exception[] innerExceptions) : base("One or more errors occurred.")
        {
            _innerExceptions = innerExceptions ?? Array.Empty<Exception>();
        }
        public AggregateException(String message) : base(message) { _innerExceptions = Array.Empty<Exception>(); }
        public AggregateException(String message, Exception innerException) : base(message, innerException)
        {
            _innerExceptions = new Exception[] { innerException };
        }
        public System.Collections.ObjectModel.ReadOnlyCollection<Exception> InnerExceptions =>
            new System.Collections.ObjectModel.ReadOnlyCollection<Exception>(_innerExceptions);
        public AggregateException Flatten()
        {
            var list = new System.Collections.Generic.List<Exception>();
            foreach (var ex in _innerExceptions)
                if (ex is AggregateException ae) list.AddRange(ae.InnerExceptions);
                else list.Add(ex);
            return new AggregateException(list.ToArray());
        }
    }

    // TaskAwaiter
    public readonly struct TaskAwaiter : INotifyCompletion
    {
        private readonly Task _task;
        internal TaskAwaiter(Task task) { _task = task; }
        public bool IsCompleted => _task?.IsCompleted ?? true;
        public void GetResult() { }
        public void OnCompleted(Action continuation) => continuation?.Invoke();
    }

    public readonly struct TaskAwaiter<TResult> : INotifyCompletion
    {
        private readonly Task<TResult> _task;
        internal TaskAwaiter(Task<TResult> task) { _task = task; }
        public bool IsCompleted => _task?.IsCompleted ?? true;
        public TResult GetResult() => _task.Result;
        public void OnCompleted(Action continuation) => continuation?.Invoke();
    }

    // ConfiguredTaskAwaitable
    public readonly struct ConfiguredTaskAwaitable
    {
        private readonly Task _task;
        internal ConfiguredTaskAwaitable(Task task) { _task = task; }
        public ConfiguredTaskAwaiter GetAwaiter() => new ConfiguredTaskAwaiter(_task);
        public readonly struct ConfiguredTaskAwaiter : INotifyCompletion
        {
            private readonly Task _task;
            internal ConfiguredTaskAwaiter(Task task) { _task = task; }
            public bool IsCompleted => _task?.IsCompleted ?? true;
            public void GetResult() { }
            public void OnCompleted(Action continuation) => continuation?.Invoke();
        }
    }

    public readonly struct ConfiguredTaskAwaitable<TResult>
    {
        private readonly Task<TResult> _task;
        internal ConfiguredTaskAwaitable(Task<TResult> task) { _task = task; }
        public ConfiguredTaskAwaiter GetAwaiter() => new ConfiguredTaskAwaiter(_task);
        public readonly struct ConfiguredTaskAwaiter : INotifyCompletion
        {
            private readonly Task<TResult> _task;
            internal ConfiguredTaskAwaiter(Task<TResult> task) { _task = task; }
            public bool IsCompleted => _task?.IsCompleted ?? true;
            public TResult GetResult() => _task.Result;
            public void OnCompleted(Action continuation) => continuation?.Invoke();
        }
    }

    // INotifyCompletion
    public interface INotifyCompletion
    {
        void OnCompleted(Action continuation);
    }

    public interface ICriticalNotifyCompletion : INotifyCompletion
    {
        void UnsafeOnCompleted(Action continuation);
    }
}
