// Plan 9 a.out Object Writer (Standalone Copy)

using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

// Made public for standalone tool
public static class Plan9ObjectWriter
{
    // Plan 9 a.out header constants
    private const uint AOUT_MAGIC_AMD64 = 0x8A97; // AMD64 Plan 9 Magic (Matches Kernel S_MAGIC)
    
    public static void ConvertElfToPlan9(string elfPath, string outputPath, ulong entryPoint)
    {
        using var elfStream = File.OpenRead(elfPath);
        using var elfReader = new BinaryReader(elfStream);
        
        // Read ELF header
        byte[] magic = elfReader.ReadBytes(4);
        if (magic[0] != 0x7F || magic[1] != 'E' || magic[2] != 'L' || magic[3] != 'F')
            throw new Exception($"{elfPath} is not a valid ELF file");
        
        byte elfClass = elfReader.ReadByte(); 
        if (elfClass != 2) throw new Exception("Only ELF64 is supported for Plan 9 conversion");
        
        elfReader.ReadBytes(11); // Skip rest of e_ident
        
        ushort e_type = elfReader.ReadUInt16();
        ushort e_machine = elfReader.ReadUInt16();
        uint e_version = elfReader.ReadUInt32();
        ulong e_entry = elfReader.ReadUInt64();
        ulong e_phoff = elfReader.ReadUInt64();
        ulong e_shoff = elfReader.ReadUInt64();
        uint e_flags = elfReader.ReadUInt32();
        ushort e_ehsize = elfReader.ReadUInt16();
        ushort e_phentsize = elfReader.ReadUInt16();
        ushort e_phnum = elfReader.ReadUInt16();
        ushort e_shentsize = elfReader.ReadUInt16();
        ushort e_shnum = elfReader.ReadUInt16();
        ushort e_shstrndx = elfReader.ReadUInt16();
        
        // --- SECTIONS ---
        byte[] textData = null;
        byte[] dataData = null;
        ulong textSize = 0;
        ulong dataSize = 0;
        ulong bssSize = 0;
        
        ulong minText = ulong.MaxValue;
        ulong maxText = 0;
        ulong minData = ulong.MaxValue;
        ulong maxData = 0;
        ulong maxDataMem = 0;

        elfStream.Seek((long)e_phoff, SeekOrigin.Begin);
        
        // Pass 1: Calculate Extents
        for (int i = 0; i < e_phnum; i++)
        {
            elfStream.Seek((long)(e_phoff + (ulong)(i * e_phentsize)), SeekOrigin.Begin);
            
            uint p_type = elfReader.ReadUInt32();
            uint p_flags = elfReader.ReadUInt32();
            ulong p_offset = elfReader.ReadUInt64();
            ulong p_vaddr = elfReader.ReadUInt64();
            ulong p_paddr = elfReader.ReadUInt64();
            ulong p_filesz = elfReader.ReadUInt64();
            ulong p_memsz = elfReader.ReadUInt64();
            ulong p_align = elfReader.ReadUInt64();
            
            const uint PT_LOAD = 1;
            const uint PF_X = 0x1;
            const uint PF_W = 0x2;
            
            if (p_type != PT_LOAD) continue;
            
            bool isExec = (p_flags & PF_X) != 0;
            bool isWrite = (p_flags & PF_W) != 0;

            if ((p_flags & PF_W) != 0) // Data Segments (RW or RWE) - Prioritize Write!
            {
                if (p_vaddr < minData) minData = p_vaddr;
                if (p_vaddr + p_filesz > maxData) maxData = p_vaddr + p_filesz;
                if (p_vaddr + p_memsz > maxDataMem) maxDataMem = p_vaddr + p_memsz;
            }
            else if ((p_flags & PF_X) != 0) // Text Segments (RX only)
            {
                if (p_vaddr < minText) minText = p_vaddr;
                if (p_vaddr + p_filesz > maxText) maxText = p_vaddr + p_filesz;
            }
        }
        
        if (minText == ulong.MaxValue) throw new Exception("No text segment found");
        
        textSize = maxText - minText;
        textData = new byte[textSize];
        
        if (minData != ulong.MaxValue)
        {
            dataSize = maxData - minData;
            dataData = new byte[dataSize];
            if (maxDataMem > maxData) bssSize = maxDataMem - maxData;
        }

        // Pass 2: Fill Buffers
        elfStream.Seek((long)e_phoff, SeekOrigin.Begin);
        for (int i = 0; i < e_phnum; i++)
        {
            elfStream.Seek((long)(e_phoff + (ulong)(i * e_phentsize)), SeekOrigin.Begin);
            
            uint p_type = elfReader.ReadUInt32();
            uint p_flags = elfReader.ReadUInt32();
            ulong p_offset = elfReader.ReadUInt64();
            ulong p_vaddr = elfReader.ReadUInt64();
            ulong p_paddr = elfReader.ReadUInt64();
            ulong p_filesz = elfReader.ReadUInt64();
            
            const uint PT_LOAD = 1;
            const uint PF_X = 0x1;
            const uint PF_W = 0x2;
            
            if (p_type != PT_LOAD) continue;
            
            long savePos = elfStream.Position;
            if ((p_flags & PF_W) != 0 && dataData != null)
            {
                elfStream.Seek((long)p_offset, SeekOrigin.Begin);
                // Be careful with overlapping segments or gaps
                if (p_vaddr >= minData && (p_vaddr + p_filesz) <= (minData + dataSize + bssSize)) 
                {
                    // Check bounds to avoid negative index relative to minData
                    int relOffset = (int)(p_vaddr - minData);
                    if (relOffset >= 0 && relOffset + (int)p_filesz <= dataData.Length)
                         elfStream.Read(dataData, relOffset, (int)p_filesz);
                }
            }
            else if ((p_flags & PF_X) != 0)
            {
                elfStream.Seek((long)p_offset, SeekOrigin.Begin);
                 // Only read if it maps to Text range (skip if it was handled as Data)
                if (textData != null && p_vaddr >= minText && (p_vaddr + p_filesz) <= (minText + textSize))
                {
                     elfStream.Read(textData, (int)(p_vaddr - minText), (int)p_filesz);
                }
            }
            elfStream.Seek(savePos, SeekOrigin.Begin);
        }

        // --- SYMBOLS ---
        using var symStream = new MemoryStream();
        using var symWriter = new BinaryWriter(symStream, Encoding.UTF8);
        
        ulong symtabOffset = 0;
        ulong symtabSize = 0;
        ulong strtabOffset = 0;
        
        // Find Symbol Table Headers
        for (int i = 0; i < e_shnum; i++)
        {
            elfStream.Seek((long)(e_shoff + (ulong)(i * e_shentsize)), SeekOrigin.Begin);
            
            uint sh_name = elfReader.ReadUInt32();
            uint sh_type = elfReader.ReadUInt32();
            ulong sh_flags = elfReader.ReadUInt64();
            ulong sh_addr = elfReader.ReadUInt64();
            ulong sh_offset = elfReader.ReadUInt64();
            ulong sh_size = elfReader.ReadUInt64();
            uint sh_link = elfReader.ReadUInt32();
            
            const uint SHT_SYMTAB = 2;
            const uint SHT_STRTAB = 3;
            
            if (sh_type == SHT_SYMTAB)
            {
                symtabOffset = sh_offset;
                symtabSize = sh_size;
                
                // Get linked string table by index
                long linkHeaderPos = (long)(e_shoff + (ulong)(sh_link * e_shentsize));
                long save = elfStream.Position;
                elfStream.Seek(linkHeaderPos + 4 + 4 + 8 + 8, SeekOrigin.Begin); // Offset to sh_offset
                strtabOffset = elfReader.ReadUInt64();
                elfStream.Seek(save, SeekOrigin.Begin);
            }
        }
        
        if (symtabOffset != 0 && strtabOffset != 0)
        {
            elfStream.Seek((long)symtabOffset, SeekOrigin.Begin);
            long endSym = (long)(symtabOffset + symtabSize);
            
            // Read symbols 1 by 1
            while (elfStream.Position < endSym)
            {
                long symStartPos = elfStream.Position;
                
                uint st_name = elfReader.ReadUInt32();
                byte st_info = elfReader.ReadByte();
                byte st_other = elfReader.ReadByte();
                ushort st_shndx = elfReader.ReadUInt16();
                ulong st_value = elfReader.ReadUInt64();
                ulong st_size = elfReader.ReadUInt64();
                
                long nextSymPos = elfStream.Position;
                
                if (st_name != 0)
                {
                    // Read Name
                    elfStream.Seek((long)(strtabOffset + st_name), SeekOrigin.Begin);
                    string name = ReadCString(elfReader);
                    
                    // Determine Type
                    byte type = (byte)'U'; // Default Undefined
                    int bind = st_info >> 4;   // 0=Local, 1=Global, 2=Weak
                    int kind = st_info & 0xF;  // 0=NoType, 1=Object, 2=Func, 3=Section, 4=File
                    
                    if (st_shndx == 0xFFF1) type = (byte)'a'; // ABS
                    else if (st_shndx == 0) type = (byte)'U'; // UNDEF
                    else if (st_shndx < 0xFF00)
                    {
                        // In a section
                        if (st_value >= minText && st_value < (minText + textSize)) type = (byte)'t';
                        else if (minData != ulong.MaxValue && st_value >= minData && st_value < (minData + dataSize + bssSize))
                        {
                            if (st_value < (minData + dataSize)) type = (byte)'d';
                            else type = (byte)'b';
                        }
                        else type = (byte)'a'; // Unknown (maybe .comment or .note)
                    }
                    
                    if (bind == 1 || bind == 2) type = (byte)char.ToUpper((char)type);
                    
                    // Filter useless symbols
                    if (kind != 3 && kind != 4 && !string.IsNullOrEmpty(name)) 
                    {
                        // Write Plan 9 Symbol
                        WriteBE64(symWriter, st_value);
                        symWriter.Write(type);
                        byte[] nameBytes = Encoding.UTF8.GetBytes(name);
                        symWriter.Write(nameBytes);
                        symWriter.Write((byte)0);
                    }
                }
                
                elfStream.Seek(nextSymPos, SeekOrigin.Begin);
            }
        }
        
        byte[] symData = symStream.ToArray();
        
        // --- Plan 9 Memory Layout ---
        // Text base: 0x200000, Header: 40 bytes (extended)
        // Text starts at: 0x200028
        const ulong PLAN9_TEXT_BASE = 0x200000;
        const ulong PLAN9_HEADER_SIZE = 40;
        const ulong PLAN9_TEXT_START = PLAN9_TEXT_BASE + PLAN9_HEADER_SIZE;
        
        // Write Plan 9 a.out
        using var outStream = File.Create(outputPath);
        using var writer = new BinaryWriter(outStream);
        
        // Calculate Plan 9 entry point by translating from ELF VA
        // Entry = Plan9TextStart + (ELF_Entry - ELF_TextBase)
        ulong elfEntryOffset = e_entry - minText;  // Offset within text segment
        ulong finalEntry = entryPoint != 0 ? entryPoint : (PLAN9_TEXT_START + elfEntryOffset);

        // Header (32 bytes standard)
        WriteBE32(writer, AOUT_MAGIC_AMD64);
        WriteBE32(writer, (uint)textSize);
        WriteBE32(writer, (uint)dataSize);
        WriteBE32(writer, (uint)bssSize);
        WriteBE32(writer, (uint)symData.Length);
        WriteBE32(writer, (uint)(finalEntry & 0xFFFFFFFF)); // Entry (low 32)
        WriteBE32(writer, 0); // spsz
        WriteBE32(writer, 0); // pcsz
        
        // Extended Header (8 bytes for 64-bit entry)
        WriteBE64(writer, finalEntry);
        
        // Segments
        writer.Write(textData);
        if (dataData != null) writer.Write(dataData);
        writer.Write(symData);
    }
    
    private static string ReadCString(BinaryReader reader)
    {
        var bytes = new List<byte>();
        while (true)
        {
            byte b = reader.ReadByte();
            if (b == 0) break;
            bytes.Add(b);
        }
        return Encoding.UTF8.GetString(bytes.ToArray());
    }
    
    private static void WriteBE32(BinaryWriter writer, uint value)
    {
        writer.Write((byte)((value >> 24) & 0xFF));
        writer.Write((byte)((value >> 16) & 0xFF));
        writer.Write((byte)((value >> 8) & 0xFF));
        writer.Write((byte)(value & 0xFF));
    }

    private static void WriteBE64(BinaryWriter writer, ulong value)
    {
        writer.Write((byte)((value >> 56) & 0xFF));
        writer.Write((byte)((value >> 48) & 0xFF));
        writer.Write((byte)((value >> 40) & 0xFF));
        writer.Write((byte)((value >> 32) & 0xFF));
        writer.Write((byte)((value >> 24) & 0xFF));
        writer.Write((byte)((value >> 16) & 0xFF));
        writer.Write((byte)((value >> 8) & 0xFF));
        writer.Write((byte)(value & 0xFF));
    }
}
