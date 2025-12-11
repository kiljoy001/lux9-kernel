/*
 * System.Threading.Tasks - Task-based asynchronous programming
 * Lux9 CLR Base Class Library
 */
namespace System.Threading.Tasks
{
    using System;
    using System.Collections.Generic;
    using System.Runtime.CompilerServices;

    /// <summary>
    /// Represents the current stage in the lifecycle of a Task.
    /// </summary>
    public enum TaskStatus
    {
        Created,
        WaitingForActivation,
        WaitingToRun,
        Running,
        WaitingForChildrenToComplete,
        RanToCompletion,
        Canceled,
        Faulted
    }

    /// <summary>
    /// Represents an asynchronous operation.
    /// </summary>
    public class Task
    {
        private volatile TaskStatus _status;
        private Exception _exception;
        private Action _action;
        private object _result;
        private readonly List<Action<Task>> _continuations = new List<Action<Task>>();
        private readonly object _stateLock = new object();
        
        public Task(Action action)
        {
            _action = action ?? throw new ArgumentNullException(nameof(action));
            _status = TaskStatus.Created;
        }
        
        protected Task()
        {
            _status = TaskStatus.Created;
        }
        
        public TaskStatus Status => _status;
        public bool IsCompleted => _status >= TaskStatus.RanToCompletion;
        public bool IsCompletedSuccessfully => _status == TaskStatus.RanToCompletion;
        public bool IsCanceled => _status == TaskStatus.Canceled;
        public bool IsFaulted => _status == TaskStatus.Faulted;
        
        public Exception Exception => _exception;
        
        public void Start()
        {
            if (_status != TaskStatus.Created)
                throw new InvalidOperationException("Task has already been started");
            
            _status = TaskStatus.WaitingToRun;
            
            // In kernel context, we run synchronously or schedule to thread pool
            RunSynchronously();
        }
        
        public void RunSynchronously()
        {
            lock (_stateLock)
            {
                if (_status >= TaskStatus.RanToCompletion) return;
                _status = TaskStatus.Running;
            }
            
            try
            {
                ExecuteCore();
                lock (_stateLock)
                {
                    _status = TaskStatus.RanToCompletion;
                }
            }
            catch (Exception ex)
            {
                lock (_stateLock)
                {
                    _exception = ex;
                    _status = TaskStatus.Faulted;
                }
            }
            
            RunContinuations();
        }
        
        protected virtual void ExecuteCore()
        {
            _action?.Invoke();
        }
        
        public void Wait()
        {
            // Spin until complete
            while (!IsCompleted)
            {
                Thread.SpinWait(1);
            }
            
            if (IsFaulted && _exception != null)
                throw _exception;
        }
        
        public bool Wait(int millisecondsTimeout)
        {
            // Simplified: doesn't actually timeout
            Wait();
            return true;
        }
        
        public Task ContinueWith(Action<Task> continuationAction)
        {
            if (continuationAction == null) throw new ArgumentNullException(nameof(continuationAction));
            
            Task continuationTask = new Task(() => continuationAction(this));
            
            lock (_stateLock)
            {
                if (IsCompleted)
                {
                    continuationTask.Start();
                }
                else
                {
                    _continuations.Add(t => continuationTask.Start());
                }
            }
            
            return continuationTask;
        }
        
        private void RunContinuations()
        {
            List<Action<Task>> toRun;
            lock (_stateLock)
            {
                toRun = new List<Action<Task>>(_continuations);
                _continuations.Clear();
            }
            
            foreach (var continuation in toRun)
            {
                try { continuation(this); }
                catch { /* Swallow continuation exceptions */ }
            }
        }
        
        // Static factory methods
        public static Task Run(Action action)
        {
            Task task = new Task(action);
            task.Start();
            return task;
        }
        
        public static Task<TResult> Run<TResult>(Func<TResult> function)
        {
            Task<TResult> task = new Task<TResult>(function);
            task.Start();
            return task;
        }
        
        public static Task Delay(int millisecondsDelay)
        {
            return Run(() => Thread.Sleep(millisecondsDelay));
        }
        
        public static Task Delay(TimeSpan delay)
        {
            return Delay((int)delay.TotalMilliseconds);
        }
        
        public static Task<TResult> FromResult<TResult>(TResult result)
        {
            return new Task<TResult>(() => result) { _result = result, _status = TaskStatus.RanToCompletion };
        }
        
        public static Task FromException(Exception exception)
        {
            Task task = new Task(() => { });
            task._exception = exception;
            task._status = TaskStatus.Faulted;
            return task;
        }
        
        public static Task CompletedTask { get; } = FromResult<object>(null);
        
        public static Task WhenAll(params Task[] tasks)
        {
            return Run(() =>
            {
                foreach (Task t in tasks)
                    t.Wait();
            });
        }
        
        public static Task WhenAll(IEnumerable<Task> tasks)
        {
            List<Task> taskList = new List<Task>(tasks);
            return WhenAll(taskList.ToArray());
        }
        
        public static Task<Task> WhenAny(params Task[] tasks)
        {
            return Run(() =>
            {
                while (true)
                {
                    foreach (Task t in tasks)
                    {
                        if (t.IsCompleted) return t;
                    }
                    Thread.SpinWait(1);
                }
            });
        }
        
        public TaskAwaiter GetAwaiter() => new TaskAwaiter(this);
    }

    /// <summary>
    /// Represents an asynchronous operation that produces a result.
    /// </summary>
    public class Task<TResult> : Task
    {
        private TResult _result;
        private Func<TResult> _function;
        
        public Task(Func<TResult> function) : base()
        {
            _function = function ?? throw new ArgumentNullException(nameof(function));
        }
        
        public TResult Result
        {
            get
            {
                Wait();
                return _result;
            }
        }
        
        protected override void ExecuteCore()
        {
            _result = _function();
        }
        
        public Task<TNewResult> ContinueWith<TNewResult>(Func<Task<TResult>, TNewResult> continuationFunction)
        {
            return new Task<TNewResult>(() => continuationFunction(this));
        }
        
        public new TaskAwaiter<TResult> GetAwaiter() => new TaskAwaiter<TResult>(this);
    }

    /// <summary>
    /// Provides an awaitable object for Task.
    /// </summary>
    public struct TaskAwaiter : INotifyCompletion
    {
        private readonly Task _task;
        
        public TaskAwaiter(Task task)
        {
            _task = task;
        }
        
        public bool IsCompleted => _task.IsCompleted;
        
        public void GetResult()
        {
            _task.Wait();
        }
        
        public void OnCompleted(Action continuation)
        {
            _task.ContinueWith(_ => continuation());
        }
    }

    /// <summary>
    /// Provides an awaitable object for Task<TResult>.
    /// </summary>
    public struct TaskAwaiter<TResult> : INotifyCompletion
    {
        private readonly Task<TResult> _task;
        
        public TaskAwaiter(Task<TResult> task)
        {
            _task = task;
        }
        
        public bool IsCompleted => _task.IsCompleted;
        
        public TResult GetResult()
        {
            return _task.Result;
        }
        
        public void OnCompleted(Action continuation)
        {
            _task.ContinueWith(_ => continuation());
        }
    }

    /// <summary>
    /// Represents the producer side of a Task.
    /// </summary>
    public class TaskCompletionSource<TResult>
    {
        private readonly Task<TResult> _task;
        private bool _completed;
        private readonly object _lock = new object();
        
        public TaskCompletionSource()
        {
            _task = new Task<TResult>(() => default);
        }
        
        public Task<TResult> Task => _task;
        
        public void SetResult(TResult result)
        {
            lock (_lock)
            {
                if (_completed) throw new InvalidOperationException("Task already completed");
                _completed = true;
                // Would need to set the task's internal result
            }
        }
        
        public bool TrySetResult(TResult result)
        {
            lock (_lock)
            {
                if (_completed) return false;
                _completed = true;
                return true;
            }
        }
        
        public void SetException(Exception exception)
        {
            lock (_lock)
            {
                if (_completed) throw new InvalidOperationException("Task already completed");
                _completed = true;
            }
        }
        
        public bool TrySetException(Exception exception)
        {
            lock (_lock)
            {
                if (_completed) return false;
                _completed = true;
                return true;
            }
        }
        
        public void SetCanceled()
        {
            lock (_lock)
            {
                if (_completed) throw new InvalidOperationException("Task already completed");
                _completed = true;
            }
        }
    }
}


    
namespace System.Runtime.CompilerServices
{
    using System.Threading.Tasks;

    /// <summary>
    /// Interface for awaiters.
    /// </summary>
    public interface INotifyCompletion
    {
        void OnCompleted(Action continuation);
    }

    /// <summary>
    /// Builder for async methods that return Task.
    /// </summary>
    public struct AsyncTaskMethodBuilder
    {
        private Task _task;
        
        public static AsyncTaskMethodBuilder Create() => new AsyncTaskMethodBuilder();
        
        public void Start<TStateMachine>(ref TStateMachine stateMachine) where TStateMachine : IAsyncStateMachine
        {
            stateMachine.MoveNext();
        }
        
        public void SetStateMachine(IAsyncStateMachine stateMachine) { }
        
        public void SetResult()
        {
            // Mark task as completed
        }
        
        public void SetException(Exception exception)
        {
            // Mark task as faulted
        }
        
        public Task Task => _task ?? Task.CompletedTask;
        
        public void AwaitOnCompleted<TAwaiter, TStateMachine>(ref TAwaiter awaiter, ref TStateMachine stateMachine)
            where TAwaiter : INotifyCompletion
            where TStateMachine : IAsyncStateMachine
        {
            awaiter.OnCompleted(stateMachine.MoveNext);
        }
        
        public void AwaitUnsafeOnCompleted<TAwaiter, TStateMachine>(ref TAwaiter awaiter, ref TStateMachine stateMachine)
            where TAwaiter : INotifyCompletion
            where TStateMachine : IAsyncStateMachine
        {
            awaiter.OnCompleted(stateMachine.MoveNext);
        }
    }
    
    /// <summary>
    /// Builder for async methods that return Task<TResult>.
    /// </summary>
    public struct AsyncTaskMethodBuilder<TResult>
    {
        private Task<TResult> _task;
        private TResult _result;
        
        public static AsyncTaskMethodBuilder<TResult> Create() => new AsyncTaskMethodBuilder<TResult>();
        
        public void Start<TStateMachine>(ref TStateMachine stateMachine) where TStateMachine : IAsyncStateMachine
        {
            stateMachine.MoveNext();
        }
        
        public void SetStateMachine(IAsyncStateMachine stateMachine) { }
        
        public void SetResult(TResult result)
        {
            _result = result;
        }
        
        public void SetException(Exception exception) { }
        
        public Task<TResult> Task => _task ?? Task.FromResult(_result);
        
        public void AwaitOnCompleted<TAwaiter, TStateMachine>(ref TAwaiter awaiter, ref TStateMachine stateMachine)
            where TAwaiter : INotifyCompletion
            where TStateMachine : IAsyncStateMachine
        {
            awaiter.OnCompleted(stateMachine.MoveNext);
        }
        
        public void AwaitUnsafeOnCompleted<TAwaiter, TStateMachine>(ref TAwaiter awaiter, ref TStateMachine stateMachine)
            where TAwaiter : INotifyCompletion
            where TStateMachine : IAsyncStateMachine
        {
            awaiter.OnCompleted(stateMachine.MoveNext);
        }
    }
    
    /// <summary>
    /// Interface for async state machines.
    /// </summary>
    public interface IAsyncStateMachine
    {
        void MoveNext();
        void SetStateMachine(IAsyncStateMachine stateMachine);
    }
}
