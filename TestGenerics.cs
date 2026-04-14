using System;
using System.Collections.Generic; // For List<T>

public class TestGenerics {
    public static int Main() {
        // Test 1: List<int> allocation
        List<int> intList = new List<int>();
        intList.Add(10); // This will involve boxing int to object if List<T> is not specialized
        intList.Add(20);

        if (intList.Count != 2) {
            return 1; // Failed Count
        }

        // Test 2: Boxing/Unboxing of a value type
        int x = 5;
        object boxedX = x; // Should use BOX opcode
        
        if (!(boxedX is int)) { // Should use ISINST opcode
            return 2; // Failed type check
        }

        int unboxedX = (int)boxedX; // Should use UNBOX opcode
        if (unboxedX != 5) {
            return 3; // Failed unboxing value
        }

        // Test 3: Simple string (uses LDSTR)
        string hello = "Hello from CLR kernel!";
        if (hello.Length == 0) { // Will involve loading field (Length)
             return 4;
        }

        // Test 4: Internal Call (if Lux9Kernel is hooked up)
        // This won't compile without Lux9Kernel reference, commenting out for now
        // Lux9.Kernel.Kernel.Print(hello);

        return 0; // Success
    }
}
