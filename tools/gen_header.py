import sys, struct, os

if len(sys.argv) < 3:
    print("Usage: gen_header.py <entry> <output>")
    sys.exit(1)

entry = int(sys.argv[1], 16)
output_file = sys.argv[2]

text_path = 'text.bin'
data_path = 'data.bin'

text_blob = open(text_path, 'rb').read()
data_blob = open(data_path, 'rb').read()

text_size = len(text_blob)
data_size = len(data_blob)
bss_size = 0 # Assume 0

# Plan 9 Header (AMD64 40-byte extended)
header = bytearray()
header += struct.pack('>I', 0x8A97)      # Magic
header += struct.pack('>I', text_size)   # Text Size (File Size of Text Segment EXCLUDING Header? Spec is ambiguous)
                                         # "The size of the header is not included in any of the other sizes."
                                         # So TextSize = len(text_blob).
header += struct.pack('>I', data_size)   # Data Size
header += struct.pack('>I', bss_size)    # BSS Size
header += struct.pack('>I', 0)           # Syms Size
header += struct.pack('>I', entry)       # Entry (Lower 32)
header += struct.pack('>I', 0)           # Spsz
header += struct.pack('>I', 0)           # Pcsz
# Extended part
header += struct.pack('>Q', entry)       # 64-bit Entry

# Assembly
with open(output_file, 'wb') as f:
    f.write(header)
    f.write(text_blob)
    
    # Calculate Padding to align Data to 4KB Page
    # Current Position = len(header) + len(text)
    curr_pos = len(header) + len(text_blob)
    # Plan 9: "Data segment starts at the first page-rounded virtual address after the text segment."
    # Virtual Address of Header Start = 0x200000 + (0x28 or 0?)
    # Wait. Text Base passed to ld was 0x200028.
    # So Text Blob starts at 0x200028.
    # Header takes 0 to 0x28? (40 bytes = 0x28).
    # So VAddr 0x200000 = Header Start.
    # VAddr 0x200028 = Text Blob Start.
    # So Current VAddr = 0x200000 + curr_pos.
    
    # We want Data to start at 0x210000 (as passed to ld).
    # So we pad until VAddr = 0x210000.
    target_data_vaddr = 0x210000
    current_vaddr = 0x200000 + curr_pos
    
    pad_len = target_data_vaddr - current_vaddr
    
    if pad_len < 0:
        print(f"Error: Text segment overflow! Curr: {hex(current_vaddr)} Target: {hex(target_data_vaddr)}")
        sys.exit(1)
        
    f.write(b'\x00' * pad_len)
    
    f.write(data_blob)
    
print(f"Native binary assembled. Text: {hex(text_size)}, Data: {hex(data_size)}, Pad: {pad_len}")
