/*
 * System.Exception - Base exception class and common exception types
 * Lux9 CLR Base Class Library
 */
namespace System
{
    /// <summary>
    /// Base class for all exceptions in the .NET type system.
    /// </summary>
    public class Exception
    {
        private string _message;
        private Exception _innerException;
        private string _stackTrace;

        public Exception() : this("Exception of type 'System.Exception' was thrown.")
        {
        }

        public Exception(string message)
        {
            _message = message;
        }

        public Exception(string message, Exception innerException) : this(message)
        {
            _innerException = innerException;
        }

        public virtual string Message => _message ?? "An error occurred.";
        public Exception InnerException => _innerException;
        public virtual string StackTrace => _stackTrace ?? "";
        
        // Set by runtime when exception is thrown
        internal void SetStackTrace(string trace) => _stackTrace = trace;
        
        public override string ToString()
        {
            string result = GetType().ToString() + ": " + Message;
            if (_innerException != null)
            {
                result += " ---> " + _innerException.ToString();
            }
            if (_stackTrace != null)
            {
                result += "\n" + _stackTrace;
            }
            return result;
        }
    }

    /// <summary>
    /// Exception thrown when an argument is invalid.
    /// </summary>
    public class ArgumentException : Exception
    {
        public string ParamName { get; }
        
        public ArgumentException() : base("Value does not fall within the expected range.") { }
        public ArgumentException(string message) : base(message) { }
        public ArgumentException(string message, string paramName) : base(message)
        {
            ParamName = paramName;
        }
        public ArgumentException(string message, Exception innerException) : base(message, innerException) { }
    }

    /// <summary>
    /// Exception thrown when a null argument is passed to a method that doesn't accept it.
    /// </summary>
    public class ArgumentNullException : ArgumentException
    {
        public ArgumentNullException() : base("Value cannot be null.") { }
        public ArgumentNullException(string paramName) : base("Value cannot be null.", paramName) { }
        public ArgumentNullException(string paramName, string message) : base(message, paramName) { }
    }

    /// <summary>
    /// Exception thrown when an argument is outside the allowable range of values.
    /// </summary>
    public class ArgumentOutOfRangeException : ArgumentException
    {
        public object ActualValue { get; }
        
        public ArgumentOutOfRangeException() : base("Specified argument was out of the range of valid values.") { }
        public ArgumentOutOfRangeException(string paramName) : base("Specified argument was out of the range of valid values.", paramName) { }
        public ArgumentOutOfRangeException(string paramName, string message) : base(message, paramName) { }
        public ArgumentOutOfRangeException(string paramName, object actualValue, string message) : base(message, paramName)
        {
            ActualValue = actualValue;
        }
    }

    /// <summary>
    /// Exception thrown when a method call is invalid for the object's current state.
    /// </summary>
    public class InvalidOperationException : Exception
    {
        public InvalidOperationException() : base("Operation is not valid due to the current state of the object.") { }
        public InvalidOperationException(string message) : base(message) { }
        public InvalidOperationException(string message, Exception innerException) : base(message, innerException) { }
    }

    /// <summary>
    /// Exception thrown when an invoked method is not supported.
    /// </summary>
    public class NotSupportedException : Exception
    {
        public NotSupportedException() : base("Specified method is not supported.") { }
        public NotSupportedException(string message) : base(message) { }
    }

    /// <summary>
    /// Exception thrown when a requested method or operation is not implemented.
    /// </summary>
    public class NotImplementedException : Exception
    {
        public NotImplementedException() : base("The method or operation is not implemented.") { }
        public NotImplementedException(string message) : base(message) { }
    }

    /// <summary>
    /// Exception thrown when there is an attempt to access an element with an invalid index.
    /// </summary>
    public class IndexOutOfRangeException : Exception
    {
        public IndexOutOfRangeException() : base("Index was outside the bounds of the array.") { }
        public IndexOutOfRangeException(string message) : base(message) { }
    }

    /// <summary>
    /// Exception thrown when a null reference is dereferenced.
    /// </summary>
    public class NullReferenceException : Exception
    {
        public NullReferenceException() : base("Object reference not set to an instance of an object.") { }
        public NullReferenceException(string message) : base(message) { }
    }

    /// <summary>
    /// Exception thrown when arithmetic operation results in overflow.
    /// </summary>
    public class OverflowException : ArithmeticException
    {
        public OverflowException() : base("Arithmetic operation resulted in an overflow.") { }
        public OverflowException(string message) : base(message) { }
    }

    /// <summary>
    /// Exception thrown for errors in arithmetic, casting, or conversion operations.
    /// </summary>
    public class ArithmeticException : Exception
    {
        public ArithmeticException() : base("Overflow or underflow in the arithmetic operation.") { }
        public ArithmeticException(string message) : base(message) { }
    }

    /// <summary>
    /// Exception thrown when dividing by zero.
    /// </summary>
    public class DivideByZeroException : ArithmeticException
    {
        public DivideByZeroException() : base("Attempted to divide by zero.") { }
        public DivideByZeroException(string message) : base(message) { }
    }

    /// <summary>
    /// Exception thrown when the format of an argument is invalid.
    /// </summary>
    public class FormatException : Exception
    {
        public FormatException() : base("Input string was not in a correct format.") { }
        public FormatException(string message) : base(message) { }
    }

    /// <summary>
    /// Exception thrown when an invalid cast or conversion is attempted.
    /// </summary>
    public class InvalidCastException : Exception
    {
        public InvalidCastException() : base("Specified cast is not valid.") { }
        public InvalidCastException(string message) : base(message) { }
    }

    /// <summary>
    /// Exception thrown when there is not enough memory to continue execution.
    /// </summary>
    public class OutOfMemoryException : Exception
    {
        public OutOfMemoryException() : base("Insufficient memory to continue the execution of the program.") { }
        public OutOfMemoryException(string message) : base(message) { }
    }

    /// <summary>
    /// Exception thrown when a stack overflow occurs because there are too many pending method calls.
    /// </summary>
    public class StackOverflowException : Exception
    {
        public StackOverflowException() : base("Operation caused a stack overflow.") { }
        public StackOverflowException(string message) : base(message) { }
    }

    /// <summary>
    /// Exception thrown when an operation is canceled.
    /// </summary>
    public class OperationCanceledException : Exception
    {
        public OperationCanceledException() : base("The operation was canceled.") { }
        public OperationCanceledException(string message) : base(message) { }
    }

    /// <summary>
    /// Exception thrown when a timeout expires.
    /// </summary>
    public class TimeoutException : Exception
    {
        public TimeoutException() : base("The operation has timed out.") { }
        public TimeoutException(string message) : base(message) { }
    }

    /// <summary>
    /// Exception thrown when accessing an object that has been disposed.
    /// </summary>
    public class ObjectDisposedException : InvalidOperationException
    {
        public string ObjectName { get; }
        
        public ObjectDisposedException(string objectName) 
            : base("Cannot access a disposed object.")
        {
            ObjectName = objectName;
        }
        
        public ObjectDisposedException(string objectName, string message) : base(message)
        {
            ObjectName = objectName;
        }
    }

    /// <summary>
    /// Exception thrown when a requested key is not found in a collection.
    /// </summary>
    public class KeyNotFoundException : Exception
    {
        public KeyNotFoundException() : base("The given key was not present in the dictionary.") { }
        public KeyNotFoundException(string message) : base(message) { }
    }
}
