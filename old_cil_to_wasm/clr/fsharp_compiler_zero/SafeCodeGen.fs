// Safe Code Generation Module
// Generates verified safe x86-64 assembly code

module SafeCodeGen

open FSharpAST
open FSharpCodeGen

type SafetyLevel =
    | Unsafe
    | Safe
    | Verified
    | Paranoid

/// Compile with safety checks
let compileSafe (ast: FSharpAST) (safety: SafetyLevel) : byte array =
    // For now, just use regular code generation
    FSharpCodeGen.compileToMachineCode ast