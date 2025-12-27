using System;
using System.Collections.Generic;
using System.IO;
using System.Numerics;

namespace BCL.Integration
{
    class Program
    {
        static void Main()
        {
            // Test 1: Core Types and Console
            Console.WriteLine("=== Lux9 BCL Integration Test (C#) ===");
            Console.WriteLine("String concatenation " + "works!");
            
            // Test 2: Collections
            List<int> list = new List<int>();
            list.Add(10);
            list.Add(20);
            Console.WriteLine("List Count should be 2: " + list.Count.ToString());
            
            // Test 3: Boxing/Unboxing
            object obj = 42;
            Console.Write("Boxed int: ");
            Console.WriteLine(obj);
            
            // Test 4: Numerics
            BigInteger big = new BigInteger(123456789);
            Console.Write("BigInteger: ");
            Console.WriteLine(big.ToString());
            
            // Test 5: IO
            using (MemoryStream mem = new MemoryStream())
            {
                mem.WriteByte(65); // 'A'
                mem.Position = 0;
                int read = mem.ReadByte();
                Console.Write("MemoryStream read: ");
                Console.WriteLine(read.ToString());
            }

            Console.WriteLine("=== Tests Complete ===");
        }
    }
}
