#!/bin/bash
set -e

echo "=== Phase 1: Building Lux9 BCL (Base Class Library) ==="
cd bcl
dotnet build Lux9.BCL.sln -c Release
cd ..

echo ""
echo "=== Phase 2: Building Language Support Libraries ==="

echo "Building Lux9.DLR (Dynamic Language Runtime)..."
dotnet build dlr/Lux9.DLR.csproj -c Release

echo "Building Lux9.FSharp (F# Core Shim)..."
dotnet build fsharp/Lux9.FSharp.fsproj -c Release

echo ""
echo "=== Build Complete! ==="
echo "Output:"
ls -lh bcl/src/System.Runtime/bin/Release/net9.0/System.Runtime.dll
ls -lh bcl/src/System.Collections/bin/Release/net9.0/System.Collections.dll
ls -lh bcl/src/System.Numerics/bin/Release/net9.0/System.Numerics.dll
ls -lh dlr/bin/Release/net9.0/Lux9.DLR.dll
ls -lh fsharp/bin/Release/net9.0/Lux9.FSharp.dll
