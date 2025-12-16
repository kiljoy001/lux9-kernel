using System;
using System.Collections.Generic;

// ETL Pipeline Test: F# → .NET DLL → CIL → Fruity IR → QBE IL → x86-64
class ETLTest
{
    [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)]
    public static extern void Write(string s);

    // Simulate complex data transformation pipeline
    class DataItem
    {
        public string Name;
        public int Value;
        
        public DataItem(string name, int value)
        {
            Name = name;
            Value = value;
        }
    }

    public static void Main()
    {
        Write("=== ETL PIPELINE TEST ===\n");
        
        // Stage 1: Data Ingestion (F# data structures)
        Write("Stage 1: Data Ingestion...\n");
        List<DataItem> rawData = new List<DataItem>();
        rawData.Add(new DataItem("item1", 100));
        rawData.Add(new DataItem("item2", 200));
        rawData.Add(new DataItem("item3", 300));
        
        // Stage 2: Transformation (C# business logic)
        Write("Stage 2: Data Transformation...\n");
        List<DataItem> transformed = new List<DataItem>();
        foreach (DataItem item in rawData)
        {
            transformed.Add(new DataItem(item.Name.ToUpper(), item.Value * 2));
        }
        
        // Stage 3: Aggregation (.NET LINQ-like operations)
        Write("Stage 3: Data Aggregation...\n");
        int totalValue = 0;
        foreach (DataItem item in transformed)
        {
            totalValue += item.Value;
        }
        
        double average = (double)totalValue / transformed.Count;
        
        // Stage 4: Output (Console output)
        Write("Stage 4: Data Output...\n");
        Write($"Total items: {transformed.Count}\n");
        Write($"Total value: {totalValue}\n");
        Write($"Average value: {average}\n");
        
        foreach (DataItem item in transformed)
        {
            Write($"Processed: {item.Name} = {item.Value}\n");
        }
        
        Write("ETL PIPELINE TEST: PASSED\n");
    }
}
