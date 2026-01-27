// Lux9.Process.cs - Process control for Lux9

using System;
using System.Text;

namespace Lux9
{
    /// <summary>
    /// Process control and lifecycle for Lux9.
    /// </summary>
    public static class Process
    {
        /// <summary>
        /// Exit the current process.
        /// </summary>
        public static unsafe void Exit(string message = null)
        {
            if (message == null)
            {
                Syscalls.Exit(null);
            }
            else
            {
                int byteCount = Encoding.UTF8.GetByteCount(message) + 1;
                byte* buffer = stackalloc byte[byteCount];
                
                fixed (char* chars = message)
                {
                    Encoding.UTF8.GetBytes(chars, message.Length, buffer, byteCount - 1);
                }
                buffer[byteCount - 1] = 0;
                
                Syscalls.Exit(buffer);
            }
        }

        /// <summary>
        /// Fork the current process.
        /// Returns: 0 in child, child PID in parent, -1 on error.
        /// </summary>
        public static int Fork(int flags = Syscalls.RFPROC)
        {
            return Syscalls.Rfork(flags);
        }

        /// <summary>
        /// Execute a new program in the current process.
        /// Does not return on success.
        /// </summary>
        public static unsafe void Exec(string path)
        {
            int byteCount = Encoding.UTF8.GetByteCount(path) + 1;
            byte* buffer = stackalloc byte[byteCount];
            
            fixed (char* chars = path)
            {
                Encoding.UTF8.GetBytes(chars, path.Length, buffer, byteCount - 1);
            }
            buffer[byteCount - 1] = 0;
            
            Syscalls.Exec(buffer);
        }

        /// <summary>
        /// Wait for a child process to exit.
        /// Returns child PID.
        /// </summary>
        public static int Wait()
        {
            return Syscalls.Wait();
        }

        /// <summary>
        /// Sleep for milliseconds.
        /// </summary>
        public static void Sleep(long ms)
        {
            Syscalls.Sleep(ms);
        }

        /// <summary>
        /// Get current nanosecond timestamp.
        /// </summary>
        public static ulong Nsec()
        {
            return Syscalls.Nsec();
        }
    }
}
