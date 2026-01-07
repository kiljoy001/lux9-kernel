using System;

class Program
{
    static int Main(string[] args)
    {
        if (args.Length < 2)
        {
            Console.Error.WriteLine("Usage: elf2plan9-cs <input.elf> <output.out> [entryPoint]");
            return 1;
        }

        string input = args[0];
        string output = args[1];
        ulong entry = 0;
        if (args.Length > 2) ulong.TryParse(args[2], out entry); // Support optional entry override

        try
        {
            Plan9ObjectWriter.ConvertElfToPlan9(input, output, entry);
            Console.WriteLine($"Converted {input} -> {output}");
            return 0;
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"Error: {ex.Message}");
            return 1;
        }
    }
}
