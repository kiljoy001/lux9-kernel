/*
 * System.Console - Console I/O
 * Lux9 CLR Base Class Library
 */
namespace System
{
    using System.IO;
    using System.P9;

    public static class Console
    {
        // Console backed by /dev/cons via 9P
        public static TextWriter Out { get; } = new ConsoleWriter();
        public static TextReader In { get; } = new ConsoleReader();

        public static void WriteLine(string value) => Internal_WriteLine(value);
        public static void WriteLine(object value) => Internal_WriteLine(value?.ToString());
        public static void WriteLine() => Internal_WriteLine("");
        
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)]
        extern static void Internal_WriteLine(string value);
        
        public static void Write(string value) => Out.Write(value);
        public static void Write(object value) => Out.Write(value?.ToString());

        // Console input methods
        public static string ReadLine() => In.ReadLine();
        public static int Read() => In.Read();

        class ConsoleWriter : TextWriter
        {
            private uint _fid;
            private static bool _initialized;

            public ConsoleWriter()
            {
                // Attach to /dev/cons on first use
                // Pebble token flows through automatically
                if (!_initialized)
                {
                    _fid = P9Internal.Attach("/dev/cons");
                    _initialized = true;
                }
            }

            public override void Write(char value)
            {
                byte[] buf = new byte[1];
                buf[0] = (byte)value;
                P9Internal.Write(_fid, buf, 0, 1, 0);
            }

            public override void Write(string value)
            {
                if (value == null || value.Length == 0)
                    return;

                // Convert string to UTF-8 bytes
                // Simple ASCII conversion for kernel context
                byte[] buf = new byte[value.Length];
                for (int i = 0; i < value.Length; i++)
                {
                    buf[i] = (byte)value[i];
                }

                P9Internal.Write(_fid, buf, 0, buf.Length, 0);
            }
        }

        class ConsoleReader : TextReader
        {
            private uint _fid;
            private static bool _initialized;
            private long _position;

            public ConsoleReader()
            {
                // Attach to /dev/cons for reading on first use
                if (!_initialized)
                {
                    _fid = P9Internal.Attach("/dev/cons");
                    _initialized = true;
                }
                _position = 0;
            }

            public override int Read()
            {
                byte[] buf = new byte[1];
                int n = P9Internal.Read(_fid, buf, 0, 1, _position);
                if (n <= 0) return -1;
                _position++;
                return buf[0];
            }

            public override string ReadLine()
            {
                // Read characters until newline or EOF
                // Using simple buffer approach for minimal BCL
                byte[] buf = new byte[1024];
                int len = 0;

                while (len < buf.Length - 1)
                {
                    int c = Read();
                    if (c < 0) // EOF
                    {
                        if (len == 0) return null;
                        break;
                    }

                    if (c == '\n')
                        break;
                    
                    if (c == '\r')
                        continue; // Skip carriage return

                    buf[len++] = (byte)c;
                }

                // Convert bytes to string
                char[] chars = new char[len];
                for (int i = 0; i < len; i++)
                {
                    chars[i] = (char)buf[i];
                }
                return new string(chars);
            }
        }
    }

    // TextReader abstract base class for Console.In
    public abstract class TextReader : IDisposable
    {
        public abstract int Read();
        public abstract string ReadLine();
        public virtual void Dispose() { }
    }
}
