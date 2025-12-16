using System;
using System.Collections.Generic;

// Smoke Test 1: Basic Console Output and Math Operations
class SmokeTest
{
    [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)]
    public static extern void Write(string s);

    public static void Main()
    {
        Write("=== C# INIT SMOKE TEST 1: Basic Operations ===\n");
        
        // Test 1: String operations
        Write("Test 1: String concatenation...\n");
        string greeting = "Hello" + " " + "World";
        Write(greeting + "\n");
        
        // Test 2: Integer math
        Write("Test 2: Math operations...\n");
        int a = 42, b = 8;
        Write($"a = {a}, b = {b}\n");
        Write($"a + b = {a + b}\n");
        Write($"a - b = {a - b}\n");
        Write($"a * b = {a * b}\n");
        Write($"a / b = {a / b}\n");
        Write($"a % b = {a % b}\n");
        
        // Test 3: Boolean logic
        Write("Test 3: Boolean operations...\n");
        bool x = true, y = false;
        Write($"x && y = {x && y}\n");
        Write($"x || y = {x || y}\n");
        Write($"!x = {!x}\n");
        
        Write("SMOKE TEST 1: PASSED\n");
    }
}
