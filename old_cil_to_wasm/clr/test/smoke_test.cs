// Minimal CLR Smoke Test
// Tests: arithmetic, control flow, method calls, arrays

public static class SmokeTest
{
    public static int Add(int a, int b)
    {
        return a + b;
    }

    public static int Factorial(int n)
    {
        if (n <= 1)
            return 1;
        return n * Factorial(n - 1);
    }

    public static int SumArray(int[] arr)
    {
        int sum = 0;
        for (int i = 0; i < arr.Length; i++)
        {
            sum = sum + arr[i];
        }
        return sum;
    }

    public static int Main()
    {
        // Test 1: Simple addition
        int result1 = Add(2, 3);  // Should be 5

        // Test 2: Recursion
        int result2 = Factorial(5);  // Should be 120

        // Test 3: Arrays and loops
        int[] numbers = new int[] { 1, 2, 3, 4, 5 };
        int result3 = SumArray(numbers);  // Should be 15

        // Return sum of all results
        return result1 + result2 + result3;  // Should be 140
    }
}
