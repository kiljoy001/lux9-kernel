import sys
import re

def main():
    if len(sys.argv) < 3:
        print("Usage: python3 hex_to_wasm.py <input_dump> <output_wasm>")
        return

    input_path = sys.argv[1]
    output_path = sys.argv[2]

    print(f"Converting {input_path} to {output_path}...")

    with open(input_path, 'r') as f:
        lines = f.readlines()

    binary_data = bytearray()
    started = False

    for line in lines:
        original_line = line
        line = line.strip()
        if not line:
            continue
            
        # Check for address prefix format "  XXXX: "
        # The dump prints "\n  %04x: "
        
        match = re.search(r'^\s*([0-9a-fA-F]+):\s+(.*)$', original_line)
        if match:
            # It's a hex line
            started = True
            hex_part = match.group(2)
            tokens = hex_part.split()
            for token in tokens:
                try:
                    if len(token) == 2:
                        byte = int(token, 16)
                        binary_data.append(byte)
                except ValueError:
                    pass
        else:
            # Not a hex line
            if started:
                # If we were strictly parsing, we should stop here
                # checking if we have enough data?
                # The log might have interleaved stuff? No, kernel is single threaded mostly.
                # Let's break to avoid garbage
                break
            else:
                # Haven't started yet, ignore header lines
                continue

    print(f"Parsed {len(binary_data)} bytes.")
    
    with open(output_path, 'wb') as f:
        f.write(binary_data)

    print("Done.")

if __name__ == "__main__":
    main()
