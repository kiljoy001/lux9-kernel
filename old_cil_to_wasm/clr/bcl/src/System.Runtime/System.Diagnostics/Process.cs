namespace System.Diagnostics
{
    using System.P9;

    // Process control via /proc/N/ctl
    // Provides standard .NET Process API backed by Plan 9 process filesystem
    public class Process : IDisposable
    {
        private int _pid;
        private uint _ctlFid;
        private bool _disposed;
        private bool _hasControl;

        // Private constructor - use factory methods
        private Process(int pid)
        {
            _pid = pid;
            _disposed = false;
            _hasControl = false;
        }

        public static Process GetCurrentProcess()
        {
            // TODO: Get actual current PID from kernel
            // For now, return process with PID 0 (kernel convention)
            return new Process(0);
        }

        public static Process GetProcessById(int pid)
        {
            var proc = new Process(pid);
            proc.OpenControl();
            return proc;
        }

        private void OpenControl()
        {
            if (_hasControl || _disposed)
                return;

            // Attach to /proc/N/ctl for process control
            string ctlPath = "/proc/" + _pid.ToString() + "/ctl";
            _ctlFid = P9Internal.Attach(ctlPath);
            _hasControl = true;
        }

        private void WriteCommand(string command)
        {
            if (_disposed)
                throw new ObjectDisposedException(nameof(Process));

            if (!_hasControl)
                OpenControl();

            // Convert command to bytes and write to ctl file
            byte[] buf = new byte[command.Length];
            for (int i = 0; i < command.Length; i++)
            {
                buf[i] = (byte)command[i];
            }

            P9Internal.Write(_ctlFid, buf, 0, buf.Length, 0);
        }

        // Kill the process
        public void Kill()
        {
            WriteCommand("kill");
        }

        // Suspend the process
        public void Suspend()
        {
            WriteCommand("stop");
        }

        // Resume the process
        public void Resume()
        {
            WriteCommand("start");
        }

        // Wake up a sleeping process
        public void WakeUp()
        {
            WriteCommand("wakeup");
        }

        // Break the process (force into broken state)
        public void Break()
        {
            WriteCommand("break");
        }

        public int Id => _pid;

        public void Dispose()
        {
            if (!_disposed)
            {
                if (_hasControl)
                {
                    P9Internal.Clunk(_ctlFid);
                }
                _disposed = true;
            }
        }

        /// <summary>
        /// Starts a process resource by specifying the name of a document or application file.
        /// </summary>
        public static Process Start(string fileName, string arguments = "")
        {
            int pid = Internal_Start(fileName, arguments);
            if (pid < 0)
            {
                // Explicitly use Concat to avoid compiler looking for missing overloads
                string msg = String.Concat("Failed to start process '", fileName);
                msg = String.Concat(msg, "': error ");
                msg = String.Concat(msg, pid.ToString());
                throw new Exception(msg);
            }
            return new Process(pid);
        }

        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)]
        private static extern int Internal_Start(string fileName, string arguments);
    }
}
