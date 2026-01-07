// Lux9.Console.cs - Console I/O for Lux9
// High-level wrapper over syscalls for stdout/stderr

using System;
using System.Runtime.InteropServices;
using System.Text;

namespace Lux9
{
    /// <summary>
    /// Console output for Lux9.
    /// Stdout = fd 1, Stderr = fd 2
    /// </summary>
    public static class Console
    {
        private const int Stdout = 1;
        private const int Stderr = 2;

        /// <summary>
        /// Write a string to stdout with newline.
        /// </summary>
        public static void WriteLine(string message)
        {
            Write(message + "\n");
        }

        /// <summary>
        /// Write a string to stdout.
        /// </summary>
        public static unsafe void Write(string message)
        {
            if (message == null) return;

            // Convert to UTF-8 bytes
            int byteCount = Encoding.UTF8.GetByteCount(message);
            byte* buffer = stackalloc byte[byteCount];
            
            fixed (char* chars = message)
            {
                Encoding.UTF8.GetBytes(chars, message.Length, buffer, byteCount);
            }

            Syscalls.Write(Stdout, buffer, byteCount);
        }

        /// <summary>
        /// Write a string to stderr with newline.
        /// </summary>
        public static void ErrorLine(string message)
        {
            Error(message + "\n");
        }

        /// <summary>
        /// Write a string to stderr.
        /// </summary>
        public static unsafe void Error(string message)
        {
            if (message == null) return;

            int byteCount = Encoding.UTF8.GetByteCount(message);
            byte* buffer = stackalloc byte[byteCount];
            
            fixed (char* chars = message)
            {
                Encoding.UTF8.GetBytes(chars, message.Length, buffer, byteCount);
            }

            Syscalls.Write(Stderr, buffer, byteCount);
        }
    }
}
