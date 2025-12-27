namespace System.IO
{
    using System.P9;

    // FileStream backed by 9P protocol
    // Transparent to developers - works exactly like standard .NET FileStream
    public class FileStream : Stream
    {
        private uint _fid;          // 9P file identifier
        private string _path;       // Path to the resource
        private long _position;     // Current position in file
        private bool _disposed;     // Track disposal
        private bool _canRead;      // Whether stream supports reading
        private bool _canWrite;     // Whether stream supports writing

        public FileStream(string path, FileMode mode)
            : this(path, mode, mode == FileMode.Open ? FileAccess.Read : FileAccess.ReadWrite)
        {
        }

        public FileStream(string path, FileMode mode, FileAccess access)
        {
            if (path == null)
                throw new ArgumentNullException(nameof(path));

            _path = path;
            _position = 0;
            _disposed = false;
            _canRead = (access & FileAccess.Read) != 0;
            _canWrite = (access & FileAccess.Write) != 0;

            // Attach to the 9P resource
            // This sends Tattach with the process's Pebble token automatically
            _fid = P9Internal.Attach(path);

            // For Append mode, seek to end
            if (mode == FileMode.Append)
            {
                long len = P9Internal.Stat(_fid);
                if (len >= 0)
                    _position = len;
            }
        }

        public override int Read(byte[] buffer, int offset, int count)
        {
            if (_disposed)
                throw new ObjectDisposedException(nameof(FileStream));
            if (!_canRead)
                throw new NotSupportedException("Stream does not support reading");
            if (buffer == null)
                throw new ArgumentNullException(nameof(buffer));
            if (offset < 0 || count < 0 || offset + count > buffer.Length)
                throw new ArgumentOutOfRangeException();

            // Tread from current position
            int n = P9Internal.Read(_fid, buffer, offset, count, _position);
            if (n > 0)
                _position += n;

            return n;
        }

        public override void Write(byte[] buffer, int offset, int count)
        {
            if (_disposed)
                throw new ObjectDisposedException(nameof(FileStream));
            if (!_canWrite)
                throw new NotSupportedException("Stream does not support writing");
            if (buffer == null)
                throw new ArgumentNullException(nameof(buffer));
            if (offset < 0 || count < 0 || offset + count > buffer.Length)
                throw new ArgumentOutOfRangeException();

            // Twrite to current position
            int n = P9Internal.Write(_fid, buffer, offset, count, _position);
            _position += n;
        }

        public override void Close()
        {
            if (!_disposed)
            {
                // Tclunk to close the fid
                P9Internal.Clunk(_fid);
                _disposed = true;
            }
        }

        public override long Length
        {
            get
            {
                if (_disposed)
                    throw new ObjectDisposedException(nameof(FileStream));

                long len = P9Internal.Stat(_fid);
                return len >= 0 ? len : 0;
            }
        }

        public override long Position
        {
            get
            {
                if (_disposed)
                    throw new ObjectDisposedException(nameof(FileStream));
                return _position;
            }
            set
            {
                if (_disposed)
                    throw new ObjectDisposedException(nameof(FileStream));
                if (value < 0)
                    throw new ArgumentOutOfRangeException(nameof(value));
                _position = value;
            }
        }

        public string Name => _path;
    }

    // File access modes
    public enum FileMode
    {
        CreateNew = 1,
        Create = 2,
        Open = 3,
        OpenOrCreate = 4,
        Truncate = 5,
        Append = 6
    }

    [Flags]
    public enum FileAccess
    {
        Read = 1,
        Write = 2,
        ReadWrite = 3
    }
}
