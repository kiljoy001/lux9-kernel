namespace System.P9
{
    using System.Runtime.CompilerServices;
    using System.Runtime.InteropServices;

    /// <summary>
    /// Opaque capability representing an exchange page with the kernel.
    /// Cannot be forged, dereferenced, or manipulated.
    /// Possession of capability = authority to use the page.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct ExchangePageCap
    {
        private readonly uint handle;  // Opaque kernel handle - DO NOT EXPOSE

        // No public constructor - can only be created by kernel
        internal ExchangePageCap(uint kernelHandle)
        {
            handle = kernelHandle;
        }

        public bool IsValid => handle != 0;
    }

    /// <summary>
    /// 9P Exchange Page interface - ALL processes communicate directly with kernel.
    ///
    /// SIMPLIFIED MODEL:
    /// - Each process gets ONE exchange page with kernel (not with init)
    /// - Processes send 9P messages directly to kernel
    /// - Kernel validates capabilities on every operation
    /// - No init middleman, no routing overhead
    ///
    /// Init is just a regular process that spawns children and reaps zombies.
    /// </summary>
    public static class ExchangePage
    {
        // Layout constants
        public const int PageSize = 8192;
        public const int RequestOffset = 0x000;
        public const int RequestSize = 0xF00;
        public const int ReplyOffset = 0x1000;
        public const int ReplySize = 0x1000;
        public const int ControlOffset = 0xF00;

        // Status codes
        public const uint STATUS_IDLE = 0;
        public const uint STATUS_PENDING = 1;
        public const uint STATUS_COMPLETE = 2;
        public const uint STATUS_ERROR = 3;

        // ============================================
        // EVERY PROCESS: Get exchange page capability
        // ============================================

        /// <summary>
        /// Get capability for THIS process's exchange page with kernel.
        /// Kernel creates the page and maps it to the process.
        /// Returns an opaque capability (NOT a raw pointer).
        /// </summary>
        /// <returns>Capability for exchange page</returns>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern ExchangePageCap GetMyPage();

        // ============================================
        // SEND/RECEIVE: Talk directly to kernel
        // ============================================

        /// <summary>
        /// Send 9P message to kernel and wait for reply.
        ///
        /// FLOW:
        ///   1. Write request to exchange page
        ///   2. Ring doorbell
        ///   3. Kernel validates capability
        ///   4. Kernel processes 9P message
        ///   5. Kernel writes reply
        ///   6. Return to caller
        /// </summary>
        /// <param name="pageCap">Capability for exchange page</param>
        /// <param name="request">9P request message</param>
        /// <param name="requestLen">Request length</param>
        /// <param name="reply">Buffer for reply</param>
        /// <param name="replyMaxLen">Max reply size</param>
        /// <returns>Reply length, or -1 on error</returns>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern int SendMessage(
            ExchangePageCap pageCap,
            byte[] request,
            int requestLen,
            byte[] reply,
            int replyMaxLen);

        // ============================================
        // LOW-LEVEL: For advanced scenarios
        // ============================================

        /// <summary>
        /// Write message to exchange page.
        /// Kernel validates capability and enforces bounds.
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool WriteRequest(
            ExchangePageCap pageCap,
            byte[] buffer,
            int offset,
            int length);

        /// <summary>
        /// Read reply from exchange page.
        /// Kernel validates capability and enforces bounds.
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern int ReadReply(
            ExchangePageCap pageCap,
            byte[] buffer,
            int offset,
            int maxLen);

        /// <summary>
        /// Ring doorbell to notify kernel.
        /// Kernel validates capability.
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void RingDoorbell(ExchangePageCap pageCap);

        /// <summary>
        /// Wait for kernel to complete processing.
        /// Blocks until status becomes COMPLETE or ERROR.
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern uint WaitForComplete(ExchangePageCap pageCap);
    }

    /// <summary>
    /// 9P message types (from fcall.h)
    /// </summary>
    public static class P9MessageType
    {
        public const byte Tversion = 100;
        public const byte Rversion = 101;
        public const byte Tauth = 102;
        public const byte Rauth = 103;
        public const byte Tattach = 104;
        public const byte Rattach = 105;
        public const byte Terror = 106;  // Illegal
        public const byte Rerror = 107;
        public const byte Tflush = 108;
        public const byte Rflush = 109;
        public const byte Twalk = 110;
        public const byte Rwalk = 111;
        public const byte Topen = 112;
        public const byte Ropen = 113;
        public const byte Tcreate = 114;
        public const byte Rcreate = 115;
        public const byte Tread = 116;
        public const byte Rread = 117;
        public const byte Twrite = 118;
        public const byte Rwrite = 119;
        public const byte Tclunk = 120;
        public const byte Rclunk = 121;
        public const byte Tremove = 122;
        public const byte Rremove = 123;
        public const byte Tstat = 124;
        public const byte Rstat = 125;
        public const byte Twstat = 126;
        public const byte Rwstat = 127;
    }
}
