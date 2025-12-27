/*
 * ECMA-335 IO Types - Lux9 BCL
 * 
 * Stream and IO types per CLI specification.
 */
namespace System.IO
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;

    // Stream (ECMA-335 IV.5.79)
    public abstract class Stream : IDisposable
    {
        public abstract bool CanRead { get; }
        public abstract bool CanWrite { get; }
        public abstract bool CanSeek { get; }
        public virtual bool CanTimeout => false;
        public abstract long Length { get; }
        public abstract long Position { get; set; }
        public virtual int ReadTimeout { get => throw new InvalidOperationException(); set => throw new InvalidOperationException(); }
        public virtual int WriteTimeout { get => throw new InvalidOperationException(); set => throw new InvalidOperationException(); }

        public abstract int Read(byte[] buffer, int offset, int count);
        public virtual int Read(Span<byte> buffer) => Read(buffer.ToArray(), 0, buffer.Length);
        public virtual int ReadByte()
        {
            byte[] buf = new byte[1];
            return Read(buf, 0, 1) == 0 ? -1 : buf[0];
        }

        public abstract void Write(byte[] buffer, int offset, int count);
        public virtual void Write(ReadOnlySpan<byte> buffer) => Write(buffer.ToArray(), 0, buffer.Length);
        public virtual void WriteByte(byte value) => Write(new byte[] { value }, 0, 1);

        public abstract long Seek(long offset, SeekOrigin origin);
        public abstract void SetLength(long value);
        public abstract void Flush();
        public virtual void Close() => Dispose(true);

        public void Dispose()
        {
            Close();
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing) { }

        public void CopyTo(Stream destination) => CopyTo(destination, 81920);
        public virtual void CopyTo(Stream destination, int bufferSize)
        {
            byte[] buffer = new byte[bufferSize];
            int read;
            while ((read = Read(buffer, 0, buffer.Length)) != 0)
                destination.Write(buffer, 0, read);
        }

        // Async methods - simple sync wrappers
        public virtual Task<int> ReadAsync(byte[] buffer, int offset, int count) =>
            Task.FromResult(Read(buffer, offset, count));
        public virtual Task WriteAsync(byte[] buffer, int offset, int count)
        {
            Write(buffer, offset, count);
            return Task.CompletedTask;
        }
        public virtual Task FlushAsync()
        {
            Flush();
            return Task.CompletedTask;
        }
        public virtual Task CopyToAsync(Stream destination) => CopyToAsync(destination, 81920);
        public virtual Task CopyToAsync(Stream destination, int bufferSize)
        {
            CopyTo(destination, bufferSize);
            return Task.CompletedTask;
        }

        public static readonly Stream Null = new NullStream();

        private sealed class NullStream : Stream
        {
            public override bool CanRead => true;
            public override bool CanWrite => true;
            public override bool CanSeek => true;
            public override long Length => 0;
            public override long Position { get => 0; set { } }
            public override int Read(byte[] buffer, int offset, int count) => 0;
            public override void Write(byte[] buffer, int offset, int count) { }
            public override long Seek(long offset, SeekOrigin origin) => 0;
            public override void SetLength(long value) { }
            public override void Flush() { }
        }
    }

    // SeekOrigin (ECMA-335)
    public enum SeekOrigin
    {
        Begin = 0,
        Current = 1,
        End = 2
    }

    // MemoryStream (ECMA-335)
    public class MemoryStream : Stream
    {
        private byte[] _buffer;
        private int _length;
        private int _position;
        private int _capacity;
        private bool _writable;
        private bool _expandable;
        private bool _isOpen;

        public MemoryStream() : this(0) { }
        public MemoryStream(int capacity)
        {
            _buffer = new byte[capacity];
            _capacity = capacity;
            _writable = true;
            _expandable = true;
            _isOpen = true;
        }
        public MemoryStream(byte[] buffer) : this(buffer, true) { }
        public MemoryStream(byte[] buffer, bool writable)
        {
            _buffer = buffer ?? throw new ArgumentNullException(nameof(buffer));
            _length = buffer.Length;
            _capacity = buffer.Length;
            _writable = writable;
            _expandable = false;
            _isOpen = true;
        }

        public override bool CanRead => _isOpen;
        public override bool CanWrite => _writable;
        public override bool CanSeek => _isOpen;
        public override long Length => _length;
        public override long Position
        {
            get => _position;
            set
            {
                if (value < 0) throw new ArgumentOutOfRangeException(nameof(value));
                _position = (int)value;
            }
        }

        public virtual int Capacity
        {
            get => _capacity;
            set
            {
                if (value < _length) throw new ArgumentOutOfRangeException(nameof(value));
                if (!_expandable && value != _capacity) throw new NotSupportedException();
                if (value != _capacity)
                {
                    byte[] newBuffer = new byte[value];
                    Array.Copy(_buffer, newBuffer, _length);
                    _buffer = newBuffer;
                    _capacity = value;
                }
            }
        }

        public override int Read(byte[] buffer, int offset, int count)
        {
            if (!_isOpen) throw new ObjectDisposedException(null);
            int n = Math.Min(count, _length - _position);
            if (n <= 0) return 0;
            Array.Copy(_buffer, _position, buffer, offset, n);
            _position += n;
            return n;
        }

        public override void Write(byte[] buffer, int offset, int count)
        {
            if (!_isOpen) throw new ObjectDisposedException(null);
            if (!_writable) throw new NotSupportedException();
            int newPos = _position + count;
            if (newPos > _capacity)
            {
                if (!_expandable) throw new NotSupportedException();
                Capacity = Math.Max(newPos, _capacity * 2);
            }
            Array.Copy(buffer, offset, _buffer, _position, count);
            _position = newPos;
            if (newPos > _length) _length = newPos;
        }

        public override long Seek(long offset, SeekOrigin origin)
        {
            if (!_isOpen) throw new ObjectDisposedException(null);
            int newPos = origin switch
            {
                SeekOrigin.Begin => (int)offset,
                SeekOrigin.Current => _position + (int)offset,
                SeekOrigin.End => _length + (int)offset,
                _ => throw new ArgumentException()
            };
            if (newPos < 0) throw new IOException("Seek before beginning of stream.");
            _position = newPos;
            return _position;
        }

        public override void SetLength(long value)
        {
            if (!_isOpen) throw new ObjectDisposedException(null);
            if (!_writable) throw new NotSupportedException();
            int newLen = (int)value;
            if (newLen > _capacity) Capacity = newLen;
            _length = newLen;
            if (_position > _length) _position = _length;
        }

        public override void Flush() { }

        public virtual byte[] ToArray()
        {
            byte[] copy = new byte[_length];
            Array.Copy(_buffer, copy, _length);
            return copy;
        }

        public virtual byte[] GetBuffer() => _buffer;
        public virtual bool TryGetBuffer(out ArraySegment<byte> buffer)
        {
            buffer = new ArraySegment<byte>(_buffer, 0, _length);
            return true;
        }

        protected override void Dispose(bool disposing)
        {
            _isOpen = false;
            _writable = false;
            base.Dispose(disposing);
        }
    }

    // TextReader (ECMA-335)
    public abstract class TextReader : IDisposable
    {
        protected TextReader() { }
        public virtual void Close() => Dispose(true);
        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }
        protected virtual void Dispose(bool disposing) { }
        public virtual int Peek() => -1;
        public virtual int Read() => -1;
        public virtual int Read(char[] buffer, int index, int count) => 0;
        public virtual String ReadLine() => null;
        public virtual String ReadToEnd() => String.Empty;
        public static readonly TextReader Null = new NullTextReader();
        private sealed class NullTextReader : TextReader { }
    }

    // TextWriter (ECMA-335)
    public abstract class TextWriter : IDisposable
    {
        protected TextWriter() { }
        public abstract System.Text.Encoding Encoding { get; }
        public virtual String NewLine { get; set; } = "\n";
        public virtual void Close() => Dispose(true);
        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }
        protected virtual void Dispose(bool disposing) { }
        public virtual void Flush() { }
        public virtual void Write(char value) { }
        public virtual void Write(char[] buffer) { if (buffer != null) Write(buffer, 0, buffer.Length); }
        public virtual void Write(char[] buffer, int index, int count) { for (int i = 0; i < count; i++) Write(buffer[index + i]); }
        public virtual void Write(String value) { if (value != null) Write(value.ToCharArray()); }
        public virtual void Write(Object value) { if (value != null) Write(value.ToString()); }
        public virtual void Write(bool value) => Write(value ? "True" : "False");
        public virtual void Write(int value) => Write(value.ToString());
        public virtual void Write(long value) => Write(value.ToString());
        public virtual void Write(double value) => Write(value.ToString());
        public void WriteLine() => Write(NewLine);
        public void WriteLine(String value) { Write(value); WriteLine(); }
        public void WriteLine(Object value) { Write(value); WriteLine(); }
        public void WriteLine(int value) { Write(value); WriteLine(); }
        public void WriteLine(char value) { Write(value); WriteLine(); }
        public static readonly TextWriter Null = new NullTextWriter();
        private sealed class NullTextWriter : TextWriter { public override System.Text.Encoding Encoding => System.Text.Encoding.UTF8; }
    }

    // StringReader (ECMA-335)
    public class StringReader : TextReader
    {
        private String _s;
        private int _pos;
        public StringReader(String s) { _s = s ?? throw new ArgumentNullException(nameof(s)); }
        public override int Peek() => _pos >= _s.Length ? -1 : _s[_pos];
        public override int Read() => _pos >= _s.Length ? -1 : _s[_pos++];
        public override int Read(char[] buffer, int index, int count)
        {
            int n = Math.Min(count, _s.Length - _pos);
            for (int i = 0; i < n; i++) buffer[index + i] = _s[_pos++];
            return n;
        }
        public override String ReadLine()
        {
            if (_pos >= _s.Length) return null;
            int start = _pos;
            while (_pos < _s.Length && _s[_pos] != '\n' && _s[_pos] != '\r') _pos++;
            String line = _s.Substring(start, _pos - start);
            if (_pos < _s.Length && _s[_pos] == '\r') _pos++;
            if (_pos < _s.Length && _s[_pos] == '\n') _pos++;
            return line;
        }
        public override String ReadToEnd()
        {
            String rest = _s.Substring(_pos);
            _pos = _s.Length;
            return rest;
        }
    }

    // StringWriter (ECMA-335)
    public class StringWriter : TextWriter
    {
        private System.Text.StringBuilder _sb;
        public StringWriter() : this(new System.Text.StringBuilder()) { }
        public StringWriter(System.Text.StringBuilder sb) { _sb = sb ?? throw new ArgumentNullException(nameof(sb)); }
        public override System.Text.Encoding Encoding => System.Text.Encoding.UTF8;
        public virtual System.Text.StringBuilder GetStringBuilder() => _sb;
        public override void Write(char value) => _sb.Append(value);
        public override void Write(String value) { if (value != null) _sb.Append(value); }
        public override String ToString() => _sb.ToString();
    }

    // IOException (ECMA-335)
    public class IOException : SystemException
    {
        public IOException() : base("I/O error occurred.") { }
        public IOException(String message) : base(message) { }
        public IOException(String message, Exception innerException) : base(message, innerException) { }
        public IOException(String message, int hresult) : base(message) { HResult = hresult; }
    }

    // FileNotFoundException (ECMA-335)
    public class FileNotFoundException : IOException
    {
        public FileNotFoundException() : base("Unable to find the specified file.") { }
        public FileNotFoundException(String message) : base(message) { }
        public FileNotFoundException(String message, String fileName) : base(message) { FileName = fileName; }
        public FileNotFoundException(String message, Exception innerException) : base(message, innerException) { }
        public FileNotFoundException(String message, String fileName, Exception innerException) : base(message, innerException) { FileName = fileName; }
        public String FileName { get; }
    }

    // DirectoryNotFoundException (ECMA-335)
    public class DirectoryNotFoundException : IOException
    {
        public DirectoryNotFoundException() : base("Could not find a part of the path.") { }
        public DirectoryNotFoundException(String message) : base(message) { }
        public DirectoryNotFoundException(String message, Exception innerException) : base(message, innerException) { }
    }

    // EndOfStreamException (ECMA-335)
    public class EndOfStreamException : IOException
    {
        public EndOfStreamException() : base("Attempted to read past the end of the stream.") { }
        public EndOfStreamException(String message) : base(message) { }
        public EndOfStreamException(String message, Exception innerException) : base(message, innerException) { }
    }

    // FileLoadException (ECMA-335)
    public class FileLoadException : IOException
    {
        public FileLoadException() : base("Could not load the specified file.") { }
        public FileLoadException(String message) : base(message) { }
        public FileLoadException(String message, String fileName) : base(message) { FileName = fileName; }
        public FileLoadException(String message, Exception innerException) : base(message, innerException) { }
        public String FileName { get; }
    }

    // PathTooLongException (ECMA-335)
    public class PathTooLongException : IOException
    {
        public PathTooLongException() : base("The specified path, file name, or both are too long.") { }
        public PathTooLongException(String message) : base(message) { }
        public PathTooLongException(String message, Exception innerException) : base(message, innerException) { }
    }

    // FileMode/FileAccess/FileShare (ECMA-335)
    public enum FileMode { CreateNew = 1, Create = 2, Open = 3, OpenOrCreate = 4, Truncate = 5, Append = 6 }
    public enum FileAccess { Read = 1, Write = 2, ReadWrite = 3 }
    [Flags] public enum FileShare { None = 0, Read = 1, Write = 2, ReadWrite = 3, Delete = 4, Inheritable = 16 }
    [Flags] public enum FileOptions { None = 0, WriteThrough = -2147483648, Asynchronous = 1073741824, RandomAccess = 268435456, DeleteOnClose = 67108864, SequentialScan = 134217728, Encrypted = 16384 }
}
