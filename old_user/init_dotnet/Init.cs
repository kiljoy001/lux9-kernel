using System.Runtime.CompilerServices;

class Init
{
    [MethodImpl(MethodImplOptions.InternalCall)]
    public static extern void Write(string s);

    public static void Main()
    {
        Write("Hello AOT World!\n");
        Write("Kernel AOT Init Shim Loaded.\n");
        
        int x = 10;
        int y = 20;
        Write("Math Test: 10 + 20...\n");
        
        if (x + y == 30) {
           Write("Math Check OK.\n");
        } else {
           Write("Math Check FAILED.\n");
        }
    }
}
