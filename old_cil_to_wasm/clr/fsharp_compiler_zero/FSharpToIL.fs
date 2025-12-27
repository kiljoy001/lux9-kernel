// FSharpToIL.fs - F# AST to .NET IL Bytecode Emitter
// Emits ECMA-335 IL bytecode from typed F# AST
// Output feeds into existing CLR pipeline: il_parser.c → Fruity → QBE → x86

module FSharpToIL

open FSharpAST
open System
open System.IO

// ==================== IL OPCODES (ECMA-335) ====================

[<Literal>]
let IL_NOP = 0x00uy
[<Literal>]
let IL_LDARG_0 = 0x02uy
[<Literal>]
let IL_LDARG_1 = 0x03uy
[<Literal>]
let IL_LDARG_2 = 0x04uy
[<Literal>]
let IL_LDARG_3 = 0x05uy
[<Literal>]
let IL_LDLOC_0 = 0x06uy
[<Literal>]
let IL_LDLOC_1 = 0x07uy
[<Literal>]
let IL_LDLOC_2 = 0x08uy
[<Literal>]
let IL_LDLOC_3 = 0x09uy
[<Literal>]
let IL_STLOC_0 = 0x0Auy
[<Literal>]
let IL_STLOC_1 = 0x0Buy
[<Literal>]
let IL_STLOC_2 = 0x0Cuy
[<Literal>]
let IL_STLOC_3 = 0x0Duy
[<Literal>]
let IL_LDARG_S = 0x0Euy
[<Literal>]
let IL_STARG_S = 0x10uy
[<Literal>]
let IL_LDLOC_S = 0x11uy
[<Literal>]
let IL_STLOC_S = 0x13uy
[<Literal>]
let IL_LDNULL = 0x14uy
[<Literal>]
let IL_LDC_I4_M1 = 0x15uy
[<Literal>]
let IL_LDC_I4_0 = 0x16uy
[<Literal>]
let IL_LDC_I4_1 = 0x17uy
[<Literal>]
let IL_LDC_I4_2 = 0x18uy
[<Literal>]
let IL_LDC_I4_3 = 0x19uy
[<Literal>]
let IL_LDC_I4_4 = 0x1Auy
[<Literal>]
let IL_LDC_I4_5 = 0x1Buy
[<Literal>]
let IL_LDC_I4_6 = 0x1Cuy
[<Literal>]
let IL_LDC_I4_7 = 0x1Duy
[<Literal>]
let IL_LDC_I4_8 = 0x1Euy
[<Literal>]
let IL_LDC_I4_S = 0x1Fuy
[<Literal>]
let IL_LDC_I4 = 0x20uy
[<Literal>]
let IL_LDC_I8 = 0x21uy
[<Literal>]
let IL_DUP = 0x25uy
[<Literal>]
let IL_POP = 0x26uy
[<Literal>]
let IL_CALL = 0x28uy
[<Literal>]
let IL_RET = 0x2Auy
[<Literal>]
let IL_BR_S = 0x2Buy
[<Literal>]
let IL_BRFALSE_S = 0x2Cuy
[<Literal>]
let IL_BRTRUE_S = 0x2Duy
[<Literal>]
let IL_BR = 0x38uy
[<Literal>]
let IL_BRFALSE = 0x39uy
[<Literal>]
let IL_BRTRUE = 0x3Auy
[<Literal>]
let IL_ADD = 0x58uy
[<Literal>]
let IL_SUB = 0x59uy
[<Literal>]
let IL_MUL = 0x5Auy
[<Literal>]
let IL_DIV = 0x5Buy
[<Literal>]
let IL_REM = 0x5Duy
[<Literal>]
let IL_AND = 0x5Fuy
[<Literal>]
let IL_OR = 0x60uy
[<Literal>]
let IL_XOR = 0x61uy
[<Literal>]
let IL_SHL = 0x62uy
[<Literal>]
let IL_SHR = 0x63uy
[<Literal>]
let IL_NEG = 0x65uy
[<Literal>]
let IL_NOT = 0x66uy
[<Literal>]
let IL_LDSTR = 0x72uy
[<Literal>]
let IL_NEWOBJ = 0x73uy
[<Literal>]
let IL_NEWARR = 0x8Duy
[<Literal>]
let IL_LDLEN = 0x8Euy
[<Literal>]
let IL_LDELEM_I4 = 0x94uy
[<Literal>]
let IL_STELEM_I4 = 0x9Euy
// Two-byte opcodes (0xFE prefix)
[<Literal>]
let IL_CEQ = 0x01uy  // After 0xFE
[<Literal>]
let IL_CGT = 0x02uy
[<Literal>]
let IL_CLT = 0x04uy

// ==================== COMPILATION CONTEXT ====================

type LocalVar = {
    Name: string
    Index: int
    Type: FSharpType option
}

type CompilationContext = {
    Locals: Map<string, LocalVar>
    NextLocalIndex: int
    Arguments: Map<string, int>
    StringTable: ResizeArray<string>
    LabelCounter: int
}

let emptyContext = {
    Locals = Map.empty
    NextLocalIndex = 0
    Arguments = Map.empty
    StringTable = ResizeArray<string>()
    LabelCounter = 0
}

// ==================== IL BUILDER ====================

type ILBuilder() =
    let code = ResizeArray<byte>()
    
    member _.Emit(opcode: byte) = code.Add(opcode)
    
    member _.EmitInt8(value: sbyte) = code.Add(byte value)
    member _.EmitUInt8(value: byte) = code.Add(value)
    
    member _.EmitInt32(value: int) =
        code.Add(byte (value &&& 0xFF))
        code.Add(byte ((value >>> 8) &&& 0xFF))
        code.Add(byte ((value >>> 16) &&& 0xFF))
        code.Add(byte ((value >>> 24) &&& 0xFF))
    
    member _.EmitInt64(value: int64) =
        code.Add(byte (value &&& 0xFFL))
        code.Add(byte ((value >>> 8) &&& 0xFFL))
        code.Add(byte ((value >>> 16) &&& 0xFFL))
        code.Add(byte ((value >>> 24) &&& 0xFFL))
        code.Add(byte ((value >>> 32) &&& 0xFFL))
        code.Add(byte ((value >>> 40) &&& 0xFFL))
        code.Add(byte ((value >>> 48) &&& 0xFFL))
        code.Add(byte ((value >>> 56) &&& 0xFFL))

    member _.Position with get() = code.Count
    
    member _.PatchInt32(position: int, value: int) =
        code.[position] <- byte (value &&& 0xFF)
        code.[position + 1] <- byte ((value >>> 8) &&& 0xFF)
        code.[position + 2] <- byte ((value >>> 16) &&& 0xFF)
        code.[position + 3] <- byte ((value >>> 24) &&& 0xFF)
    
    member _.ToArray() = code.ToArray()

// ==================== CODE GENERATION ====================

let emitLdcI4 (il: ILBuilder) (value: int) =
    match value with
    | -1 -> il.Emit(IL_LDC_I4_M1)
    | 0 -> il.Emit(IL_LDC_I4_0)
    | 1 -> il.Emit(IL_LDC_I4_1)
    | 2 -> il.Emit(IL_LDC_I4_2)
    | 3 -> il.Emit(IL_LDC_I4_3)
    | 4 -> il.Emit(IL_LDC_I4_4)
    | 5 -> il.Emit(IL_LDC_I4_5)
    | 6 -> il.Emit(IL_LDC_I4_6)
    | 7 -> il.Emit(IL_LDC_I4_7)
    | 8 -> il.Emit(IL_LDC_I4_8)
    | n when n >= -128 && n <= 127 ->
        il.Emit(IL_LDC_I4_S)
        il.EmitInt8(sbyte n)
    | n ->
        il.Emit(IL_LDC_I4)
        il.EmitInt32(n)

let emitLdloc (il: ILBuilder) (index: int) =
    match index with
    | 0 -> il.Emit(IL_LDLOC_0)
    | 1 -> il.Emit(IL_LDLOC_1)
    | 2 -> il.Emit(IL_LDLOC_2)
    | 3 -> il.Emit(IL_LDLOC_3)
    | n when n < 256 ->
        il.Emit(IL_LDLOC_S)
        il.EmitUInt8(byte n)
    | _ -> failwith "Local index too large"

let emitStloc (il: ILBuilder) (index: int) =
    match index with
    | 0 -> il.Emit(IL_STLOC_0)
    | 1 -> il.Emit(IL_STLOC_1)
    | 2 -> il.Emit(IL_STLOC_2)
    | 3 -> il.Emit(IL_STLOC_3)
    | n when n < 256 ->
        il.Emit(IL_STLOC_S)
        il.EmitUInt8(byte n)
    | _ -> failwith "Local index too large"

let emitLdarg (il: ILBuilder) (index: int) =
    match index with
    | 0 -> il.Emit(IL_LDARG_0)
    | 1 -> il.Emit(IL_LDARG_1)
    | 2 -> il.Emit(IL_LDARG_2)
    | 3 -> il.Emit(IL_LDARG_3)
    | n when n < 256 ->
        il.Emit(IL_LDARG_S)
        il.EmitUInt8(byte n)
    | _ -> failwith "Argument index too large"

/// Emit IL for a binary operator
let emitBinaryOp (il: ILBuilder) (op: Operator) =
    match op with
    | OpAdd -> il.Emit(IL_ADD)
    | OpSubtract -> il.Emit(IL_SUB)
    | OpMultiply -> il.Emit(IL_MUL)
    | OpDivide -> il.Emit(IL_DIV)
    | OpModulo -> il.Emit(IL_REM)
    | OpBitAnd -> il.Emit(IL_AND)
    | OpBitOr -> il.Emit(IL_OR)
    | OpBitXor -> il.Emit(IL_XOR)
    | OpShiftLeft -> il.Emit(IL_SHL)
    | OpShiftRight -> il.Emit(IL_SHR)
    | OpEqual ->
        il.Emit(0xFEuy)  // Two-byte prefix
        il.Emit(IL_CEQ)
    | OpGreater ->
        il.Emit(0xFEuy)
        il.Emit(IL_CGT)
    | OpLess ->
        il.Emit(0xFEuy)
        il.Emit(IL_CLT)
    | OpGreaterEq ->
        // a >= b  <==>  !(a < b)
        il.Emit(0xFEuy)
        il.Emit(IL_CLT)
        emitLdcI4 il 0
        il.Emit(0xFEuy)
        il.Emit(IL_CEQ)
    | OpLessEq ->
        // a <= b  <==>  !(a > b)
        il.Emit(0xFEuy)
        il.Emit(IL_CGT)
        emitLdcI4 il 0
        il.Emit(0xFEuy)
        il.Emit(IL_CEQ)
    | OpNotEqual ->
        il.Emit(0xFEuy)
        il.Emit(IL_CEQ)
        emitLdcI4 il 0
        il.Emit(0xFEuy)
        il.Emit(IL_CEQ)
    | OpLogicalAnd ->
        il.Emit(IL_AND)
    | OpLogicalOr ->
        il.Emit(IL_OR)
    | _ -> failwithf "Unsupported operator: %A" op

/// Main AST to IL compiler
let rec emitAST (il: ILBuilder) (ctx: CompilationContext) (ast: FSharpAST) : CompilationContext =
    match ast with
    | ASTLiteral(LitInt value) ->
        if value >= int64 Int32.MinValue && value <= int64 Int32.MaxValue then
            emitLdcI4 il (int value)
        else
            il.Emit(IL_LDC_I8)
            il.EmitInt64(value)
        ctx
        
    | ASTLiteral(LitBool true) ->
        emitLdcI4 il 1
        ctx
        
    | ASTLiteral(LitBool false) ->
        emitLdcI4 il 0
        ctx
        
    | ASTLiteral(LitUnit) ->
        // Unit is represented as void/nothing - emit nothing
        ctx
        
    | ASTLiteral(LitString s) ->
        // Add to string table and emit ldstr
        let idx = ctx.StringTable.Count
        ctx.StringTable.Add(s)
        il.Emit(IL_LDSTR)
        il.EmitInt32(0x70000000 ||| idx)  // User string token
        ctx
        
    | ASTIdent name ->
        // Look up in locals first, then arguments
        match Map.tryFind name ctx.Locals with
        | Some local ->
            emitLdloc il local.Index
            ctx
        | None ->
            match Map.tryFind name ctx.Arguments with
            | Some argIdx ->
                emitLdarg il argIdx
                ctx
            | None ->
                failwithf "Unknown identifier: %s" name
                
    | ASTBinaryOp(op, left, right) ->
        let ctx1 = emitAST il ctx left
        let ctx2 = emitAST il ctx1 right
        emitBinaryOp il op
        ctx2
        
    | ASTLet(name, value, body) ->
        // Emit value
        let ctx1 = emitAST il ctx value
        // Allocate local
        let localIdx = ctx1.NextLocalIndex
        let local = { Name = name; Index = localIdx; Type = None }
        let ctx2 = { ctx1 with 
                        Locals = Map.add name local ctx1.Locals
                        NextLocalIndex = localIdx + 1 }
        // Store to local
        emitStloc il localIdx
        // Emit body
        emitAST il ctx2 body
        
    | ASTIf(cond, thenExpr, elseExpr) ->
        // Emit condition
        let ctx1 = emitAST il ctx cond
        // brfalse to else
        il.Emit(IL_BRFALSE)
        let elsePatchPos = il.Position
        il.EmitInt32(0)  // Placeholder
        // Emit then
        let ctx2 = emitAST il ctx1 thenExpr
        // br to end
        il.Emit(IL_BR)
        let endPatchPos = il.Position
        il.EmitInt32(0)  // Placeholder
        // Patch else jump
        let elsePos = il.Position
        il.PatchInt32(elsePatchPos, elsePos - elsePatchPos - 4)
        // Emit else
        let ctx3 = emitAST il ctx2 elseExpr
        // Patch end jump
        let endPos = il.Position
        il.PatchInt32(endPatchPos, endPos - endPatchPos - 4)
        ctx3
        
    | ASTSequence(first, second) ->
        let ctx1 = emitAST il ctx first
        il.Emit(IL_POP)  // Discard first result
        emitAST il ctx1 second
        
    | ASTUnit ->
        ctx
        
    | ASTLambda(params', body) ->
        // Lambda compilation requires creating a new method
        // For now, inline simple lambdas
        let mutable ctx' = ctx
        for (i, param) in List.indexed params' do
            ctx' <- { ctx' with Arguments = Map.add param i ctx'.Arguments }
        emitAST il ctx' body
        
    | ASTApp(func, arg) ->
        // Emit argument first
        let ctx1 = emitAST il ctx arg
        // Then emit function and call
        let ctx2 = emitAST il ctx1 func
        il.Emit(IL_CALL)
        il.EmitInt32(0)  // Method token - would need proper resolution
        ctx2
        
    | ASTTuple(elements) ->
        // Emit each element, leaving on stack
        let mutable ctx' = ctx
        for elem in elements do
            ctx' <- emitAST il ctx' elem
        ctx'
        
    | ASTList(elements) ->
        // Create array
        emitLdcI4 il (List.length elements)
        il.Emit(IL_NEWARR)
        il.EmitInt32(0x01000001)  // System.Object token
        // Store each element
        for (i, elem) in List.indexed elements do
            il.Emit(IL_DUP)
            emitLdcI4 il i
            let _ = emitAST il ctx elem
            il.Emit(IL_STELEM_I4)
        ctx
        
    | _ ->
        failwithf "Unsupported AST node: %A" ast

/// Compile F# AST to IL bytecode
let compileToIL (ast: FSharpAST) : byte array =
    let il = ILBuilder()
    let _ = emitAST il emptyContext ast
    il.Emit(IL_RET)
    il.ToArray()

/// Compile to full method body (with header)
let compileToMethodBody (ast: FSharpAST) (localCount: int) : byte array =
    let code = compileToIL ast
    
    if code.Length < 64 && localCount = 0 then
        // Tiny format: (code_size << 2) | 0x02
        let header = byte ((code.Length <<< 2) ||| 0x02)
        Array.concat [| [| header |]; code |]
    else
        // Fat format
        let flags = 0x3003us  // Fat format, init locals
        let maxStack = 8us
        let header = [|
            byte (flags &&& 0xFFus)
            byte (flags >>> 8)
            byte (maxStack &&& 0xFFus)
            byte (maxStack >>> 8)
            byte (code.Length &&& 0xFF)
            byte ((code.Length >>> 8) &&& 0xFF)
            byte ((code.Length >>> 16) &&& 0xFF)
            byte ((code.Length >>> 24) &&& 0xFF)
            0uy; 0uy; 0uy; 0uy  // Local var sig token
        |]
        Array.concat [| header; code |]
