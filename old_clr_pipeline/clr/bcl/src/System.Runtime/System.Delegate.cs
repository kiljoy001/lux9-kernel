namespace System
{
    public abstract class Delegate : Object
    {
        // Kernel support for delegates would go here
        // Usually contains target object and method pointer
    }

    public abstract class MulticastDelegate : Delegate
    {
        // Support for delegate chains
    }

    // Standard Delegates
    public delegate void Action();
    public delegate void Action<T>(T obj);
    public delegate void Action<T1, T2>(T1 arg1, T2 arg2);

    public delegate TResult Func<out TResult>();
    public delegate TResult Func<in T, out TResult>(T arg);
    public delegate TResult Func<in T1, in T2, out TResult>(T1 arg1, T2 arg2);
}
