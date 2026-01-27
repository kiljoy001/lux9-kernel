using System;
using System.IO;

class GenHeader
{
    static void Main(string[] args)
    {
        if (args.Length < 4)
        {
            Console.Error.WriteLine("Usage: GenHeader <text.bin> <data.bin> <output_header.bin> <entry>");
            return;
        }

        string textPath = args[0];
        string dataPath = args[1];
        string outPath = args[2];
        uint entry = uint.Parse(args[3].Replace("0x", ""), System.Globalization.NumberStyles.HexNumber);

        long textSize = new FileInfo(textPath).Length;
        long dataSize = new FileInfo(dataPath).Length;
        
        // Plan 9 Exec Header (Big Endian)
        using var stream = File.Create(outPath);
        using var writer = new BinaryWriter(stream);
        
        WriteBE32(writer, 0x8A97); // S_MAGIC (amd64)
        WriteBE32(writer, (uint)textSize);
        WriteBE32(writer, (uint)dataSize);
        WriteBE32(writer, 0); // BSS (Assume 0 for now)
        WriteBE32(writer, 0); // Syms (0 for now)
        WriteBE32(writer, entry); // Entry (32-bit)
        WriteBE32(writer, 0); // spsz
        WriteBE32(writer, 0); // pcsz
        
        // Extended Header (40 bytes total, so 8 bytes extra)
        // 32 bytes written so far. 8 bytes left.
        // S_MAGIC implies 40 bytes header?
        // Wait. "The header... contains 4-byte integers... Exec { ... pcsz }". (32 bytes).
        // Spec says: "S_MAGIC ... extended header... 8 bytes entry point".
        // Man page says "Header ... 4-byte integers".
        // It doesn't mention 40 bytes explicitly in the struct.
        // But 9front S_MAGIC DOES require it.
        // We write 64-bit enty.
        
        WriteBE32(writer, 0); // High 32 entry
        WriteBE32(writer, entry); // Low 32 entry (dup)
    }

    static void WriteBE32(BinaryWriter w, uint v)
    {
        w.Write((byte)(v >> 24));
        w.Write((byte)(v >> 16));
        w.Write((byte)(v >> 8));
        w.Write((byte)v);
    }
}
