using System;
using System.Collections.Generic;

// Collections and Data Structures Test
class CollectionsTest
{
    [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)]
    public static extern void Write(string s);

    public static void Main()
    {
        Write("=== COLLECTIONS AND DATA STRUCTURES TEST ===\n");
        
        // Test 1: List operations
        Write("Test 1: Generic List operations...\n");
        List<string> names = new List<string>();
        names.Add("Alice");
        names.Add("Bob");
        names.Add("Charlie");
        
        Write($"  List count: {names.Count}\n");
        Write($"  First item: {names[0]}\n");
        Write($"  Last item: {names[names.Count - 1]}\n");
        
        // Test 2: Dictionary operations
        Write("Test 2: Dictionary operations...\n");
        Dictionary<string, int> ages = new Dictionary<string, int>();
        ages["Alice"] = 25;
        ages["Bob"] = 30;
        ages["Charlie"] = 35;
        
        Write($"  Alice's age: {ages["Alice"]}\n");
        Write($"  Dictionary count: {ages.Count}\n");
        
        // Test 3: Array operations
        Write("Test 3: Array operations...\n");
        int[] numbers = {10, 20, 30, 40, 50};
        Write($"  Array length: {numbers.Length}\n");
        Write($"  Sum of array: {SumArray(numbers)}\n");
        
        // Test 4: Stack simulation
        Write("Test 4: Stack simulation...\n");
        Stack<int> stack = new Stack<int>();
        stack.Push(100);
        stack.Push(200);
        stack.Push(300);
        
        Write($"  Stack count: {stack.Count}\n");
        Write($"  Top item: {stack.Peek()}\n");
        Write($"  Popped: {stack.Pop()}\n");
        Write($"  Stack count after pop: {stack.Count}\n");
        
        // Test 5: Queue simulation
        Write("Test 5: Queue simulation...\n");
        Queue<string> queue = new Queue<string>();
        queue.Enqueue("First");
        queue.Enqueue("Second");
        queue.Enqueue("Third");
        
        Write($"  Queue count: {queue.Count}\n");
        Write($"  Front item: {queue.Peek()}\n");
        Write($"  Dequeued: {queue.Dequeue()}\n");
        Write($"  Queue count after dequeue: {queue.Count}\n");
        
        Write("COLLECTIONS TEST: PASSED\n");
    }
    
    static int SumArray(int[] arr)
    {
        int sum = 0;
        for (int i = 0; i < arr.Length; i++)
        {
            sum += arr[i];
        }
        return sum;
    }
}
