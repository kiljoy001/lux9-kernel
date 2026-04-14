// Lux9.Syscalls.cs - Low-level syscall bindings for Lux9
// These P/Invoke to the C implementations in liblux

using System;
using System.Runtime.InteropServices;

namespace Lux9
{
    /// <summary>
    /// Low-level Lux9 system calls via P/Invoke to liblux.
    /// All I/O goes through 9P messages on the exchange page.
    /// </summary>
    public static unsafe class Syscalls
    {
        private const string LibLux = "liblux";

        [DllImport(LibLux, EntryPoint = "sys_open")]
        public static extern int Open(byte* path, int mode);

        [DllImport(LibLux, EntryPoint = "sys_close")]
        public static extern int Close(int fd);

        [DllImport(LibLux, EntryPoint = "sys_read")]
        public static extern long Read(int fd, byte* buf, long n);

        [DllImport(LibLux, EntryPoint = "sys_write")]
        public static extern long Write(int fd, byte* buf, long n);

        [DllImport(LibLux, EntryPoint = "sys_pread")]
        public static extern long Pread(int fd, byte* buf, long n, long offset);

        [DllImport(LibLux, EntryPoint = "sys_pwrite")]
        public static extern long Pwrite(int fd, byte* buf, long n, long offset);

        [DllImport(LibLux, EntryPoint = "sys_create")]
        public static extern int Create(byte* path, int mode, uint perm);

        [DllImport(LibLux, EntryPoint = "sys_exit")]
        public static extern void Exit(byte* msg);

        [DllImport(LibLux, EntryPoint = "sys_rfork")]
        public static extern int Rfork(int flags);

        [DllImport(LibLux, EntryPoint = "sys_exec")]
        public static extern void Exec(byte* path);

        [DllImport(LibLux, EntryPoint = "sys_pipe")]
        public static extern int Pipe(int* fds);

        [DllImport(LibLux, EntryPoint = "sys_seek")]
        public static extern long Seek(int fd, long offset, int whence);

        [DllImport(LibLux, EntryPoint = "sys_wait")]
        public static extern int Wait();

        [DllImport(LibLux, EntryPoint = "sys_stat")]
        public static extern int Stat(byte* path, byte* buf, int nbuf);

        [DllImport(LibLux, EntryPoint = "sys_mount")]
        public static extern int Mount(int fd, int afd, byte* old, int flags, byte* aname);

        [DllImport(LibLux, EntryPoint = "sys_sleep")]
        public static extern int Sleep(long ms);

        [DllImport(LibLux, EntryPoint = "sys_nsec")]
        public static extern ulong Nsec();

        // RFORK flags
        public const int RFPROC = 1 << 4;
        public const int RFMEM = 1 << 5;
        public const int RFNOWAIT = 1 << 6;
    }
}
