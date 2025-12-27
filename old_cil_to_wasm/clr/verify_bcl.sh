#!/bin/bash
set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"

echo "=== Building Lux9.CoreLib ==="
dotnet build corelib/Lux9.CoreLib.csproj -c Release

echo "=== Building Lux9.FSharp ==="
dotnet build fsharp/Lux9.FSharp.fsproj -c Release

FSHARP_DLL="fsharp/bin/Release/net9.0/Lux9.FSharp.dll"
CORELIB_DLL="corelib/bin/Release/net9.0/Lux9.CoreLib.dll"

if [ ! -f "$FSHARP_DLL" ]; then
    echo "Error: FSharp DLL not found at $FSHARP_DLL"
    exit 1
fi

echo "=== Compiling BCL Verification Test ==="
# We use fsc (F# Compiler) directly if available, or dotnet fsc
if command -v fsc &> /dev/null; then
    COMPILER="fsc"
else
    # Fallback to dotnet executables if installed, or just skip if no compiler found
    # Assuming standard environment might not have 'fsc' on path but typically does with mono or dotnet sdk
    # If using dotnet sdk, usually 'dotnet build' handles projects.
    # Let's try to create a temp project for the test
    echo "fsc not found, creating temp project"
    rm -rf test/TestVerify
    mkdir -p test/TestVerify
    cd test/TestVerify
    dotnet new console -lang F# --force > /dev/null
    cp ../test_fsharp_bcl.fs Program.fs
    
    # We need to reference the local DLLs.
    # dotnet add reference is for projects or nuget. For loose DLLs, we can use <Reference> in csproj/fsproj
    cat > TestVerify.fsproj <<EOF
<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <OutputType>Exe</OutputType>
    <TargetFramework>net9.0</TargetFramework>
    <DisableImplicitFSharpCoreReference>true</DisableImplicitFSharpCoreReference>
    <NoStdLib>true</NoStdLib>
    <NoConfig>true</NoConfig>
  </PropertyGroup>

  <ItemGroup>
    <Compile Include="Program.fs" />
  </ItemGroup>

  <ItemGroup>
    <Reference Include="Lux9.CoreLib">
      <HintPath>../../$CORELIB_DLL</HintPath>
    </Reference>
    <Reference Include="Lux9.FSharp">
      <HintPath>../../$FSHARP_DLL</HintPath>
    </Reference>
  </ItemGroup>
</Project>
EOF

    echo "Building Test Project..."
    dotnet build -c Release
    echo "Test Project Built Successfully!"
    
    # We cannot run it because it's a kernel DLL with no runtime here?
    # Or maybe we can run it with dotnet?
    # Lux9.CoreLib is NoStdLib, so it won't run on standard .NET runtime easily unless it's pure logic and we get lucky.
    # But for verification of *compilation* (API completeness), building is enough.
    exit 0
fi

$COMPILER --nologo --target:exe --out:test_fsharp_bcl.exe -r:"$FSHARP_DLL" -r:"$CORELIB_DLL" test/test_fsharp_bcl.fs --noframework

echo "Verification compilation successful!"
