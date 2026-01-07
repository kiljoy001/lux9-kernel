namespace System.IO
{
    using System;

    public abstract class Stream : IDisposable
    {
        public abstract void Write(byte[] buffer, int offset, int count);
        public abstract int Read(byte[] buffer, int offset, int count);
        
        public virtual void WriteByte(byte value)
        {
            Write(new byte[] { value }, 0, 1);
        }

        public virtual int ReadByte()
        {
            byte[] buffer = new byte[1];
            if (Read(buffer, 0, 1) == 0) return -1;
            return buffer[0];
        }

        public virtual void Close() {}
        public void Dispose() { Close(); }
        // Stub properties
        public virtual long Length => 0;
        public virtual long Position { get => 0; set {} }
    }

    public class MemoryStream : Stream
    {
        private byte[] _buffer;
        private int _position;
        private int _length;

        public MemoryStream() : this(new byte[0]) { }
        public MemoryStream(byte[] buffer)
        {
            _buffer = buffer;
            _length = buffer.Length;
        }

        public override void Write(byte[] buffer, int offset, int count) { }
        public override int Read(byte[] buffer, int offset, int count) { return 0; }
    }

    public abstract class TextWriter : IDisposable
    {
        public virtual void Write(char value) {}
        public virtual void Write(string value) 
        {
            // Stub: kernel would use internal call to iterate string
        }
        public virtual void WriteLine(string value)
        {
            Write(value);
            Write('\n');
        }
        public void Dispose() {}
    }


}
