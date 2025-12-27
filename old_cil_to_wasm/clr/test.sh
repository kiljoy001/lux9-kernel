#!/bin/bash
# test.sh - Build and test IL parser and IL → Fruity converter

set -e  # Exit on error

echo "=== Building IL Parser Test ==="
gcc -o test_il_parser test_il_parser.c il_parser.c il_disasm.c -I. -Wall -Wextra
echo "IL Parser build complete!"

echo ""
echo "=== Building IL → Fruity Converter Test ==="
gcc -o test_il_to_fruity test_il_to_fruity.c il_to_fruity.c il_parser.c il_disasm.c -I. -I./fruity -Wall -Wextra
echo "IL → Fruity build complete!"

echo ""
echo "=== Checking for F# compiler ==="
if command -v fsc &> /dev/null; then
    echo "F# compiler found!"

    echo ""
    echo "=== Compiling F# test program ==="
    fsc test_hello.fs
    echo "F# compilation complete!"

    echo ""
    echo "=== Running IL Parser ==="
    ./test_il_parser test_hello.dll

    echo ""
    echo "=== Running IL → Fruity Converter ==="
    ./test_il_to_fruity test_hello.dll

else
    echo "F# compiler (fsc) not found."
    echo ""
    echo "To install F#:"
    echo "  Ubuntu/Debian: sudo apt-get install fsharp"
    echo "  Or download .NET SDK from: https://dotnet.microsoft.com/download"
    echo ""
    echo "You can also test with any existing .NET DLL:"
    echo "  ./test_il_parser path/to/some.dll"
    echo "  ./test_il_to_fruity path/to/some.dll"
fi
