namespace System.P9
{
    using System.Runtime.CompilerServices;

    // Internal P9 protocol interface - NOT part of public API
    // All standard .NET APIs (FileStream, Console, Process) use this internally
    internal static class P9Internal
    {
        // Attach to a 9P resource (Tattach)
        // Returns fid (file identifier) for subsequent operations
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern uint Attach(string path);

        // Read from fid (Tread)
        // Returns number of bytes actually read
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern int Read(uint fid, byte[] buffer, int offset, int count, long position);

        // Write to fid (Twrite)
        // Returns number of bytes actually written
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern int Write(uint fid, byte[] buffer, int offset, int count, long position);

        // Close fid (Tclunk)
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Clunk(uint fid);

        // Get file statistics (Tstat)
        // Returns length, or -1 on error
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern long Stat(uint fid);
    }
}
