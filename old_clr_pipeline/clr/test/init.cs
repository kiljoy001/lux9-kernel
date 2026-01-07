// init.cs - Minimal CLR init program for Lux9 kernel
// Tests Kernel Profile Collections

using System;
using System.Collections.Generic;

class Init
{
    static int Main()
    {
        Console.WriteLine("Lux9 CLR: Starting Collections Test...");
        bool pass = true;
        
        try
        {
            pass &= TestList();
            pass &= TestDictionary();
        }
        catch (Exception e)
        {
            Console.WriteLine("Test Suite Exception: " + e.ToString());
            pass = false;
        }

        if (pass)
        {
            Console.WriteLine("ALL TESTS PASSED");
            return 0;
        }
        else
        {
            Console.WriteLine("SOME TESTS FAILED");
            return 1;
        }
    }

    static bool TestList()
    {
        Console.WriteLine("Testing List<T>...");
        var list = new List<int>();
        
        // Test Add
        list.Add(10);
        list.Add(20);
        list.Add(30);
        
        if (list.Count != 3) { Console.WriteLine("FAIL: Count should be 3"); return false; }
        if (list[0] != 10) { Console.WriteLine("FAIL: item 0 incorrect"); return false; }
        if (list[2] != 30) { Console.WriteLine("FAIL: item 2 incorrect"); return false; }
        
        // Test Remove
        list.Remove(20);
        if (list.Count != 2) { Console.WriteLine("FAIL: Count after remove should be 2"); return false; }
        if (list[1] != 30) { Console.WriteLine("FAIL: item 1 should be 30"); return false; }
        
        // Test IndexOf
        if (list.IndexOf(30) != 1) { Console.WriteLine("FAIL: IndexOf incorrect"); return false; }
        
        // Test Foreach
        int sum = 0;
        foreach(int val in list) sum += val;
        
        if (sum != 40) { Console.WriteLine("FAIL: Sum should be 10+30=40"); return false; }
        
        Console.WriteLine("List<T> PASS");
        return true;
    }

    static bool TestDictionary()
    {
        Console.WriteLine("Testing Dictionary<TKey, TValue>...");
        var dict = new Dictionary<string, int>();
        
        // Test Add
        dict.Add("one", 1);
        dict.Add("two", 2);
        dict["three"] = 3;
        
        if (dict.Count != 3) { Console.WriteLine("FAIL: Count should be 3"); return false; }
        
        // Test ContainsKey
        if (!dict.ContainsKey("two")) { Console.WriteLine("FAIL: Should contain 'two'"); return false; }
        if (dict.ContainsKey("four")) { Console.WriteLine("FAIL: Should not contain 'four'"); return false; }
        
        // Test Get
        if (dict["one"] != 1) { Console.WriteLine("FAIL: Value for 'one' incorrect"); return false; }
        
        // Test TryGetValue
        int val;
        if (!dict.TryGetValue("three", out val) || val != 3) { Console.WriteLine("FAIL: TryGetValue failed"); return false; }
        
        // Test Remove
        dict.Remove("two");
        if (dict.Count != 2) { Console.WriteLine("FAIL: Count after remove should be 2"); return false; }
        if (dict.ContainsKey("two")) { Console.WriteLine("FAIL: Should not contain 'two' after remove"); return false; }
        
        // Test Re-add (collision handling check implied if hash behaves safely)
        dict.Add("two", 22);
        if (dict["two"] != 22) { Console.WriteLine("FAIL: Re-added value incorrect"); return false; }
        
        Console.WriteLine("Dictionary PASS");
        return true;
    }
}
