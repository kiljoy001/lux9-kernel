// PEWriter.fs - Minimal .NET PE/DLL Writer
// Emits minimal PE/COFF with CLI metadata for IL bytecode
// Based on ECMA-335 specification

module PEWriter

open System
open System.IO
open System.Text

// ==================== PE STRUCTURES ====================

[<Literal>]
let DOS_SIGNATURE = 0x5A4Dus  // "MZ"
[<Literal>]
let PE_SIGNATURE = 0x00004550u  // "PE\0\0"
[<Literal>]
let CLI_HEADER_RVA = 0x2000u
[<Literal>]
let METADATA_RVA = 0x2048u

type PEBuilder() =
    let data = ResizeArray<byte>()
    
    member _.Position = data.Count
    member _.Write(b: byte) = data.Add(b)
    member _.Write(bytes: byte array) = data.AddRange(bytes)
    
    member _.WriteUInt16(value: uint16) =
        data.Add(byte (value &&& 0xFFus))
        data.Add(byte (value >>> 8))
        
    member _.WriteUInt32(value: uint32) =
        data.Add(byte (value &&& 0xFFu))
        data.Add(byte ((value >>> 8) &&& 0xFFu))
        data.Add(byte ((value >>> 16) &&& 0xFFu))
        data.Add(byte (value >>> 24))
        
    member _.WriteUInt64(value: uint64) =
        data.Add(byte (value &&& 0xFFUL))
        data.Add(byte ((value >>> 8) &&& 0xFFUL))
        data.Add(byte ((value >>> 16) &&& 0xFFUL))
        data.Add(byte ((value >>> 24) &&& 0xFFUL))
        data.Add(byte ((value >>> 32) &&& 0xFFUL))
        data.Add(byte ((value >>> 40) &&& 0xFFUL))
        data.Add(byte ((value >>> 48) &&& 0xFFUL))
        data.Add(byte (value >>> 56))
        
    member _.WriteString(s: string) =
        let bytes = Encoding.UTF8.GetBytes(s)
        data.AddRange(bytes)
        data.Add(0uy)  // Null terminator
        
    member this.Align(alignment: int) =
        while data.Count % alignment <> 0 do
            data.Add(0uy)
            
    member _.Patch(position: int, value: uint32) =
        data.[position] <- byte (value &&& 0xFFu)
        data.[position + 1] <- byte ((value >>> 8) &&& 0xFFu)
        data.[position + 2] <- byte ((value >>> 16) &&& 0xFFu)
        data.[position + 3] <- byte (value >>> 24)
        
    member _.ToArray() = data.ToArray()

// ==================== MINIMAL PE GENERATION ====================

/// Generate minimal DOS header
let writeDOSHeader (pe: PEBuilder) =
    pe.WriteUInt16(DOS_SIGNATURE)  // e_magic
    for _ in 1..29 do pe.WriteUInt16(0us)  // Padding
    pe.WriteUInt32(0x80u)  // e_lfanew - offset to PE header

/// Generate PE/COFF header for x64
let writePEHeader (pe: PEBuilder) (codeSize: int) =
    pe.WriteUInt32(PE_SIGNATURE)
    
    // COFF header
    pe.WriteUInt16(0x8664us)  // Machine: AMD64
    pe.WriteUInt16(2us)       // NumberOfSections
    pe.WriteUInt32(0u)        // TimeDateStamp
    pe.WriteUInt32(0u)        // PointerToSymbolTable
    pe.WriteUInt32(0u)        // NumberOfSymbols
    pe.WriteUInt16(240us)     // SizeOfOptionalHeader
    pe.WriteUInt16(0x2022us)  // Characteristics: DLL, LARGE_ADDRESS, EXECUTABLE
    
    // Optional header (PE32+)
    pe.WriteUInt16(0x20Bus)   // Magic: PE32+
    pe.WriteUInt16(0us)       // Linker version
    pe.WriteUInt32(uint32 codeSize)  // SizeOfCode
    pe.WriteUInt32(0u)        // SizeOfInitializedData
    pe.WriteUInt32(0u)        // SizeOfUninitializedData
    pe.WriteUInt32(0x2000u)   // AddressOfEntryPoint
    pe.WriteUInt32(0x2000u)   // BaseOfCode
    pe.WriteUInt64(0x10000000UL)  // ImageBase
    pe.WriteUInt32(0x2000u)   // SectionAlignment
    pe.WriteUInt32(0x200u)    // FileAlignment
    pe.WriteUInt16(6us)       // OS version major
    pe.WriteUInt16(0us)       // OS version minor
    pe.WriteUInt16(0us)       // Image version
    pe.WriteUInt16(0us)       
    pe.WriteUInt16(6us)       // Subsystem version major
    pe.WriteUInt16(0us)       
    pe.WriteUInt32(0u)        // Win32VersionValue
    pe.WriteUInt32(0x6000u)   // SizeOfImage
    pe.WriteUInt32(0x200u)    // SizeOfHeaders
    pe.WriteUInt32(0u)        // CheckSum
    pe.WriteUInt16(3us)       // Subsystem: CONSOLE
    pe.WriteUInt16(0x8160us)  // DllCharacteristics: NX, DYNAMIC_BASE, ASLR
    pe.WriteUInt64(0x100000UL)  // SizeOfStackReserve
    pe.WriteUInt64(0x1000UL)    // SizeOfStackCommit
    pe.WriteUInt64(0x100000UL)  // SizeOfHeapReserve
    pe.WriteUInt64(0x1000UL)    // SizeOfHeapCommit
    pe.WriteUInt32(0u)          // LoaderFlags
    pe.WriteUInt32(16u)         // NumberOfRvaAndSizes
    
    // Data directories (16 entries)
    for i in 0..15 do
        if i = 14 then
            // CLI header
            pe.WriteUInt32(CLI_HEADER_RVA)
            pe.WriteUInt32(72u)
        else
            pe.WriteUInt32(0u)
            pe.WriteUInt32(0u)

/// Write section headers
let writeSectionHeaders (pe: PEBuilder) (textSize: int) =
    // .text section
    pe.Write(Encoding.ASCII.GetBytes(".text\x00\x00\x00"))  // Name
    pe.WriteUInt32(uint32 textSize)  // VirtualSize
    pe.WriteUInt32(0x2000u)          // VirtualAddress
    pe.WriteUInt32(uint32 textSize)  // SizeOfRawData
    pe.WriteUInt32(0x200u)           // PointerToRawData
    pe.WriteUInt32(0u)               // PointerToRelocations
    pe.WriteUInt32(0u)               // PointerToLinenumbers
    pe.WriteUInt16(0us)              // NumberOfRelocations
    pe.WriteUInt16(0us)              // NumberOfLinenumbers
    pe.WriteUInt32(0x60000020u)      // Characteristics: CODE, EXECUTE, READ
    
    // .reloc section (empty but required)
    pe.Write(Encoding.ASCII.GetBytes(".reloc\x00\x00"))
    pe.WriteUInt32(0u)
    pe.WriteUInt32(0x4000u)
    pe.WriteUInt32(0u)
    pe.WriteUInt32(0u)
    pe.WriteUInt32(0u)
    pe.WriteUInt32(0u)
    pe.WriteUInt16(0us)
    pe.WriteUInt16(0us)
    pe.WriteUInt32(0x42000040u)

/// Write CLI header
let writeCLIHeader (pe: PEBuilder) (metadataRVA: uint32) (metadataSize: uint32) =
    pe.WriteUInt32(72u)              // Cb (header size)
    pe.WriteUInt16(2us)              // MajorRuntimeVersion
    pe.WriteUInt16(5us)              // MinorRuntimeVersion
    pe.WriteUInt32(metadataRVA)      // MetaDataRVA
    pe.WriteUInt32(metadataSize)     // MetaDataSize
    pe.WriteUInt32(0x00000001u)      // Flags: ILONLY
    pe.WriteUInt32(0x06000001u)      // EntryPointToken (MethodDef row 1)
    pe.WriteUInt64(0UL)              // Resources
    pe.WriteUInt64(0UL)              // StrongNameSignature
    pe.WriteUInt64(0UL)              // CodeManagerTable
    pe.WriteUInt64(0UL)              // VTableFixups
    pe.WriteUInt64(0UL)              // ExportAddressTableJumps

/// Write minimal CLI metadata
let writeMetadata (pe: PEBuilder) (moduleName: string) (ilCode: byte array) =
    let metadataStart = pe.Position
    
    // Metadata header
    pe.WriteUInt32(0x424A5342u)  // Signature "BSJB"
    pe.WriteUInt16(1us)          // Major version
    pe.WriteUInt16(1us)          // Minor version
    pe.WriteUInt32(0u)           // Reserved
    pe.WriteUInt32(12u)          // Version string length
    pe.Write(Encoding.ASCII.GetBytes("v4.0.30319\x00\x00"))  // Version (12 bytes)
    pe.WriteUInt16(0us)          // Flags
    pe.WriteUInt16(5us)          // Number of streams
    
    // Stream headers (offsets to be patched)
    let streamHeadersStart = pe.Position
    
    // #~ stream
    pe.WriteUInt32(0u)  // Offset (patch later)
    pe.WriteUInt32(0u)  // Size (patch later)
    pe.Write(Encoding.ASCII.GetBytes("#~\x00\x00"))
    
    // #Strings stream
    pe.WriteUInt32(0u)
    pe.WriteUInt32(0u)
    pe.Write(Encoding.ASCII.GetBytes("#Strings\x00\x00\x00\x00"))
    
    // #US stream (user strings)
    pe.WriteUInt32(0u)
    pe.WriteUInt32(0u)
    pe.Write(Encoding.ASCII.GetBytes("#US\x00"))
    
    // #GUID stream
    pe.WriteUInt32(0u)
    pe.WriteUInt32(0u)
    pe.Write(Encoding.ASCII.GetBytes("#GUID\x00\x00\x00"))
    
    // #Blob stream
    pe.WriteUInt32(0u)
    pe.WriteUInt32(0u)
    pe.Write(Encoding.ASCII.GetBytes("#Blob\x00\x00\x00"))
    
    pe.Align(4)
    
    // For a minimal implementation, we'd need to properly write:
    // - #~ tables (Module, TypeDef, MethodDef, etc.)
    // - #Strings heap
    // - #Blob heap (method signatures, etc.)
    // - IL code
    
    // Simplified: just embed the IL code directly
    // Real implementation would build proper metadata tables
    pe.Write(ilCode)
    
    metadataStart

/// Generate complete .NET DLL from IL bytecode
let generateDLL (moduleName: string) (ilCode: byte array) : byte array =
    let pe = PEBuilder()
    
    // 1. DOS header
    writeDOSHeader pe
    pe.Align(0x80)
    
    // 2. PE/COFF header
    let codeSize = ilCode.Length + 200  // IL + metadata overhead
    writePEHeader pe codeSize
    
    // 3. Section headers
    writeSectionHeaders pe codeSize
    pe.Align(0x200)
    
    // 4. .text section starts at file offset 0x200
    // CLI header
    writeCLIHeader pe METADATA_RVA (uint32 ilCode.Length + 100u)
    pe.Align(8)
    
    // 5. Metadata and IL
    let _ = writeMetadata pe moduleName ilCode
    
    pe.Align(0x200)
    pe.ToArray()
