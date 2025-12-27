/*
 * System.IO Exceptions
 * Lux9 CLR Base Class Library
 */
namespace System.IO
{
    /// <summary>
    /// Base exception for I/O errors.
    /// </summary>
    public class IOException : Exception
    {
        public IOException() : base("I/O error occurred.") { }
        public IOException(string message) : base(message) { }
        public IOException(string message, Exception innerException) : base(message, innerException) { }
    }

    /// <summary>
    /// Exception thrown when a file is not found.
    /// </summary>
    public class FileNotFoundException : IOException
    {
        public string FileName { get; }
        
        public FileNotFoundException() : base("Unable to find the specified file.") { }
        public FileNotFoundException(string message) : base(message) { }
        public FileNotFoundException(string message, string fileName) : base(message)
        {
            FileName = fileName;
        }
    }

    /// <summary>
    /// Exception thrown when a directory is not found.
    /// </summary>
    public class DirectoryNotFoundException : IOException
    {
        public DirectoryNotFoundException() : base("Attempted to access a path that is not on the disk.") { }
        public DirectoryNotFoundException(string message) : base(message) { }
    }

    /// <summary>
    /// Exception thrown when a path or filename is invalid.
    /// </summary>
    public class PathTooLongException : IOException
    {
        public PathTooLongException() : base("The specified path, file name, or both are too long.") { }
        public PathTooLongException(string message) : base(message) { }
    }

    /// <summary>
    /// Exception thrown when the end of a stream is reached unexpectedly.
    /// </summary>
    public class EndOfStreamException : IOException
    {
        public EndOfStreamException() : base("Attempted to read past the end of the stream.") { }
        public EndOfStreamException(string message) : base(message) { }
    }
}
