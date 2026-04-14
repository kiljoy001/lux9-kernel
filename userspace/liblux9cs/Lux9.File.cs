// Lux9.File.cs - File I/O for Lux9
// Safe wrappers around file syscalls

using System;
using System.Text;

namespace Lux9
{
    /// <summary>
    /// File operations for Lux9.
    /// All paths are 9P paths (e.g., /dev/cons, /usr/hello.txt)
    /// </summary>
    public static class File
    {
        // Open modes
        public const int OREAD = 0;
        public const int OWRITE = 1;
        public const int ORDWR = 2;
        public const int OTRUNC = 16;

        /// <summary>
        /// Open a file and return file descriptor.
        /// </summary>
        public static unsafe int Open(string path, int mode = OREAD)
        {
            int byteCount = Encoding.UTF8.GetByteCount(path) + 1;
            byte* buffer = stackalloc byte[byteCount];
            
            fixed (char* chars = path)
            {
                Encoding.UTF8.GetBytes(chars, path.Length, buffer, byteCount - 1);
            }
            buffer[byteCount - 1] = 0; // null terminate

            return Syscalls.Open(buffer, mode);
        }

        /// <summary>
        /// Close a file descriptor.
        /// </summary>
        public static int Close(int fd)
        {
            return Syscalls.Close(fd);
        }

        /// <summary>
        /// Read from a file descriptor into a byte array.
        /// Returns number of bytes read.
        /// </summary>
        public static unsafe long Read(int fd, byte[] buffer, int offset, int count)
        {
            if (buffer == null || offset < 0 || count < 0 || offset + count > buffer.Length)
                return -1;

            fixed (byte* ptr = &buffer[offset])
            {
                return Syscalls.Read(fd, ptr, count);
            }
        }

        /// <summary>
        /// Write to a file descriptor from a byte array.
        /// Returns number of bytes written.
        /// </summary>
        public static unsafe long Write(int fd, byte[] buffer, int offset, int count)
        {
            if (buffer == null || offset < 0 || count < 0 || offset + count > buffer.Length)
                return -1;

            fixed (byte* ptr = &buffer[offset])
            {
                return Syscalls.Write(fd, ptr, count);
            }
        }

        /// <summary>
        /// Write a string to a file descriptor.
        /// </summary>
        public static unsafe long Write(int fd, string text)
        {
            int byteCount = Encoding.UTF8.GetByteCount(text);
            byte* buffer = stackalloc byte[byteCount];
            
            fixed (char* chars = text)
            {
                Encoding.UTF8.GetBytes(chars, text.Length, buffer, byteCount);
            }

            return Syscalls.Write(fd, buffer, byteCount);
        }

        /// <summary>
        /// Create a new file.
        /// </summary>
        public static unsafe int Create(string path, int mode = OWRITE, uint perm = 0644)
        {
            int byteCount = Encoding.UTF8.GetByteCount(path) + 1;
            byte* buffer = stackalloc byte[byteCount];
            
            fixed (char* chars = path)
            {
                Encoding.UTF8.GetBytes(chars, path.Length, buffer, byteCount - 1);
            }
            buffer[byteCount - 1] = 0;

            return Syscalls.Create(buffer, mode, perm);
        }

        /// <summary>
        /// Read entire file as string.
        /// </summary>
        public static string ReadAllText(string path)
        {
            int fd = Open(path, OREAD);
            if (fd < 0) return null;

            byte[] buffer = new byte[4096];
            int totalRead = 0;
            
            while (true)
            {
                long n = Read(fd, buffer, totalRead, buffer.Length - totalRead);
                if (n <= 0) break;
                totalRead += (int)n;
                
                // Expand buffer if needed
                if (totalRead >= buffer.Length)
                {
                    byte[] newBuffer = new byte[buffer.Length * 2];
                    Array.Copy(buffer, newBuffer, totalRead);
                    buffer = newBuffer;
                }
            }
            
            Close(fd);
            return Encoding.UTF8.GetString(buffer, 0, totalRead);
        }

        /// <summary>
        /// Write string to file (creates/truncates).
        /// </summary>
        public static bool WriteAllText(string path, string content)
        {
            int fd = Create(path, OWRITE | OTRUNC, 0644);
            if (fd < 0) return false;
            
            long written = Write(fd, content);
            Close(fd);
            
            return written >= 0;
        }
    }
}
