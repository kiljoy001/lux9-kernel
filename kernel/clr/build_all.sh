#!/bin/bash
set -e

echo "=== Building Layer 1: Universal CoreLib ==="
./build_corelib.sh

echo ""
echo "=== Building Layer 2: Lux9.DLR (Iron Language Support) ==="
dotnet build dlr/Lux9.DLR.csproj -c Release --no-dependencies

echo ""
echo "=== Building Layer 2: Lux9.FSharp (F# Support) ==="
dotnet build fsharp/Lux9.FSharp.fsproj -c Release --no-dependencies

echo ""
echo "All Multi-Language Support Libraries Built Successfully!"
ls -lh corelib/bin/Release/net9.0/Lux9.CoreLib.dll
ls -lh dlr/bin/Release/net9.0/Lux9.DLR.dll
ls -lh fsharp/bin/Release/net9.0/Lux9.FSharp.dll
