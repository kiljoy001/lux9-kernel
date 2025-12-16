using System;
using System.Collections.Generic;

// Performance Baseline Test
class PerformanceTest
{
    [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)]
    public static extern void Write(string s);

    public static void Main()
    {
        Write("=== PERFORMANCE BASELINE TEST ===\n");
        
        // Performance Test 1: String operations
        Write("Performance Test 1: String concatenation...\n");
        string testString = "";
        int iterations = 1000;
        
        for (int i = 0; i < iterations; i++)
        {
            testString += "x";
        }
        Write($"  Completed {iterations} string concatenations\n");
        
        // Performance Test 2: Loop performance
        Write("Performance Test 2: Loop operations...\n");
        int sum = 0;
        for (int i = 0; i < 10000; i++)
        {
            sum += i;
        }
        Write($"  Loop sum result: {sum}\n");
        
        // Performance Test 3: Array operations
        Write("Performance Test 3: Array operations...\n");
        int[] array = new int[1000];
        for (int i = 0; i < array.Length; i++)
        {
            array[i] = i * 2;
        }
        
        int arraySum = 0;
        for (int i = 0; i < array.Length; i++)
        {
            arraySum += array[i];
        }
        Write($"  Array sum: {arraySum}\n");
        
        // Performance Test 4: List operations
        Write("Performance Test 4: List operations...\n");
        List<int> list = new List<int>();
        for (int i = 0; i < 500; i++)
        {
            list.Add(i);
        }
        
        int listSum = 0;
        foreach (int item in list)
        {
            listSum += item;
        }
        Write($"  List sum: {listSum}\n");
        
        // Performance Test 5: Method call overhead
        Write("Performance Test 5: Method call overhead...\n");
        int methodResult = 0;
        for (int i = 0; i < 1000; i++)
        {
            methodResult += SimpleMethod(i);
        }
        Write($"  Method call result: {methodResult}\n");
        
        Write("PERFORMANCE TEST: PASSED\n");
    }
    
    static int SimpleMethod(int input)
    {
        return input * input;
    }
}
