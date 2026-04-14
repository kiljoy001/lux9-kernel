// Hello.cs - Hello World example for bflat on Lux9
// Compile with: bflat build Hello.cs --os:uefi (or custom lux9 target)

using Lux9;

class Program
{
    static void Main()
    {
        Console.WriteLine("Hello from C# on Lux9!");
        Console.WriteLine("This was compiled with bflat - no .NET runtime!");
        
        // Demo: Get current time
        ulong nsec = Process.Nsec();
        Console.WriteLine($"Current time: {nsec} ns");
        
        // Demo: Read a file
        string content = File.ReadAllText("/dev/hostowner");
        if (content != null)
        {
            Console.WriteLine($"Host owner: {content}");
        }
        
        Console.WriteLine("Goodbye!");
        Process.Exit(null);
    }
}
