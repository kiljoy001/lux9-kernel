/*
 * ECMA-335 Exception Types - Lux9 BCL
 * 
 * All exception types required by the CLI specification.
 */
namespace System
{
    // Exception (ECMA-335 IV.5.46)
    public class Exception
    {
        private String _message;
        private Exception _innerException;
        private String _stackTrace;

        public Exception() : this(null, null) { }
        public Exception(String message) : this(message, null) { }
        public Exception(String message, Exception innerException)
        {
            _message = message;
            _innerException = innerException;
        }

        public virtual String Message => _message ?? $"Exception of type '{GetType().FullName}' was thrown.";
        public Exception InnerException => _innerException;
        public virtual String StackTrace => _stackTrace;
        public virtual String HelpLink { get; set; }
        public virtual int HResult { get; set; }
        public virtual String Source { get; set; }

        public Exception GetBaseException()
        {
            Exception e = this;
            while (e._innerException != null) e = e._innerException;
            return e;
        }

        public override String ToString() =>
            _innerException == null ? $"{GetType().FullName}: {Message}" :
            $"{GetType().FullName}: {Message} ---> {_innerException}";
    }

    // SystemException (ECMA-335 IV.5.47)
    public class SystemException : Exception
    {
        public SystemException() : base() { }
        public SystemException(String message) : base(message) { }
        public SystemException(String message, Exception innerException) : base(message, innerException) { }
    }

    // ApplicationException (ECMA-335 IV.5.48)
    public class ApplicationException : Exception
    {
        public ApplicationException() : base() { }
        public ApplicationException(String message) : base(message) { }
        public ApplicationException(String message, Exception innerException) : base(message, innerException) { }
    }

    // ArgumentException (ECMA-335 IV.5.49)
    public class ArgumentException : SystemException
    {
        private String _paramName;
        public ArgumentException() : base("Value does not fall within the expected range.") { }
        public ArgumentException(String message) : base(message) { }
        public ArgumentException(String message, String paramName) : base(message) { _paramName = paramName; }
        public ArgumentException(String message, Exception innerException) : base(message, innerException) { }
        public ArgumentException(String message, String paramName, Exception innerException) : base(message, innerException) { _paramName = paramName; }
        public virtual String ParamName => _paramName;
        public override String Message => _paramName == null ? base.Message : $"{base.Message} (Parameter '{_paramName}')";
    }

    // ArgumentNullException (ECMA-335 IV.5.50)
    public class ArgumentNullException : ArgumentException
    {
        public ArgumentNullException() : base("Value cannot be null.") { }
        public ArgumentNullException(String paramName) : base("Value cannot be null.", paramName) { }
        public ArgumentNullException(String paramName, String message) : base(message, paramName) { }
        public ArgumentNullException(String message, Exception innerException) : base(message, innerException) { }
    }

    // ArgumentOutOfRangeException (ECMA-335 IV.5.51)
    public class ArgumentOutOfRangeException : ArgumentException
    {
        private Object _actualValue;
        public ArgumentOutOfRangeException() : base("Specified argument was out of the range of valid values.") { }
        public ArgumentOutOfRangeException(String paramName) : base("Specified argument was out of the range of valid values.", paramName) { }
        public ArgumentOutOfRangeException(String paramName, String message) : base(message, paramName) { }
        public ArgumentOutOfRangeException(String paramName, Object actualValue, String message) : base(message, paramName) { _actualValue = actualValue; }
        public ArgumentOutOfRangeException(String message, Exception innerException) : base(message, innerException) { }
        public virtual Object ActualValue => _actualValue;
    }

    // ArithmeticException (ECMA-335 IV.5.52)
    public class ArithmeticException : SystemException
    {
        public ArithmeticException() : base("Overflow or underflow in the arithmetic operation.") { }
        public ArithmeticException(String message) : base(message) { }
        public ArithmeticException(String message, Exception innerException) : base(message, innerException) { }
    }

    // DivideByZeroException (ECMA-335 IV.5.53)
    public class DivideByZeroException : ArithmeticException
    {
        public DivideByZeroException() : base("Attempted to divide by zero.") { }
        public DivideByZeroException(String message) : base(message) { }
        public DivideByZeroException(String message, Exception innerException) : base(message, innerException) { }
    }

    // OverflowException (ECMA-335 IV.5.54)
    public class OverflowException : ArithmeticException
    {
        public OverflowException() : base("Arithmetic operation resulted in an overflow.") { }
        public OverflowException(String message) : base(message) { }
        public OverflowException(String message, Exception innerException) : base(message, innerException) { }
    }

    // NotFiniteNumberException (ECMA-335)
    public class NotFiniteNumberException : ArithmeticException
    {
        private double _offendingNumber;
        public NotFiniteNumberException() : base("Number encountered was not a finite quantity.") { }
        public NotFiniteNumberException(String message) : base(message) { }
        public NotFiniteNumberException(double offendingNumber) : base() { _offendingNumber = offendingNumber; }
        public NotFiniteNumberException(String message, double offendingNumber) : base(message) { _offendingNumber = offendingNumber; }
        public NotFiniteNumberException(String message, Exception innerException) : base(message, innerException) { }
        public double OffendingNumber => _offendingNumber;
    }

    // ArrayTypeMismatchException (ECMA-335 IV.5.55)
    public class ArrayTypeMismatchException : SystemException
    {
        public ArrayTypeMismatchException() : base("Attempted to access an element as a type incompatible with the array.") { }
        public ArrayTypeMismatchException(String message) : base(message) { }
        public ArrayTypeMismatchException(String message, Exception innerException) : base(message, innerException) { }
    }

    // IndexOutOfRangeException (ECMA-335 IV.5.56)
    public class IndexOutOfRangeException : SystemException
    {
        public IndexOutOfRangeException() : base("Index was outside the bounds of the array.") { }
        public IndexOutOfRangeException(String message) : base(message) { }
        public IndexOutOfRangeException(String message, Exception innerException) : base(message, innerException) { }
    }

    // InvalidCastException (ECMA-335 IV.5.57)
    public class InvalidCastException : SystemException
    {
        public InvalidCastException() : base("Specified cast is not valid.") { }
        public InvalidCastException(String message) : base(message) { }
        public InvalidCastException(String message, Exception innerException) : base(message, innerException) { }
    }

    // InvalidOperationException (ECMA-335 IV.5.58)
    public class InvalidOperationException : SystemException
    {
        public InvalidOperationException() : base("Operation is not valid due to the current state of the object.") { }
        public InvalidOperationException(String message) : base(message) { }
        public InvalidOperationException(String message, Exception innerException) : base(message, innerException) { }
    }

    // NullReferenceException (ECMA-335 IV.5.59)
    public class NullReferenceException : SystemException
    {
        public NullReferenceException() : base("Object reference not set to an instance of an object.") { }
        public NullReferenceException(String message) : base(message) { }
        public NullReferenceException(String message, Exception innerException) : base(message, innerException) { }
    }

    // NotImplementedException (ECMA-335 IV.5.60)
    public class NotImplementedException : SystemException
    {
        public NotImplementedException() : base("The method or operation is not implemented.") { }
        public NotImplementedException(String message) : base(message) { }
        public NotImplementedException(String message, Exception innerException) : base(message, innerException) { }
    }

    // NotSupportedException (ECMA-335 IV.5.61)
    public class NotSupportedException : SystemException
    {
        public NotSupportedException() : base("Specified method is not supported.") { }
        public NotSupportedException(String message) : base(message) { }
        public NotSupportedException(String message, Exception innerException) : base(message, innerException) { }
    }

    // ObjectDisposedException (ECMA-335)
    public class ObjectDisposedException : InvalidOperationException
    {
        private String _objectName;
        public ObjectDisposedException(String objectName) : base("Cannot access a disposed object.") { _objectName = objectName; }
        public ObjectDisposedException(String objectName, String message) : base(message) { _objectName = objectName; }
        public ObjectDisposedException(String message, Exception innerException) : base(message, innerException) { }
        public String ObjectName => _objectName ?? String.Empty;
    }

    // OutOfMemoryException (ECMA-335 IV.5.62)
    public class OutOfMemoryException : SystemException
    {
        public OutOfMemoryException() : base("Insufficient memory to continue the execution of the program.") { }
        public OutOfMemoryException(String message) : base(message) { }
        public OutOfMemoryException(String message, Exception innerException) : base(message, innerException) { }
    }

    // StackOverflowException (ECMA-335 IV.5.63)
    public class StackOverflowException : SystemException
    {
        public StackOverflowException() : base("Operation caused a stack overflow.") { }
        public StackOverflowException(String message) : base(message) { }
        public StackOverflowException(String message, Exception innerException) : base(message, innerException) { }
    }

    // TypeInitializationException (ECMA-335 IV.5.64)
    public class TypeInitializationException : SystemException
    {
        private String _typeName;
        public TypeInitializationException(String fullTypeName, Exception innerException)
            : base($"The type initializer for '{fullTypeName}' threw an exception.", innerException)
        {
            _typeName = fullTypeName;
        }
        public String TypeName => _typeName;
    }

    // FormatException (ECMA-335 IV.5.65)
    public class FormatException : SystemException
    {
        public FormatException() : base("Input string was not in a correct format.") { }
        public FormatException(String message) : base(message) { }
        public FormatException(String message, Exception innerException) : base(message, innerException) { }
    }

    // RankException (ECMA-335)
    public class RankException : SystemException
    {
        public RankException() : base("Attempt to operate on an array with the incorrect number of dimensions.") { }
        public RankException(String message) : base(message) { }
        public RankException(String message, Exception innerException) : base(message, innerException) { }
    }

    // TypeLoadException (ECMA-335)
    public class TypeLoadException : SystemException
    {
        public TypeLoadException() : base("Failure has occurred while loading a type.") { }
        public TypeLoadException(String message) : base(message) { }
        public TypeLoadException(String message, Exception innerException) : base(message, innerException) { }
    }

    // MissingMemberException (ECMA-335)
    public class MissingMemberException : SystemException
    {
        public MissingMemberException() : base("Attempted to access a missing member.") { }
        public MissingMemberException(String message) : base(message) { }
        public MissingMemberException(String message, Exception innerException) : base(message, innerException) { }
    }

    // MissingMethodException (ECMA-335)
    public class MissingMethodException : MissingMemberException
    {
        public MissingMethodException() : base("Attempted to access a missing method.") { }
        public MissingMethodException(String message) : base(message) { }
        public MissingMethodException(String message, Exception innerException) : base(message, innerException) { }
    }

    // MissingFieldException (ECMA-335)
    public class MissingFieldException : MissingMemberException
    {
        public MissingFieldException() : base("Attempted to access a missing field.") { }
        public MissingFieldException(String message) : base(message) { }
        public MissingFieldException(String message, Exception innerException) : base(message, innerException) { }
    }

    // FieldAccessException (ECMA-335)
    public class FieldAccessException : MemberAccessException
    {
        public FieldAccessException() : base("Attempt to access a field failed.") { }
        public FieldAccessException(String message) : base(message) { }
        public FieldAccessException(String message, Exception innerException) : base(message, innerException) { }
    }

    // MethodAccessException (ECMA-335)
    public class MethodAccessException : MemberAccessException
    {
        public MethodAccessException() : base("Attempt to access a method failed.") { }
        public MethodAccessException(String message) : base(message) { }
        public MethodAccessException(String message, Exception innerException) : base(message, innerException) { }
    }

    // MemberAccessException (ECMA-335)
    public class MemberAccessException : SystemException
    {
        public MemberAccessException() : base("Cannot access a member.") { }
        public MemberAccessException(String message) : base(message) { }
        public MemberAccessException(String message, Exception innerException) : base(message, innerException) { }
    }

    // ExecutionEngineException (ECMA-335 - deprecated)
    [Obsolete("This type is for compatibility only.")]
    public sealed class ExecutionEngineException : SystemException
    {
        public ExecutionEngineException() : base("Internal error in the runtime.") { }
        public ExecutionEngineException(String message) : base(message) { }
        public ExecutionEngineException(String message, Exception innerException) : base(message, innerException) { }
    }

    // BadImageFormatException (ECMA-335)
    public class BadImageFormatException : SystemException
    {
        public BadImageFormatException() : base("Format of the executable (.exe) or library (.dll) is invalid.") { }
        public BadImageFormatException(String message) : base(message) { }
        public BadImageFormatException(String message, String fileName) : base(message) { FileName = fileName; }
        public BadImageFormatException(String message, Exception innerException) : base(message, innerException) { }
        public BadImageFormatException(String message, String fileName, Exception innerException) : base(message, innerException) { FileName = fileName; }
        public String FileName { get; }
    }

    // EntryPointNotFoundException (ECMA-335)
    public class EntryPointNotFoundException : TypeLoadException
    {
        public EntryPointNotFoundException() : base("Entry point was not found.") { }
        public EntryPointNotFoundException(String message) : base(message) { }
        public EntryPointNotFoundException(String message, Exception innerException) : base(message, innerException) { }
    }

    // DuplicateWaitObjectException (ECMA-335)
    public class DuplicateWaitObjectException : ArgumentException
    {
        public DuplicateWaitObjectException() : base("Duplicate objects in argument.") { }
        public DuplicateWaitObjectException(String parameterName) : base("Duplicate objects in argument.", parameterName) { }
        public DuplicateWaitObjectException(String parameterName, String message) : base(message, parameterName) { }
        public DuplicateWaitObjectException(String message, Exception innerException) : base(message, innerException) { }
    }

    // CannotUnloadAppDomainException (ECMA-335)
    public class CannotUnloadAppDomainException : SystemException
    {
        public CannotUnloadAppDomainException() : base("Attempt to unload the AppDomain failed.") { }
        public CannotUnloadAppDomainException(String message) : base(message) { }
        public CannotUnloadAppDomainException(String message, Exception innerException) : base(message, innerException) { }
    }
}

namespace System.Collections.Generic
{
    // KeyNotFoundException (ECMA-335)
    public class KeyNotFoundException : SystemException
    {
        public KeyNotFoundException() : base("The given key was not present in the dictionary.") { }
        public KeyNotFoundException(String message) : base(message) { }
        public KeyNotFoundException(String message, Exception innerException) : base(message, innerException) { }
    }
}
