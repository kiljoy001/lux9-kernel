using System;

// Control Flow Test: If/Else, Loops, Switch
class ControlFlowTest
{
    [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)]
    public static extern void Write(string s);

    public static void Main()
    {
        Write("=== CONTROL FLOW TEST ===\n");
        
        // Test 1: If/Else
        Write("Test 1: Conditional statements...\n");
        int score = 85;
        if (score >= 90)
        {
            Write("Grade: A\n");
        }
        else if (score >= 80)
        {
            Write("Grade: B\n");
        }
        else if (score >= 70)
        {
            Write("Grade: C\n");
        }
        else
        {
            Write("Grade: F\n");
        }
        
        // Test 2: For loop
        Write("Test 2: For loop...\n");
        Write("Counting 1 to 5: ");
        for (int i = 1; i <= 5; i++)
        {
            Write(i.ToString() + " ");
        }
        Write("\n");
        
        // Test 3: While loop
        Write("Test 3: While loop...\n");
        int countdown = 3;
        while (countdown > 0)
        {
            Write($"Countdown: {countdown}\n");
            countdown--;
        }
        Write("Blast off!\n");
        
        // Test 4: Switch statement
        Write("Test 4: Switch statement...\n");
        int day = 3;
        string dayName;
        switch (day)
        {
            case 1: dayName = "Monday"; break;
            case 2: dayName = "Tuesday"; break;
            case 3: dayName = "Wednesday"; break;
            case 4: dayName = "Thursday"; break;
            case 5: dayName = "Friday"; break;
            default: dayName = "Weekend"; break;
        }
        Write($"Day {day} is {dayName}\n");
        
        Write("CONTROL FLOW TEST: PASSED\n");
    }
}
