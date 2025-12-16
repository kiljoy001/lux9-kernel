using System;

/// <summary>
/// Minimal init - just print and loop forever
/// Phase 1: Get this to boot and print "INIT ALIVE"
/// Phase 2: Add 9P communication
/// Phase 3: Add namespace setup
/// </summary>
class Init
{
    static void Main(string[] args)
    {
        Console.WriteLine("=== Lux9 Init (Minimal) ===");
        Console.WriteLine("PID: 1 (hopefully!)");
        Console.WriteLine("Phase 1: Just stay alive");
        Console.WriteLine();

        // Loop forever - traditional init behavior
        // TODO: Reap zombies
        int tick = 0;
        while (true)
        {
            if (tick % 10 == 0)
            {
                Console.WriteLine($"[INIT] Alive: tick={tick}");
            }

            System.Threading.Thread.Sleep(1000);
            tick++;
        }
    }
}
