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

        public static void WriteLine(string value) => Internal_WriteLine(value);
        public static void WriteLine(object value) => Internal_WriteLine(value?.ToString());
        
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)]
        extern static void Internal_WriteLine(string value);
        
        public static void Write(string value) => Out.Write(value);
        public static void Write(object value) => Out.Write(value?.ToString());

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
    }
}
