namespace System
{
    using System.Runtime.CompilerServices;

    public static class Environment
    {
        public static string NewLine => "\n";
        
        public static string MachineName => "Lux9Kernel";

        public static extern int TickCount {
            [MethodImpl(MethodImplOptions.InternalCall)]
            get;
        }
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Exit(int exitCode);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void FailFast(string message);
    }
}
