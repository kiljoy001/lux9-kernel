// F# to x86-64 Code Generator
// Based on Intel 64 and IA-32 Architectures Software Developer's Manual
// Converts F# AST to native x86-64 machine code

module FSharpCodeGen

open FSharpAST
open FSharpModuleSystem
open System
open System.Collections.Generic

// ==================== x86-64 INSTRUCTION ENCODING ====================
// Based on Intel 64 manual section 2.1

type Register =
    | RAX = 0 | RCX = 1 | RDX = 2 | RBX = 3
    | RSP = 4 | RBP = 5 | RSI = 6 | RDI = 7
    | R8 = 8  | R9 = 9  | R10 = 10 | R11 = 11
    | R12 = 12 | R13 = 13 | R14 = 14 | R15 = 15

type Instruction =
    // Move instructions - Intel manual Table 2-4
    | MOV_REG_IMM64 of reg: Register * imm: int64    // 48 B8+r mov RAX, imm64
    | MOV_REG_REG of dst: Register * src: Register   // 48 89 /r mov r/m64, r64
    | MOV_MEM_REG of addr: Register * offset: int * src: Register  // 48 89 /r mov [addr+offset], src
    | MOV_REG_MEM of dst: Register * addr: Register * offset: int  // 48 8B /r mov dst, [addr+offset]
    | MOV_MEM_IMM16 of addr: Register * imm: int16   // 66 C7 /0 mov word [addr], imm16
    
    // Arithmetic instructions - Intel manual Section 4.1
    | ADD_REG_REG of dst: Register * src: Register   // 48 01 /r add r/m64, r64
    | SUB_REG_REG of dst: Register * src: Register   // 48 29 /r sub r/m64, r64
    | MUL_REG of reg: Register                       // 48 F7 /4 mul r/m64
    | DIV_REG of reg: Register                       // 48 F7 /6 div r/m64
    
    // Compare and jump - Intel manual Section 4.1
    | CMP_REG_REG of reg1: Register * reg2: Register // 48 39 /r cmp r/m64, r64
    | JE_REL32 of offset: int32                      // 0F 84 cd je rel32
    | JNE_REL32 of offset: int32                     // 0F 85 cd jne rel32
    | JG_REL32 of offset: int32                      // 0F 8F cd jg rel32
    | JLE_REL32 of offset: int32                     // 0F 8E cd jle rel32
    | JMP_REL32 of offset: int32                     // E9 cd jmp rel32
    
    // Set instructions - Intel manual Section 4.5
    | SETG_REG of reg: Register                      // 0F 9F /0 setg r/m8
    | SETL_REG of reg: Register                      // 0F 9C /0 setl r/m8
    | SETE_REG of reg: Register                      // 0F 94 /0 sete r/m8
    | SETNE_REG of reg: Register                     // 0F 95 /0 setne r/m8
    | SETGE_REG of reg: Register                     // 0F 9D /0 setge r/m8
    | SETLE_REG of reg: Register                     // 0F 9E /0 setle r/m8
    
    // Stack operations - Intel manual Section 4.2
    | PUSH_REG of reg: Register                      // 50+r push r64
    | POP_REG of reg: Register                       // 58+r pop r64
    
    // Function calls - Intel manual Section 4.3
    | CALL_REL32 of offset: int32                    // E8 cd call rel32
    | RET                                            // C3 ret
    | TAIL_JMP_REL32 of offset: int32               // E9 cd - tail call via jmp
    
    // System calls - x86-64 System Call ABI
    | SYSCALL                                        // 0F 05 syscall
    | MACH_MSG of timeout: int32                     // Mach message passing
    | MACH_TASK_SELF                                 // Get current task port
    | VM_ALLOCATE of size: int64                     // Allocate virtual memory
    
    // Memory management - based on memory_enhancement_working.v proof
    | NUMA_ALLOC of size: int64 * node: int32       // NUMA-aware allocation
    | CPU_LOCAL_ALLOC of size: int64 * cpu: int32   // CPU-local memory pool
    | CACHE_ALIGN_ALLOC of size: int64               // Cache-aligned allocation
    
    // Exception handling
    | INT3                                           // CC int3 - breakpoint/exception

// ==================== MACHINE CODE ENCODING ====================
// Direct implementation of Intel 64 instruction formats

/// REX prefix for 64-bit operations (Intel manual Section 2.2.1)
let rexPrefix (w: bool) (r: bool) (x: bool) (b: bool) : byte =
    let mutable prefix = 0x40uy
    if w then prefix <- prefix ||| 0x08uy  // REX.W bit
    if r then prefix <- prefix ||| 0x04uy  // REX.R bit  
    if x then prefix <- prefix ||| 0x02uy  // REX.X bit
    if b then prefix <- prefix ||| 0x01uy  // REX.B bit
    prefix

/// ModR/M byte encoding (Intel manual Section 2.1.5)
let modrmByte (mode: byte) (reg: byte) (rm: byte) : byte =
    (mode <<< 6) ||| (reg <<< 3) ||| rm

/// Encode register number with REX handling
let encodeRegister (reg: Register) : byte * bool =
    let regNum = byte (int reg)
    if regNum >= 8uy then
        (regNum - 8uy, true)  // Extended register, needs REX.R or REX.B
    else
        (regNum, false)

/// Convert 64-bit immediate to little-endian bytes
let int64ToBytes (value: int64) : byte array =
    BitConverter.GetBytes(value)

/// Convert 32-bit immediate to little-endian bytes  
let int32ToBytes (value: int32) : byte array =
    BitConverter.GetBytes(value)

/// Encode single instruction to machine code
let encodeInstruction (instr: Instruction) : byte array =
    match instr with
    
    // MOV r64, imm64 - Intel manual: 48 B8+r mov RAX,imm64
    | MOV_REG_IMM64(reg, imm) ->
        let (regNum, needsRexB) = encodeRegister reg
        let rex = rexPrefix true false false needsRexB  // REX.W=1 for 64-bit
        let opcode = 0xB8uy + regNum
        Array.concat [|
            [| rex; opcode |]
            int64ToBytes imm
        |]
    
    // MOV r/m64, r64 - Intel manual: 48 89 /r
    | MOV_REG_REG(dst, src) ->
        let (dstNum, needsRexB) = encodeRegister dst
        let (srcNum, needsRexR) = encodeRegister src
        let rex = rexPrefix true needsRexR false needsRexB
        let modrm = modrmByte 3uy srcNum dstNum  // mode=11 (register direct)
        [| rex; 0x89uy; modrm |]
    
    // MOV [addr+offset], src - Intel manual: 48 89 /r
    | MOV_MEM_REG(addr, offset, src) ->
        let (addrNum, needsRexB) = encodeRegister addr
        let (srcNum, needsRexR) = encodeRegister src
        let rex = rexPrefix true needsRexR false needsRexB
        
        if offset = 0 then
            // No displacement - mode=00
            let modrm = modrmByte 0uy srcNum addrNum
            [| rex; 0x89uy; modrm |]
        elif offset >= -128 && offset <= 127 then
            // 8-bit displacement - mode=01
            let modrm = modrmByte 1uy srcNum addrNum
            [| rex; 0x89uy; modrm; byte offset |]
        else
            // 32-bit displacement - mode=10
            let modrm = modrmByte 2uy srcNum addrNum
            Array.concat [|
                [| rex; 0x89uy; modrm |]
                int32ToBytes (int32 offset)
            |]
    
    // MOV dst, [addr+offset] - Intel manual: 48 8B /r
    | MOV_REG_MEM(dst, addr, offset) ->
        let (dstNum, needsRexR) = encodeRegister dst
        let (addrNum, needsRexB) = encodeRegister addr
        let rex = rexPrefix true needsRexR false needsRexB
        
        if offset = 0 then
            // No displacement - mode=00
            let modrm = modrmByte 0uy dstNum addrNum
            [| rex; 0x8Buy; modrm |]
        elif offset >= -128 && offset <= 127 then
            // 8-bit displacement - mode=01
            let modrm = modrmByte 1uy dstNum addrNum
            [| rex; 0x8Buy; modrm; byte offset |]
        else
            // 32-bit displacement - mode=10
            let modrm = modrmByte 2uy dstNum addrNum
            Array.concat [|
                [| rex; 0x8Buy; modrm |]
                int32ToBytes (int32 offset)
            |]
    
    // MOV word [addr], imm16 - Intel manual: 66 C7 /0
    | MOV_MEM_IMM16(addr, imm) ->
        let (addrNum, needsRexB) = encodeRegister addr
        let rex = rexPrefix false false false needsRexB
        let modrm = modrmByte 0uy 0uy addrNum  // mode=00, reg=000 for immediate
        Array.concat [|
            [| 0x66uy; rex; 0xC7uy; modrm |]  // 16-bit prefix + instruction
            BitConverter.GetBytes(imm).[0..1]  // 16-bit immediate
        |]
    
    // ADD r/m64, r64 - Intel manual: 48 01 /r
    | ADD_REG_REG(dst, src) ->
        let (dstNum, needsRexB) = encodeRegister dst
        let (srcNum, needsRexR) = encodeRegister src
        let rex = rexPrefix true needsRexR false needsRexB
        let modrm = modrmByte 3uy srcNum dstNum
        [| rex; 0x01uy; modrm |]
    
    // SUB r/m64, r64 - Intel manual: 48 29 /r
    | SUB_REG_REG(dst, src) ->
        let (dstNum, needsRexB) = encodeRegister dst
        let (srcNum, needsRexR) = encodeRegister src
        let rex = rexPrefix true needsRexR false needsRexB
        let modrm = modrmByte 3uy srcNum dstNum
        [| rex; 0x29uy; modrm |]
    
    // CMP r/m64, r64 - Intel manual: 48 39 /r
    | CMP_REG_REG(reg1, reg2) ->
        let (reg1Num, needsRexB) = encodeRegister reg1
        let (reg2Num, needsRexR) = encodeRegister reg2
        let rex = rexPrefix true needsRexR false needsRexB
        let modrm = modrmByte 3uy reg2Num reg1Num
        [| rex; 0x39uy; modrm |]
    
    // Conditional jump JE rel32 - Intel manual: 0F 84 cd
    | JE_REL32(offset) ->
        Array.concat [|
            [| 0x0Fuy; 0x84uy |]
            int32ToBytes offset
        |]
    
    | JNE_REL32(offset) ->
        Array.concat [|
            [| 0x0Fuy; 0x85uy |]
            int32ToBytes offset
        |]
    
    | JG_REL32(offset) ->
        Array.concat [|
            [| 0x0Fuy; 0x8Fuy |]
            int32ToBytes offset
        |]
    
    | JLE_REL32(offset) ->
        Array.concat [|
            [| 0x0Fuy; 0x8Euy |]
            int32ToBytes offset
        |]
    
    // Set instructions - use low byte of register
    | SETG_REG(reg) ->
        let (regNum, needsRexB) = encodeRegister reg
        let modrm = modrmByte 3uy 0uy regNum  // mode=11, reg=000
        if needsRexB then
            [| 0x41uy; 0x0Fuy; 0x9Fuy; modrm |]  // REX.B prefix for R8-R15
        else
            [| 0x0Fuy; 0x9Fuy; modrm |]
    
    | SETL_REG(reg) ->
        let (regNum, needsRexB) = encodeRegister reg
        let modrm = modrmByte 3uy 0uy regNum
        if needsRexB then
            [| 0x41uy; 0x0Fuy; 0x9Cuy; modrm |]
        else
            [| 0x0Fuy; 0x9Cuy; modrm |]
    
    | SETE_REG(reg) ->
        let (regNum, needsRexB) = encodeRegister reg
        let modrm = modrmByte 3uy 0uy regNum
        if needsRexB then
            [| 0x41uy; 0x0Fuy; 0x94uy; modrm |]
        else
            [| 0x0Fuy; 0x94uy; modrm |]
    
    | SETNE_REG(reg) ->
        let (regNum, needsRexB) = encodeRegister reg
        let modrm = modrmByte 3uy 0uy regNum
        if needsRexB then
            [| 0x41uy; 0x0Fuy; 0x95uy; modrm |]
        else
            [| 0x0Fuy; 0x95uy; modrm |]
    
    | SETGE_REG(reg) ->
        let (regNum, needsRexB) = encodeRegister reg
        let modrm = modrmByte 3uy 0uy regNum
        if needsRexB then
            [| 0x41uy; 0x0Fuy; 0x9Duy; modrm |]
        else
            [| 0x0Fuy; 0x9Duy; modrm |]
    
    | SETLE_REG(reg) ->
        let (regNum, needsRexB) = encodeRegister reg
        let modrm = modrmByte 3uy 0uy regNum
        if needsRexB then
            [| 0x41uy; 0x0Fuy; 0x9Euy; modrm |]
        else
            [| 0x0Fuy; 0x9Euy; modrm |]
    
    // Unconditional jump JMP rel32 - Intel manual: E9 cd
    | JMP_REL32(offset) ->
        Array.concat [|
            [| 0xE9uy |]
            int32ToBytes offset
        |]
    
    // PUSH r64 - Intel manual: 50+r
    | PUSH_REG(reg) ->
        let (regNum, needsRexB) = encodeRegister reg
        if needsRexB then
            let rex = rexPrefix false false false true
            [| rex; 0x50uy + regNum |]
        else
            [| 0x50uy + regNum |]
    
    // POP r64 - Intel manual: 58+r
    | POP_REG(reg) ->
        let (regNum, needsRexB) = encodeRegister reg
        if needsRexB then
            let rex = rexPrefix false false false true
            [| rex; 0x58uy + regNum |]
        else
            [| 0x58uy + regNum |]
    
    // CALL rel32 - Intel manual: E8 cd
    | CALL_REL32(offset) ->
        Array.concat [|
            [| 0xE8uy |]
            int32ToBytes offset
        |]
    
    // TAIL_JMP rel32 - Intel manual: E9 cd (JMP rel32)
    | TAIL_JMP_REL32(offset) ->
        Array.concat [|
            [| 0xE9uy |]
            int32ToBytes offset
        |]
    
    // RET - Intel manual: C3
    | RET ->
        [| 0xC3uy |]
    
    // SYSCALL - Intel manual: 0F 05
    | SYSCALL ->
        [| 0x0Fuy; 0x05uy |]
    
    // Mach system calls - implemented as syscalls with specific RAX values
    | MACH_MSG(timeout) ->
        // Set up Mach message call: RAX = 25 (Mach msg trap number)
        Array.concat [|
            [| 0x48uy; 0xC7uy; 0xC0uy; 0x19uy; 0x00uy; 0x00uy; 0x00uy |]  // MOV RAX, 25
            [| 0x0Fuy; 0x05uy |]  // SYSCALL
        |]
    
    | MACH_TASK_SELF ->
        // Get current task port: RAX = 26
        Array.concat [|
            [| 0x48uy; 0xC7uy; 0xC0uy; 0x1Auy; 0x00uy; 0x00uy; 0x00uy |]  // MOV RAX, 26
            [| 0x0Fuy; 0x05uy |]  // SYSCALL
        |]
    
    | VM_ALLOCATE(size) ->
        // VM allocate: RAX = 10
        Array.concat [|
            [| 0x48uy; 0xC7uy; 0xC0uy; 0x0Auy; 0x00uy; 0x00uy; 0x00uy |]  // MOV RAX, 10
            int64ToBytes size  // Size parameter in RDI (set by caller)
            [| 0x0Fuy; 0x05uy |]  // SYSCALL
        |]
    
    // NUMA-aware allocation - custom syscall
    | NUMA_ALLOC(size, node) ->
        Array.concat [|
            [| 0x48uy; 0xC7uy; 0xC0uy; 0x64uy; 0x00uy; 0x00uy; 0x00uy |]  // MOV RAX, 100 (custom)
            int64ToBytes size
            int32ToBytes node
            [| 0x0Fuy; 0x05uy |]  // SYSCALL
        |]
    
    // CPU-local allocation - custom syscall
    | CPU_LOCAL_ALLOC(size, cpu) ->
        Array.concat [|
            [| 0x48uy; 0xC7uy; 0xC0uy; 0x65uy; 0x00uy; 0x00uy; 0x00uy |]  // MOV RAX, 101
            int64ToBytes size
            int32ToBytes cpu
            [| 0x0Fuy; 0x05uy |]  // SYSCALL
        |]
    
    // Cache-aligned allocation - custom syscall
    | CACHE_ALIGN_ALLOC(size) ->
        Array.concat [|
            [| 0x48uy; 0xC7uy; 0xC0uy; 0x66uy; 0x00uy; 0x00uy; 0x00uy |]  // MOV RAX, 102
            int64ToBytes size
            [| 0x0Fuy; 0x05uy |]  // SYSCALL
        |]
    
    // INT3 - breakpoint/exception instruction
    | INT3 ->
        [| 0xCCuy |]  // INT3 opcode
    
    // MUL and DIV need more complex encoding - simplified for now
    | MUL_REG(reg) ->
        let (regNum, needsRexB) = encodeRegister reg
        let rex = rexPrefix true false false needsRexB
        let modrm = modrmByte 3uy 4uy regNum  // /4 for MUL
        [| rex; 0xF7uy; modrm |]
    
    | DIV_REG(reg) ->
        let (regNum, needsRexB) = encodeRegister reg
        let rex = rexPrefix true false false needsRexB
        let modrm = modrmByte 3uy 6uy regNum  // /6 for DIV
        [| rex; 0xF7uy; modrm |]

// ==================== CODE GENERATION CONTEXT ====================

type CodeGenContext = {
    Instructions: Instruction list
    Labels: Map<string, int>
    Relocations: (int * string) list  // (offset, label)
    NextRegister: int
    StackOffset: int
    SymbolTable: Map<string, Register>  // Variable name to register mapping
    ModuleEnv: ModuleEnvironment  // Module environment for name resolution
}

let emptyContext = {
    Instructions = []
    Labels = Map.empty
    Relocations = []
    NextRegister = 0
    StackOffset = 0
    SymbolTable = Map.empty
    ModuleEnv = emptyEnvironment
}

/// Allocate next available register (avoiding RSP and RBP)
let allocRegister (ctx: CodeGenContext) : CodeGenContext * Register =
    // Skip RSP (4) and RBP (5)
    let availableRegs = [| 0; 1; 2; 3; 6; 7; 8; 9; 10; 11; 12; 13; 14; 15 |]
    let regIndex = ctx.NextRegister % availableRegs.Length
    let reg = enum<Register> availableRegs.[regIndex]
    let newCtx = { ctx with NextRegister = ctx.NextRegister + 1 }
    (newCtx, reg)

/// Emit instruction
let emit (ctx: CodeGenContext) (instr: Instruction) : CodeGenContext =
    { ctx with Instructions = instr :: ctx.Instructions }

/// Add label at current position
let addLabel (ctx: CodeGenContext) (label: string) : CodeGenContext =
    let position = List.length ctx.Instructions
    { ctx with Labels = Map.add label position ctx.Labels }

// ==================== DISCRIMINATED UNION CODE GENERATION ====================

/// Register type definition in compilation context
let registerTypeDefinition (ctx: CodeGenContext) (typeDef: TypeDefinition) : CodeGenContext =
    match typeDef with
    | TypeUnion(name, typeParams, cases) ->
        // In full implementation, would store constructor->tag mapping
        // For now, just add to context as comment
        ctx
    | TypeAlias(name, typeParams, aliasType) ->
        // Type alias - no runtime representation needed
        ctx
    | TypeRecord(name, typeParams, fields) ->
        // Record type - would calculate field offsets
        ctx

/// Get constructor tag number (simplified mapping)
let getConstructorTag (constructorName: string) : int =
    match constructorName with
    | "None" -> 0
    | "Some" -> 1
    | "Ok" -> 0  
    | "Error" -> 1
    | "Left" -> 0
    | "Right" -> 1
    | _ -> 
        // Hash constructor name to get consistent tag
        constructorName.GetHashCode() &&& 0xFF

/// Compile pattern matching for union constructors  
let compileUnionPattern (ctx: CodeGenContext) (valueReg: Register) (constructorName: string) (argPatterns: Pattern list) : CodeGenContext * bool =
    // Load tag from union value (assume tag is at offset 0)
    let (ctx2, tagReg) = allocRegister ctx
    let expectedTag = getConstructorTag constructorName
    
    // For now, simplified: assume valueReg contains the tag directly
    let (ctx3, expectedReg) = allocRegister ctx2
    let ctx4 = emit ctx3 (MOV_REG_IMM64(expectedReg, int64 expectedTag))
    let ctx5 = emit ctx4 (CMP_REG_REG(valueReg, expectedReg))
    
    // In full implementation:
    // 1. Load tag from memory: MOV tagReg, [valueReg + 0]
    // 2. Compare with expected tag
    // 3. If match, extract fields and match arg patterns
    
    (ctx5, false)  // Caller checks flags with JE

// ==================== TAIL CALL OPTIMIZATION ====================

/// Check if an expression is in tail position
let rec isInTailPosition (expr: FSharpAST) (parent: FSharpAST option) : bool =
    match parent with
    | None -> true  // Top level is tail position
    | Some p ->
        match p with
        // Last expression in let body is tail position
        | ASTLet(_, _, body) when obj.ReferenceEquals(body, expr) -> true
        | ASTLetRec(_, body) when obj.ReferenceEquals(body, expr) -> true
        
        // Both branches of if are tail positions
        | ASTIf(_, thenExpr, elseExpr) ->
            obj.ReferenceEquals(thenExpr, expr) || obj.ReferenceEquals(elseExpr, expr)
            
        // Last expression in match cases
        | ASTMatch(_, cases) ->
            cases |> List.exists (fun (Case(_, _, caseExpr)) ->
                obj.ReferenceEquals(caseExpr, expr))
                
        // Last in sequence
        | ASTSequence(_, second) when obj.ReferenceEquals(second, expr) -> true
        
        // Lambda body is tail position
        | ASTLambda(_, body) when obj.ReferenceEquals(body, expr) -> true
        
        | _ -> false

/// Detect if a function call is recursive (calling same function)
let isRecursiveCall (funcName: string) (expr: FSharpAST) : bool =
    match expr with
    | ASTApp(ASTIdent name, _) -> name = funcName
    | _ -> false

/// Check if an expression is a tail recursive call
let isTailRecursiveCall (currentFunc: string) (parent: FSharpAST) (expr: FSharpAST) : bool =
    isInTailPosition expr (Some parent) && isRecursiveCall currentFunc expr

/// Optimize tail calls by replacing recursive calls with jumps
let rec optimizeTailCalls (currentFunc: string) (ast: FSharpAST) : FSharpAST =
    match ast with
    // Function definitions - track the current function name
    | ASTLet(name, ASTLambda(params, body), rest) ->
        let optimizedBody = optimizeTailCallsInContext name params body
        let optimizedRest = optimizeTailCalls currentFunc rest
        ASTLet(name, ASTLambda(params, optimizedBody), optimizedRest)
        
    // Recursive function definitions
    | ASTLetRec(bindings, body) ->
        let optimizedBindings = bindings |> List.map (fun (name, func) ->
            match func with
            | ASTLambda(params, lambdaBody) ->
                let optimizedBody = optimizeTailCallsInContext name params lambdaBody
                (name, ASTLambda(params, optimizedBody))
            | _ -> (name, func))
        let optimizedRest = optimizeTailCalls currentFunc body
        ASTLetRec(optimizedBindings, optimizedRest)
        
    // Recursive expressions
    | ASTIf(cond, thenExpr, elseExpr) ->
        ASTIf(
            optimizeTailCalls currentFunc cond,
            optimizeTailCalls currentFunc thenExpr,
            optimizeTailCalls currentFunc elseExpr
        )
        
    | ASTSequence(first, second) ->
        ASTSequence(
            optimizeTailCalls currentFunc first,
            optimizeTailCalls currentFunc second
        )
        
    | ASTMatch(expr, cases) ->
        let optimizedCases = cases |> List.map (fun (Case(pat, guard, caseExpr)) ->
            Case(pat, guard, optimizeTailCalls currentFunc caseExpr))
        ASTMatch(optimizeTailCalls currentFunc expr, optimizedCases)
        
    // Other expressions pass through
    | _ -> ast

and optimizeTailCallsInContext (funcName: string) (params: string list) (body: FSharpAST) : FSharpAST =
    // Transform the function body, marking tail calls
    // A call is in tail position if it's the last thing that happens before returning
    let rec transformBodyInTailPos (ast: FSharpAST) (inTailPos: bool) : FSharpAST =
        match ast with
        | ASTApp(ASTIdent name, args) when name = funcName && inTailPos ->
            // This is a recursive call in tail position - mark for optimization
            ASTApp(ASTIdent ("__tail_" + name), args)
            
        | ASTApp(func, args) ->
            // Non-tail recursive call - transform recursively
            ASTApp(transformBodyInTailPos func false, transformBodyInTailPos args false)
                
        | ASTIf(cond, thenExpr, elseExpr) ->
            // Both branches are in tail position if the if is in tail position
            ASTIf(
                transformBodyInTailPos cond false,
                transformBodyInTailPos thenExpr inTailPos,
                transformBodyInTailPos elseExpr inTailPos
            )
            
        | ASTSequence(first, second) ->
            // Only the second part is in tail position
            ASTSequence(
                transformBodyInTailPos first false,
                transformBodyInTailPos second inTailPos
            )
            
        | ASTMatch(expr, cases) ->
            // All case expressions are in tail position if the match is
            let newCases = cases |> List.map (fun (Case(pat, guard, caseExpr)) ->
                let newGuard = guard |> Option.map (fun g -> transformBodyInTailPos g false)
                Case(pat, newGuard, transformBodyInTailPos caseExpr inTailPos))
            ASTMatch(transformBodyInTailPos expr false, newCases)
            
        | _ -> ast
    
    // Start with the body in tail position (it's the return value of the function)
    transformBodyInTailPos body true

// ==================== F# AST TO x86-64 COMPILATION ====================

/// Calculate instruction size for offset calculation
let getInstructionSize (instr: Instruction) : int =
    match instr with
    | MOV_REG_IMM64(reg, _) -> 
        // REX.W + opcode + imm64
        if int reg < 8 then 10 else 11  // Extra byte for REX.B if r8-r15
    | MOV_REG_REG(_, _) -> 3     // REX.W + opcode + ModRM
    | MOV_MEM_REG(_, offset, _) ->
        if offset = 0 then 3      // REX.W + opcode + ModRM
        elif offset >= -128 && offset <= 127 then 4  // + disp8
        else 7                     // + disp32
    | MOV_REG_MEM(_, _, offset) ->
        if offset = 0 then 3      // REX.W + opcode + ModRM
        elif offset >= -128 && offset <= 127 then 4  // + disp8
        else 7                     // + disp32
    | CMP_REG_REG(_, _) -> 3     // REX.W + opcode + ModRM
    | JE_REL32(_) -> 6            // 0F 84 + rel32
    | JNE_REL32(_) -> 6           // 0F 85 + rel32
    | JG_REL32(_) -> 6            // 0F 8F + rel32
    | JLE_REL32(_) -> 6           // 0F 8E + rel32
    | JMP_REL32(_) -> 5           // E9 + rel32
    | ADD_REG_REG(_, _) -> 3      // REX.W + opcode + ModRM
    | SUB_REG_REG(_, _) -> 3      // REX.W + opcode + ModRM
    | MUL_REG(_) -> 3             // REX.W + F7 /4
    | DIV_REG(_) -> 3             // REX.W + F7 /6
    | SETG_REG(_) -> 4            // 0F 9F + ModRM (+ REX if needed)
    | SETL_REG(_) -> 4            // 0F 9C + ModRM
    | SETE_REG(_) -> 4            // 0F 94 + ModRM
    | SETNE_REG(_) -> 4           // 0F 95 + ModRM
    | SETGE_REG(_) -> 4           // 0F 9D + ModRM
    | SETLE_REG(_) -> 4           // 0F 9E + ModRM
    | PUSH_REG(reg) -> if int reg < 8 then 1 else 2  // 50+r or REX.B + 50+r
    | POP_REG(reg) -> if int reg < 8 then 1 else 2   // 58+r or REX.B + 58+r
    | CALL_REL32(_) -> 5          // E8 + rel32
    | TAIL_JMP_REL32(_) -> 5      // E9 + rel32
    | RET -> 1                    // C3
    | SYSCALL -> 2                // 0F 05
    | INT3 -> 1                   // CC
    | _ -> 4                      // Conservative estimate

let rec compileExpression (ctx: CodeGenContext) (ast: FSharpAST) : CodeGenContext * Register =
    match ast with
    
    // Compile declaration sequence: compile each declaration in order, return last result
    | ASTDeclarationSequence declarations ->
        let rec compileDeclarations ctx decls lastReg =
            match decls with
            | [] -> (ctx, lastReg)
            | [last] -> 
                let (finalCtx, finalReg) = compileExpression ctx last
                (finalCtx, finalReg)
            | decl :: rest ->
                let (newCtx, _) = compileExpression ctx decl
                compileDeclarations newCtx rest Register.RAX  // Use RAX as default
        
        compileDeclarations ctx declarations Register.RAX
        
    // Module declarations - update environment
    | ASTModule moduleDecl ->
        let newEnv = addModule ctx.ModuleEnv moduleDecl
        let ctx2 = { ctx with ModuleEnv = newEnv }
        let (ctx3, reg) = allocRegister ctx2
        let ctx4 = emit ctx3 (MOV_REG_IMM64(reg, 0L))  // Unit value as 0
        (ctx4, reg)
    
    | ASTNamespace(name, modules) ->
        let newEnv = List.fold addModule ctx.ModuleEnv modules
        let ctx2 = { ctx with ModuleEnv = newEnv }
        let (ctx3, reg) = allocRegister ctx2
        let ctx4 = emit ctx3 (MOV_REG_IMM64(reg, 0L))  // Unit value as 0
        (ctx4, reg)
    
    | ASTOpen path ->
        let newEnv = openModule ctx.ModuleEnv path
        let ctx2 = { ctx with ModuleEnv = newEnv }
        let (ctx3, reg) = allocRegister ctx2
        let ctx4 = emit ctx3 (MOV_REG_IMM64(reg, 0L))  // Unit value as 0
        (ctx4, reg)
    
    // Compile integer literal: mov reg, immediate
    | ASTLiteral(LitInt value) ->
        let (ctx2, reg) = allocRegister ctx
        let ctx3 = emit ctx2 (MOV_REG_IMM64(reg, value))
        (ctx3, reg)
    
    // Compile boolean literal: mov reg, 0/1
    | ASTLiteral(LitBool value) ->
        let (ctx2, reg) = allocRegister ctx
        let imm = if value then 1L else 0L
        let ctx3 = emit ctx2 (MOV_REG_IMM64(reg, imm))
        (ctx3, reg)
    
    // Compile string literal - allocate string in memory
    | ASTLiteral(LitString value) ->
        // Calculate string size (length + null terminator)
        let strSize = value.Length + 1
        
        // Allocate memory for string
        let (ctx2, sizeReg) = allocRegister ctx
        let ctx3 = emit ctx2 (MOV_REG_IMM64(sizeReg, int64 strSize))
        
        // Use mmap to allocate memory
        let ctx4 = emit ctx3 (MOV_REG_IMM64(Register.RDI, 0L))  // addr = NULL
        let ctx5 = emit ctx4 (MOV_REG_REG(Register.RSI, sizeReg))  // length
        let ctx6 = emit ctx5 (MOV_REG_IMM64(Register.RDX, 3L))  // PROT_READ|PROT_WRITE
        let ctx7 = emit ctx6 (MOV_REG_IMM64(Register.R10, 0x22L))  // MAP_PRIVATE|MAP_ANONYMOUS
        let ctx8 = emit ctx7 (MOV_REG_IMM64(Register.R8, -1L))  // fd = -1
        let ctx9 = emit ctx8 (MOV_REG_IMM64(Register.R9, 0L))  // offset = 0
        let ctx10 = emit ctx9 (MOV_REG_IMM64(Register.RAX, 9L))  // mmap syscall
        let ctx11 = emit ctx10 SYSCALL
        
        // RAX now contains pointer to string memory
        let (ctx12, strReg) = allocRegister ctx11
        let ctx13 = emit ctx12 (MOV_REG_REG(strReg, Register.RAX))
        
        // Copy string bytes to memory
        let ctx14 = 
            value.ToCharArray()
            |> Array.fold (fun (accCtx, idx) ch ->
                let (tempCtx, tempReg) = allocRegister accCtx
                let ctx' = emit tempCtx (MOV_REG_IMM64(tempReg, int64 (int ch)))
                let ctx'' = emit ctx' (MOV_MEM_REG(strReg, idx, tempReg))
                (ctx'', idx + 1)
            ) (ctx13, 0)
            |> fst
        
        // Add null terminator
        let (ctx15, zeroReg) = allocRegister ctx14
        let ctx16 = emit ctx15 (MOV_REG_IMM64(zeroReg, 0L))
        let ctx17 = emit ctx16 (MOV_MEM_REG(strReg, value.Length, zeroReg))
        
        (ctx17, strReg)
    
    // Compile variable reference - lookup in symbol table and modules
    | ASTIdent name ->
        match Map.tryFind name ctx.SymbolTable with
        | Some reg ->
            // Variable found in symbol table - return its register
            (ctx, reg)
        | None ->
            // Try to resolve through module system
            match resolveName ctx.ModuleEnv name with
            | Some (MemberValue(_, value, _), _) ->
                // Found in module - compile the value
                compileExpression ctx value
            | _ ->
                // Variable not found - allocate new register with default value
                let (ctx2, reg) = allocRegister ctx
                let ctx3 = emit ctx2 (MOV_REG_IMM64(reg, 0L))
                (ctx3, reg)
    
    // Handle qualified names like A.x or System.Console.WriteLine
    | ASTLongIdent path ->
        match resolveQualifiedName ctx.ModuleEnv path with
        | Some (MemberValue(name, value, _), modulePath) ->
            // Found value in module - compile it
            compileExpression ctx value
        | Some (MemberFunction(_, params, body, _), _) ->
            // Found function - for now just compile as placeholder
            let (ctx2, reg) = allocRegister ctx
            let ctx3 = emit ctx2 (MOV_REG_IMM64(reg, 0L))
            (ctx3, reg)
        | None ->
            // Not found - compile as placeholder
            let (ctx2, reg) = allocRegister ctx
            let ctx3 = emit ctx2 (MOV_REG_IMM64(reg, 0L))
            (ctx3, reg)
    
    // Handle dot access like expr.field
    | ASTDotAccess(expr, field) ->
        // For now, just compile the expression and ignore field access
        // TODO: Implement proper field/property access
        compileExpression ctx expr
    
    // Compile let binding: let name = value in body
    | ASTLet(name, value, body) ->
        // Compile the value first
        let (ctx2, valueReg) = compileExpression ctx value
        
        // Add binding to symbol table
        let newSymbolTable = Map.add name valueReg ctx2.SymbolTable
        let ctx3 = { ctx2 with SymbolTable = newSymbolTable }
        
        // Compile the body with the new binding
        let (ctx4, bodyReg) = compileExpression ctx3 body
        
        // The result is the body expression
        (ctx4, bodyReg)
    
    // Compile recursive let binding: let rec f = ... and g = ... in body
    | ASTLetRec(bindings, body) ->
        // For recursive bindings, we need to allocate registers first,
        // then compile the definitions with all names in scope
        
        // Allocate registers for all recursive bindings
        let (ctx2, bindingRegs) = 
            List.fold (fun (accCtx, accRegs) (name, _) ->
                let (newCtx, reg) = allocRegister accCtx
                (newCtx, (name, reg) :: accRegs)
            ) (ctx, []) bindings
        
        // Add all bindings to symbol table before compiling values
        let newSymbolTable = 
            List.fold (fun symTable (name, reg) ->
                Map.add name reg symTable
            ) ctx2.SymbolTable bindingRegs
        
        let ctx3 = { ctx2 with SymbolTable = newSymbolTable }
        
        // Now compile each binding value with all names in scope
        let ctx4 = 
            List.fold2 (fun accCtx (name, value) (_, reg) ->
                // For functions, generate a label for recursion
                match value with
                | ASTLambda(params, funcBody) ->
                    let funcLabel = sprintf "rec_%s_%d" name (List.length accCtx.Instructions)
                    let labelCtx = addLabel accCtx funcLabel
                    let pushCtx = emit labelCtx (PUSH_REG(Register.RBP))
                    let frameCtx = emit pushCtx (MOV_REG_REG(Register.RBP, Register.RSP))
                    
                    // Compile function body
                    let (bodyCtx, bodyReg) = compileExpression frameCtx funcBody
                    let retCtx = emit bodyCtx (MOV_REG_REG(Register.RAX, bodyReg))
                    let epilogueCtx = emit retCtx (MOV_REG_REG(Register.RSP, Register.RBP))
                    let popCtx = emit epilogueCtx (POP_REG(Register.RBP))
                    let finalCtx = emit popCtx RET
                    
                    // Store function address in register
                    emit finalCtx (MOV_REG_IMM64(reg, 0L))  // Placeholder for function address
                | _ ->
                    // Non-function recursive binding
                    let (valueCtx, valueReg) = compileExpression accCtx value
                    emit valueCtx (MOV_REG_REG(reg, valueReg))
            ) ctx3 bindings bindingRegs
        
        // Compile the body with all recursive bindings in scope
        compileExpression ctx4 body
    
    // Compile binary operations
    | ASTBinaryOp(OpAdd, left, right) ->
        let (ctx2, leftReg) = compileExpression ctx left
        let (ctx3, rightReg) = compileExpression ctx2 right
        let ctx4 = emit ctx3 (ADD_REG_REG(leftReg, rightReg))
        (ctx4, leftReg)  // Result in left register
    
    | ASTBinaryOp(OpSubtract, left, right) ->
        let (ctx2, leftReg) = compileExpression ctx left
        let (ctx3, rightReg) = compileExpression ctx2 right
        let ctx4 = emit ctx3 (SUB_REG_REG(leftReg, rightReg))
        (ctx4, leftReg)
    
    // Comparison operators - set result to 1 (true) or 0 (false)
    | ASTBinaryOp(OpGreater, left, right) ->
        let (ctx2, leftReg) = compileExpression ctx left
        let (ctx3, rightReg) = compileExpression ctx2 right
        let ctx4 = emit ctx3 (CMP_REG_REG(leftReg, rightReg))
        let (ctx5, resultReg) = allocRegister ctx4
        // Set result to 0, then conditionally set to 1
        let ctx6 = emit ctx5 (MOV_REG_IMM64(resultReg, 0L))
        let ctx7 = emit ctx6 (SETG_REG(resultReg))  // Set if greater
        (ctx7, resultReg)
    
    | ASTBinaryOp(OpLess, left, right) ->
        let (ctx2, leftReg) = compileExpression ctx left
        let (ctx3, rightReg) = compileExpression ctx2 right
        let ctx4 = emit ctx3 (CMP_REG_REG(leftReg, rightReg))
        let (ctx5, resultReg) = allocRegister ctx4
        let ctx6 = emit ctx5 (MOV_REG_IMM64(resultReg, 0L))
        let ctx7 = emit ctx6 (SETL_REG(resultReg))  // Set if less
        (ctx7, resultReg)
    
    | ASTBinaryOp(OpEqual, left, right) ->
        let (ctx2, leftReg) = compileExpression ctx left
        let (ctx3, rightReg) = compileExpression ctx2 right
        let ctx4 = emit ctx3 (CMP_REG_REG(leftReg, rightReg))
        let (ctx5, resultReg) = allocRegister ctx4
        let ctx6 = emit ctx5 (MOV_REG_IMM64(resultReg, 0L))
        let ctx7 = emit ctx6 (SETE_REG(resultReg))  // Set if equal
        (ctx7, resultReg)
    
    | ASTBinaryOp(OpNotEqual, left, right) ->
        let (ctx2, leftReg) = compileExpression ctx left
        let (ctx3, rightReg) = compileExpression ctx2 right
        let ctx4 = emit ctx3 (CMP_REG_REG(leftReg, rightReg))
        let (ctx5, resultReg) = allocRegister ctx4
        let ctx6 = emit ctx5 (MOV_REG_IMM64(resultReg, 0L))
        let ctx7 = emit ctx6 (SETNE_REG(resultReg))  // Set if not equal
        (ctx7, resultReg)
    
    | ASTBinaryOp(OpGreaterEq, left, right) ->
        let (ctx2, leftReg) = compileExpression ctx left
        let (ctx3, rightReg) = compileExpression ctx2 right
        let ctx4 = emit ctx3 (CMP_REG_REG(leftReg, rightReg))
        let (ctx5, resultReg) = allocRegister ctx4
        let ctx6 = emit ctx5 (MOV_REG_IMM64(resultReg, 0L))
        let ctx7 = emit ctx6 (SETGE_REG(resultReg))  // Set if greater or equal
        (ctx7, resultReg)
    
    | ASTBinaryOp(OpLessEq, left, right) ->
        let (ctx2, leftReg) = compileExpression ctx left
        let (ctx3, rightReg) = compileExpression ctx2 right
        let ctx4 = emit ctx3 (CMP_REG_REG(leftReg, rightReg))
        let (ctx5, resultReg) = allocRegister ctx4
        let ctx6 = emit ctx5 (MOV_REG_IMM64(resultReg, 0L))
        let ctx7 = emit ctx6 (SETLE_REG(resultReg))  // Set if less or equal
        (ctx7, resultReg)
    
    // Logical operators
    | ASTBinaryOp(OpAnd, left, right) ->
        // Short-circuit evaluation: if left is false, don't evaluate right
        let (ctx2, leftReg) = compileExpression ctx left
        let (ctx3, zeroReg) = allocRegister ctx2
        let ctx4 = emit ctx3 (MOV_REG_IMM64(zeroReg, 0L))
        let ctx5 = emit ctx4 (CMP_REG_REG(leftReg, zeroReg))
        let ctx6 = emit ctx5 (JE_REL32(0))  // Jump if left is false
        let (ctx7, rightReg) = compileExpression ctx6 right
        let (ctx8, resultReg) = allocRegister ctx7
        let ctx9 = emit ctx8 (MOV_REG_REG(resultReg, rightReg))
        (ctx9, resultReg)
    
    // String concatenation
    | ASTBinaryOp(OpConcat, left, right) ->
        // Compile both string operands
        let (ctx2, leftReg) = compileExpression ctx left
        let (ctx3, rightReg) = compileExpression ctx2 right
        
        // Calculate lengths (would need strlen in real implementation)
        // For now, assume strings have length stored at offset -8
        // This is a simplification - real implementation would be more complex
        
        // Allocate new string with combined length
        let (ctx4, len1Reg) = allocRegister ctx3
        let (ctx5, len2Reg) = allocRegister ctx4
        let (ctx6, totalLenReg) = allocRegister ctx5
        
        // For simplicity, assume fixed-length strings for now
        // In production, would need proper string length tracking
        let ctx7 = emit ctx6 (MOV_REG_IMM64(totalLenReg, 256L))  // Max string size
        
        // Allocate memory for result
        let ctx8 = emit ctx7 (MOV_REG_IMM64(Register.RDI, 0L))
        let ctx9 = emit ctx8 (MOV_REG_REG(Register.RSI, totalLenReg))
        let ctx10 = emit ctx9 (MOV_REG_IMM64(Register.RDX, 3L))
        let ctx11 = emit ctx10 (MOV_REG_IMM64(Register.R10, 0x22L))
        let ctx12 = emit ctx11 (MOV_REG_IMM64(Register.R8, -1L))
        let ctx13 = emit ctx12 (MOV_REG_IMM64(Register.R9, 0L))
        let ctx14 = emit ctx13 (MOV_REG_IMM64(Register.RAX, 9L))
        let ctx15 = emit ctx14 SYSCALL
        
        let (ctx16, resultReg) = allocRegister ctx15
        let ctx17 = emit ctx16 (MOV_REG_REG(resultReg, Register.RAX))
        
        // Copy strings (simplified - would need proper strcpy)
        // Return pointer to concatenated string
        (ctx17, resultReg)
    
    | ASTBinaryOp(OpOr, left, right) ->
        // Short-circuit evaluation: if left is true, don't evaluate right
        let (ctx2, leftReg) = compileExpression ctx left
        let (ctx3, zeroReg) = allocRegister ctx2
        let ctx4 = emit ctx3 (MOV_REG_IMM64(zeroReg, 0L))
        let ctx5 = emit ctx4 (CMP_REG_REG(leftReg, zeroReg))
        let ctx6 = emit ctx5 (JNE_REL32(0))  // Jump if left is true
        let (ctx7, rightReg) = compileExpression ctx6 right
        let (ctx8, resultReg) = allocRegister ctx7
        let ctx9 = emit ctx8 (MOV_REG_REG(resultReg, rightReg))
        (ctx9, resultReg)
    
    // Compile function application: call convention
    | ASTApp(func, arg) ->
        match func with
        // Tail call optimization: jump instead of call + return
        | ASTIdent name when name.StartsWith("__tail_") ->
            let actualName = name.Substring(7)  // Remove "__tail_" prefix
            
            // Compile argument and place in calling convention register
            let (ctx2, argReg) = compileExpression ctx arg
            let ctx3 = emit ctx2 (MOV_REG_REG(Register.RDI, argReg))
            
            // Tail call: jump to function start (reuse stack frame)
            let ctx4 = emit ctx3 (TAIL_JMP_REL32(0))  // TODO: Calculate actual offset to function start
            
            // For tail calls, we don't need a result register because we're jumping
            let (ctx5, resultReg) = allocRegister ctx4
            (ctx5, resultReg)
            
        // Regular function call
        | _ ->
            // Simplified: compile argument, then function, then call
            let (ctx2, argReg) = compileExpression ctx arg
            let (ctx3, funcReg) = compileExpression ctx2 func
            // Move argument to RDI (first parameter register)
            let ctx4 = emit ctx3 (MOV_REG_REG(Register.RDI, argReg))
            let ctx5 = emit ctx4 (CALL_REL32(0))  // TODO: Proper function addresses
            let (ctx6, resultReg) = allocRegister ctx5
            let ctx7 = emit ctx6 (MOV_REG_REG(resultReg, Register.RAX))  // Result in RAX
            (ctx7, resultReg)
    
    // Compile if expression: compare + conditional jump
    | ASTIf(cond, thenExpr, elseExpr) ->
        let (ctx2, condReg) = compileExpression ctx cond
        let (ctx3, zeroReg) = allocRegister ctx2
        let ctx4 = emit ctx3 (MOV_REG_IMM64(zeroReg, 0L))
        let ctx5 = emit ctx4 (CMP_REG_REG(condReg, zeroReg))
        
        // Jump to else if condition is false (zero)
        let ctx6 = emit ctx5 (JE_REL32(0))  // Placeholder offset
        let elseJumpPos = List.length ctx6.Instructions - 1
        
        // Compile then branch
        let (ctx7, thenReg) = compileExpression ctx6 thenExpr
        let ctx8 = emit ctx7 (JMP_REL32(0))  // Jump over else
        let endJumpPos = List.length ctx8.Instructions - 1
        
        // Compile else branch
        let elseLabel = sprintf "else_%d" elseJumpPos
        let ctx9 = addLabel ctx8 elseLabel
        let (ctx10, elseReg) = compileExpression ctx9 elseExpr
        
        // Both results should be in same register for consistent output
        let (ctx11, resultReg) = allocRegister ctx10
        let ctx12 = emit ctx11 (MOV_REG_REG(resultReg, thenReg))
        let ctx13 = emit ctx12 (MOV_REG_REG(resultReg, elseReg))
        
        (ctx13, resultReg)
    
    // Compile lambda: create function prologue
    | ASTLambda(parameters, body) ->
        let funcLabel = sprintf "lambda_%d" (List.length ctx.Instructions)
        let ctx2 = addLabel ctx funcLabel
        let ctx3 = emit ctx2 (PUSH_REG(Register.RBP))
        let ctx4 = emit ctx3 (MOV_REG_REG(Register.RBP, Register.RSP))
        
        // Compile function body
        let (ctx5, bodyReg) = compileExpression ctx4 body
        let ctx6 = emit ctx5 (MOV_REG_REG(Register.RAX, bodyReg))  // Return value
        
        // Function epilogue
        let ctx7 = emit ctx6 (MOV_REG_REG(Register.RSP, Register.RBP))
        let ctx8 = emit ctx7 (POP_REG(Register.RBP))
        let ctx9 = emit ctx8 RET
        
        // Return function address (simplified)
        let (ctx10, funcPtrReg) = allocRegister ctx9
        let ctx11 = emit ctx10 (MOV_REG_IMM64(funcPtrReg, 0L))  // TODO: Actual address
        (ctx11, funcPtrReg)
    
    // Pattern matching: compile to series of conditional tests
    | ASTMatch(expr, cases) ->
        let (ctx2, exprReg) = compileExpression ctx expr
        compilePatternMatching ctx2 exprReg cases
    
    // Exception handling: try-with-finally
    | ASTTry(tryExpr, handlers, finallyOpt) ->
        // Save current stack pointer for unwinding
        let (ctx2, stackSaveReg) = allocRegister ctx
        let ctx3 = emit ctx2 (MOV_REG_REG(stackSaveReg, Register.RSP))
        
        // Set up exception handler frame
        let tryLabel = sprintf "try_%d" (List.length ctx3.Instructions)
        let handlerLabel = sprintf "handler_%d" (List.length ctx3.Instructions)
        let finallyLabel = sprintf "finally_%d" (List.length ctx3.Instructions)
        let endLabel = sprintf "try_end_%d" (List.length ctx3.Instructions)
        
        // Try block
        let ctx4 = addLabel ctx3 tryLabel
        let (ctx5, tryResultReg) = compileExpression ctx4 tryExpr
        
        // Jump to finally if no exception
        let finallyJumpPos = List.length ctx5.Instructions
        let ctx6 = emit ctx5 (JMP_REL32(0))  // Will be patched
        
        // Exception handler
        let ctx7 = addLabel ctx6 handlerLabel
        let (ctx8, handlerResultReg) = compileExceptionHandlers ctx7 handlers
        
        // Finally block (if present)
        let ctx9 = if Option.isSome finallyOpt then
                      let ctx = addLabel ctx8 finallyLabel
                      let (ctx', _) = compileExpression ctx (Option.get finallyOpt)
                      ctx'
                   else ctx8
        
        // End label
        let ctx10 = addLabel ctx9 endLabel
        
        // Patch jump offset  
        // Simple implementation - in real code would calculate actual offset
        let ctx11 = patchJump ctx10 finallyJumpPos 10
        
        // Result is either try result or handler result
        (ctx11, tryResultReg)
    
    // Exception raising: raise expr
    | ASTRaise expr ->
        // Compile the exception value
        let (ctx2, exceptionReg) = compileExpression ctx expr
        
        // Set exception register (by convention, use RDI for exception value)
        let ctx3 = emit ctx2 (MOV_REG_REG(Register.RDI, exceptionReg))
        
        // Trigger exception - using INT3 as a simple exception mechanism
        // In a real implementation, this would unwind the stack to find handlers
        let ctx4 = emit ctx3 INT3
        
        // Return dummy register (unreachable after raise)
        let (ctx5, dummyReg) = allocRegister ctx4
        (ctx5, dummyReg)
    
    // Type definition - store type info and compile body
    | ASTTypeDefinition(typeDef, body) ->
        // For now, just compile the body - type info would be stored in context
        // In full implementation, would register constructors in symbol table
        let ctx2 = registerTypeDefinition ctx typeDef
        compileExpression ctx2 body
    
    // Constructor application - create tagged union value
    | ASTConstructor(name, args) ->
        match args with
        | [] ->
            // Nullary constructor - just the tag
            let (ctx2, tagReg) = allocRegister ctx
            let tag = getConstructorTag name
            let ctx3 = emit ctx2 (MOV_REG_IMM64(tagReg, int64 tag))
            (ctx3, tagReg)
        
        | [singleArg] ->
            // Unary constructor - allocate memory for tag + value
            let (ctx2, tagReg) = allocRegister ctx
            let tag = getConstructorTag name
            let ctx3 = emit ctx2 (MOV_REG_IMM64(tagReg, int64 tag))
            
            // Compile the argument
            let (ctx4, argReg) = compileExpression ctx3 singleArg
            
            // For now, just return tag (simplified)
            (ctx4, tagReg)
        
        | multipleArgs ->
            // N-ary constructor - for now just return tag
            let (ctx2, tagReg) = allocRegister ctx
            let tag = getConstructorTag name
            let ctx3 = emit ctx2 (MOV_REG_IMM64(tagReg, int64 tag))
            (ctx3, tagReg)
    
    // Compile array literal [| e1; e2; ... |]
    | ASTArray elements ->
        // Allocate memory for array: 8 bytes for length + 8 bytes per element
        let arraySize = 8 + (elements.Length * 8)
        let (ctx2, sizeReg) = allocRegister ctx
        let ctx3 = emit ctx2 (MOV_REG_IMM64(sizeReg, int64 arraySize))
        
        // Call malloc (using syscall - mmap for simplicity)
        // mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0)
        let ctx4 = emit ctx3 (MOV_REG_IMM64(Register.RDI, 0L))  // addr = NULL
        let ctx5 = emit ctx4 (MOV_REG_REG(Register.RSI, sizeReg))  // length
        let ctx6 = emit ctx5 (MOV_REG_IMM64(Register.RDX, 3L))  // PROT_READ|PROT_WRITE
        let ctx7 = emit ctx6 (MOV_REG_IMM64(Register.R10, 0x22L))  // MAP_PRIVATE|MAP_ANONYMOUS
        let ctx8 = emit ctx7 (MOV_REG_IMM64(Register.R8, -1L))  // fd = -1
        let ctx9 = emit ctx8 (MOV_REG_IMM64(Register.R9, 0L))  // offset = 0
        let ctx10 = emit ctx9 (MOV_REG_IMM64(Register.RAX, 9L))  // mmap syscall number
        let ctx11 = emit ctx10 SYSCALL
        
        // RAX now contains pointer to allocated memory
        let (ctx12, arrayReg) = allocRegister ctx11
        let ctx13 = emit ctx12 (MOV_REG_REG(arrayReg, Register.RAX))
        
        // Store array length at offset 0
        let (ctx14, lengthReg) = allocRegister ctx13
        let ctx15 = emit ctx14 (MOV_REG_IMM64(lengthReg, int64 elements.Length))
        let ctx16 = emit ctx15 (MOV_MEM_REG(arrayReg, 0, lengthReg))
        
        // Compile and store each element
        let ctx17 = 
            List.fold2 (fun accCtx elem idx ->
                let (newCtx, elemReg) = compileExpression accCtx elem
                // Store element at offset 8 + (idx * 8)
                let offset = 8 + (idx * 8)
                emit newCtx (MOV_MEM_REG(arrayReg, offset, elemReg))
            ) ctx16 elements [0..elements.Length-1]
        
        // Return pointer to array
        (ctx17, arrayReg)
    
    // Compile list literal [ e1; e2; ... ]
    | ASTList elements ->
        // Similar to arrays but with different memory layout
        let (ctx2, lengthReg) = allocRegister ctx
        let ctx3 = emit ctx2 (MOV_REG_IMM64(lengthReg, int64 elements.Length))
        
        // Compile each element
        let (ctx4, elemRegs) = 
            List.fold (fun (accCtx, accRegs) elem ->
                let (newCtx, reg) = compileExpression accCtx elem
                (newCtx, reg :: accRegs)
            ) (ctx3, []) elements
        
        // For now, just return the length register
        (ctx4, lengthReg)
    
    // Compile record { field1 = e1; field2 = e2; ... }
    | ASTRecord fields ->
        // Compile each field value
        let (ctx2, fieldRegs) = 
            List.fold (fun (accCtx, accRegs) (fieldName, fieldValue) ->
                let (newCtx, reg) = compileExpression accCtx fieldValue
                (newCtx, (fieldName, reg) :: accRegs)
            ) (ctx, []) fields
        
        // For now, return a placeholder
        let (ctx3, reg) = allocRegister ctx2
        let ctx4 = emit ctx3 (MOV_REG_IMM64(reg, int64 fields.Length))
        (ctx4, reg)
    
    // Array/List indexing: obj.[index]
    | ASTIndexer(obj, index) ->
        // Compile the array/list object
        let (ctx2, objReg) = compileExpression ctx obj
        // Compile the index
        let (ctx3, indexReg) = compileExpression ctx2 index
        
        // Calculate offset: 8 bytes for length + index * 8
        let (ctx4, offsetReg) = allocRegister ctx3
        let ctx5 = emit ctx4 (MOV_REG_IMM64(offsetReg, 8L))  // Skip length field
        let (ctx6, tempReg) = allocRegister ctx5
        let ctx7 = emit ctx6 (MOV_REG_IMM64(tempReg, 8L))  // Element size
        // Multiply index by element size
        let ctx8 = emit ctx7 (MOV_REG_REG(Register.RAX, indexReg))
        let ctx9 = emit ctx8 (MUL_REG(tempReg))
        // Add to base offset
        let ctx10 = emit ctx9 (ADD_REG_REG(offsetReg, Register.RAX))
        
        // Load element from memory
        let (ctx11, resultReg) = allocRegister ctx10
        let ctx12 = emit ctx11 (MOV_REG_REG(Register.RAX, objReg))
        let ctx13 = emit ctx12 (ADD_REG_REG(Register.RAX, offsetReg))
        let ctx14 = emit ctx13 (MOV_REG_MEM(resultReg, Register.RAX, 0))
        (ctx14, resultReg)
    
    // For loop: for i = start to stop do body
    | ASTFor(var, start, stop, body) ->
        // Compile start value
        let (ctx2, startReg) = compileExpression ctx start
        // Compile stop value
        let (ctx3, stopReg) = compileExpression ctx2 stop
        
        // Allocate loop variable
        let (ctx4, loopReg) = allocRegister ctx3
        let ctx5 = emit ctx4 (MOV_REG_REG(loopReg, startReg))
        
        // Add loop variable to symbol table
        let loopCtx = { ctx5 with SymbolTable = Map.add var loopReg ctx5.SymbolTable }
        
        // Mark where the loop comparison starts (for jumping back)
        let loopStartMark = List.length loopCtx.Instructions
        
        // Check loop condition: i <= stop
        let ctx6 = emit loopCtx (CMP_REG_REG(loopReg, stopReg))
        let ctx7 = emit ctx6 (JG_REL32(999999))  // Placeholder - will patch with forward jump to exit
        let condJumpMark = List.length ctx7.Instructions
        
        // Compile loop body
        let (ctx8, _) = compileExpression ctx7 body
        
        // Increment loop variable
        let (ctx9, oneReg) = allocRegister ctx8
        let ctx10 = emit ctx9 (MOV_REG_IMM64(oneReg, 1L))
        let ctx11 = emit ctx10 (ADD_REG_REG(loopReg, oneReg))
        
        // Jump back to loop start - need to calculate actual byte offset
        let ctx12 = emit ctx11 (JMP_REL32(-999999))  // Placeholder - will patch
        let backJumpMark = List.length ctx12.Instructions
        
        // Exit point - add a nop or result
        let (ctx13, resultReg) = allocRegister ctx12
        let ctx14 = emit ctx13 (MOV_REG_IMM64(resultReg, 0L))
        
        // Now patch the jumps with correct offsets
        let finalInstructions = 
            ctx14.Instructions
            |> List.rev  // Get correct order
            |> List.mapi (fun idx instr ->
                // Calculate actual byte positions
                let pos_before = 
                    List.take idx (List.rev ctx14.Instructions)
                    |> List.sumBy getInstructionSize
                    
                if idx = condJumpMark - 1 then
                    // Forward jump to exit (skip loop body)
                    let exit_pos = 
                        List.rev ctx14.Instructions
                        |> List.sumBy getInstructionSize
                    let jump_end = pos_before + 6  // JG is 6 bytes
                    JG_REL32(int32 (exit_pos - jump_end))
                    
                elif idx = backJumpMark - 1 then
                    // Backward jump to loop start
                    let loop_start_pos = 
                        List.take (loopStartMark) (List.rev ctx14.Instructions)
                        |> List.sumBy getInstructionSize
                    let jump_end = pos_before + 5  // JMP is 5 bytes
                    JMP_REL32(int32 (loop_start_pos - jump_end))
                    
                else
                    instr
            )
        
        ({ ctx14 with Instructions = List.rev finalInstructions }, resultReg)
    
    // While loop: while condition do body
    | ASTWhile(condition, body) ->
        // Mark loop start for jumping back
        let loopStartMark = List.length ctx.Instructions
        
        // Compile condition
        let (ctx2, condReg) = compileExpression ctx condition
        
        // Check if condition is false (0)
        let (ctx3, zeroReg) = allocRegister ctx2
        let ctx4 = emit ctx3 (MOV_REG_IMM64(zeroReg, 0L))
        let ctx5 = emit ctx4 (CMP_REG_REG(condReg, zeroReg))
        let ctx6 = emit ctx5 (JE_REL32(999999))  // Placeholder - will patch
        let condJumpMark = List.length ctx6.Instructions
        
        // Compile loop body
        let (ctx7, _) = compileExpression ctx6 body
        
        // Jump back to loop start
        let ctx8 = emit ctx7 (JMP_REL32(-999999))  // Placeholder - will patch
        let backJumpMark = List.length ctx8.Instructions
        
        // Exit point
        let (ctx9, resultReg) = allocRegister ctx8
        let ctx10 = emit ctx9 (MOV_REG_IMM64(resultReg, 0L))
        
        // Patch jumps with correct offsets
        let finalInstructions = 
            ctx10.Instructions
            |> List.rev  // Get correct order
            |> List.mapi (fun idx instr ->
                let pos_before = 
                    List.take idx (List.rev ctx10.Instructions)
                    |> List.sumBy getInstructionSize
                    
                if idx = condJumpMark - 1 then
                    // Forward jump to exit if condition false
                    let exit_pos = 
                        List.rev ctx10.Instructions
                        |> List.sumBy getInstructionSize
                    let jump_end = pos_before + 6  // JE is 6 bytes
                    JE_REL32(int32 (exit_pos - jump_end))
                    
                elif idx = backJumpMark - 1 then
                    // Backward jump to loop start
                    let loop_start_pos = 
                        List.take loopStartMark (List.rev ctx10.Instructions)
                        |> List.sumBy getInstructionSize
                    let jump_end = pos_before + 5  // JMP is 5 bytes
                    JMP_REL32(int32 (loop_start_pos - jump_end))
                    
                else
                    instr
            )
        
        ({ ctx10 with Instructions = List.rev finalInstructions }, resultReg)
    
    // Default case
    | _ ->
        let (ctx2, reg) = allocRegister ctx
        let ctx3 = emit ctx2 (MOV_REG_IMM64(reg, 0L))
        (ctx3, reg)

/// Compile exception handlers
and compileExceptionHandlers (ctx: CodeGenContext) (handlers: MatchCase list) : CodeGenContext * Register =
    match handlers with
    | [] ->
        // No handlers - re-raise exception
        let ctx2 = emit ctx INT3
        let (ctx3, reg) = allocRegister ctx2
        (ctx3, reg)
    | Case(pattern, guard, handlerExpr) :: rest ->
        // For now, simple implementation - just compile the first handler
        // In a real implementation, would check exception type against pattern
        compileExpression ctx handlerExpr

/// Patch a jump instruction with the correct offset
and patchJump (ctx: CodeGenContext) (instrIndex: int) (offset: int) : CodeGenContext =
    // Simple implementation - would need to modify instruction list
    // In real implementation, would update the jump offset
    ctx



/// Compile pattern matching to conditional jumps
and compilePatternMatching (ctx: CodeGenContext) (exprReg: Register) (cases: MatchCase list) : CodeGenContext * Register =
    match cases with
    | [] ->
        // No cases - should not happen in well-formed code
        let (ctx2, reg) = allocRegister ctx
        let ctx3 = emit ctx2 (MOV_REG_IMM64(reg, 0L))
        (ctx3, reg)
        
    | [Case(pattern, guard, expr)] ->
        // Last case - just compile the expression
        match guard with
        | None -> compileExpression ctx expr
        | Some guardExpr ->
            // Test guard condition
            let (ctx2, guardReg) = compileExpression ctx guardExpr
            let (ctx3, zeroReg) = allocRegister ctx2
            let ctx4 = emit ctx3 (MOV_REG_IMM64(zeroReg, 0L))
            let ctx5 = emit ctx4 (CMP_REG_REG(guardReg, zeroReg))
            let ctx6 = emit ctx5 (JE_REL32(0))  // Jump if guard fails
            compileExpression ctx6 expr
            
    | Case(pattern, guard, expr) :: restCases ->
        // Test pattern, if match compile expression, else try next case
        let (ctx2, patternMatches) = compilePattern ctx exprReg pattern
        
        // Jump logic depends on whether pattern always matches
        if patternMatches then
            // Pattern always matches (e.g., wildcard)
            let (ctx3, resultReg) = compileExpression ctx2 expr
            (ctx3, resultReg)  // Don't compile remaining cases
        else
            // Pattern needs runtime test
            // Use JNE to skip if pattern doesn't match (after CMP in compilePattern)
            let skipSize = 15  // Estimate bytes to skip to next case
            let ctx3 = emit ctx2 (JNE_REL32(skipSize))  
            
            // Check guard if present
            let ctx4 = 
                match guard with
                | None -> ctx3
                | Some guardExpr ->
                    let (ctx_g, guardReg) = compileExpression ctx3 guardExpr
                    let (ctx_g2, zeroReg) = allocRegister ctx_g
                    let ctx_g3 = emit ctx_g2 (MOV_REG_IMM64(zeroReg, 0L))
                    let ctx_g4 = emit ctx_g3 (CMP_REG_REG(guardReg, zeroReg))
                    emit ctx_g4 (JE_REL32(10))  // Jump if guard fails
            
            // Compile this case's expression
            let (ctx5, resultReg) = compileExpression ctx4 expr
            
            // Jump to end (skip remaining cases)
            let endJumpSize = 20 * List.length restCases  // Estimate
            let ctx6 = emit ctx5 (JMP_REL32(endJumpSize))
            
            // Compile remaining cases
            let (ctx7, finalReg) = compilePatternMatching ctx6 exprReg restCases
            (ctx7, resultReg)  // Return first matching case's register

/// Compile a single pattern test - returns context and whether match succeeded (in flags)
and compilePattern (ctx: CodeGenContext) (valueReg: Register) (pattern: Pattern) : CodeGenContext * bool =
    match pattern with
    | PatWildcard ->
        // Wildcard always matches
        (ctx, true)
        
    | PatLiteral(LitInt value) ->
        // Compare integer literal
        let (ctx2, litReg) = allocRegister ctx
        let ctx3 = emit ctx2 (MOV_REG_IMM64(litReg, value))
        let ctx4 = emit ctx3 (CMP_REG_REG(valueReg, litReg))
        // Result in flags (ZF=1 if equal)
        (ctx4, false)  // Caller checks with JE
        
    | PatLiteral(LitBool true) ->
        // Compare with 1 (true)
        let (ctx2, oneReg) = allocRegister ctx
        let ctx3 = emit ctx2 (MOV_REG_IMM64(oneReg, 1L))
        let ctx4 = emit ctx3 (CMP_REG_REG(valueReg, oneReg))
        (ctx4, false)
        
    | PatLiteral(LitBool false) ->
        // Compare with 0 (false)
        let (ctx2, zeroReg) = allocRegister ctx
        let ctx3 = emit ctx2 (MOV_REG_IMM64(zeroReg, 0L))
        let ctx4 = emit ctx3 (CMP_REG_REG(valueReg, zeroReg))
        (ctx4, false)
        
    | PatVar(name) ->
        // Variable pattern always matches, binds value to name
        // In a real implementation, would add to environment
        (ctx, true)
        
    | PatAs(innerPattern, name) ->
        // As pattern: match inner pattern and also bind to name
        let (ctx2, innerMatches) = compilePattern ctx valueReg innerPattern
        // Would bind valueReg to name in environment
        (ctx2, innerMatches)
        
    | PatOr(left, right) ->
        // Or pattern: try left, if fails try right
        let (ctx2, leftMatches) = compilePattern ctx valueReg left
        if leftMatches then
            (ctx2, true)
        else
            // Compile test for left pattern, jump if matches
            let testLabel = sprintf "or_test_%d" (List.length ctx2.Instructions)
            let matchLabel = sprintf "or_match_%d" (List.length ctx2.Instructions)
            let ctx3 = emit ctx2 (JE_REL32(5))  // Jump if left matches
            
            // Try right pattern
            let (ctx4, rightMatches) = compilePattern ctx3 valueReg right
            (ctx4, rightMatches)
            
    | PatCons(head, tail) ->
        // Cons pattern: check if list is non-empty, extract head and tail
        // For simplicity, we'll just check if it's non-empty
        let (ctx2, zeroReg) = allocRegister ctx
        let ctx3 = emit ctx2 (MOV_REG_IMM64(zeroReg, 0L))
        let ctx4 = emit ctx3 (CMP_REG_REG(valueReg, zeroReg))
        (ctx4, false)  // Non-empty check
        
    | PatList [] ->
        // Empty list pattern - check if value is 0 (representing empty list)
        let (ctx2, zeroReg) = allocRegister ctx
        let ctx3 = emit ctx2 (MOV_REG_IMM64(zeroReg, 0L))
        let ctx4 = emit ctx3 (CMP_REG_REG(valueReg, zeroReg))
        (ctx4, false)
        
    | PatTuple (PatVar ctorName :: argPatterns) ->
        // Constructor pattern (represented as tuple with constructor name first)
        compileUnionPattern ctx valueReg ctorName argPatterns
        
    | PatTuple patterns ->
        // Regular tuple pattern - for now, always match
        // In real implementation, would destructure and match each component
        (ctx, true)
        
    | _ ->
        // Other patterns - simplified for now
        (ctx, true)

// ==================== MAIN COMPILATION FUNCTIONS ====================

/// Resolve labels and calculate jump offsets
let resolveLabels (ctx: CodeGenContext) : Instruction list =
    let instructions = List.rev ctx.Instructions
    
    // Calculate instruction positions
    let positions = 
        instructions
        |> List.scan (fun pos instr -> pos + getInstructionSize instr) 0
        |> List.take instructions.Length
    
    // Create position map
    let positionMap = List.zip [0..instructions.Length-1] positions |> Map.ofList
    
    // Resolve jumps
    instructions
    |> List.mapi (fun i instr ->
        match instr with
        | JE_REL32(0) | JNE_REL32(0) | JG_REL32(0) | JLE_REL32(0) | JMP_REL32(0) ->
            // Look for label at next instruction
            let targetIdx = i + 1
            if targetIdx < instructions.Length then
                let currentPos = Map.find i positionMap
                let targetPos = Map.find targetIdx positionMap
                let offset = targetPos - currentPos - getInstructionSize instr
                match instr with
                | JE_REL32(_) -> JE_REL32(offset)
                | JNE_REL32(_) -> JNE_REL32(offset)
                | JG_REL32(_) -> JG_REL32(offset)
                | JLE_REL32(_) -> JLE_REL32(offset)
                | JMP_REL32(_) -> JMP_REL32(offset)
                | _ -> instr
            else
                instr
        | _ -> instr
    )

/// Compile F# AST to x86-64 instructions
let compile (ast: FSharpAST) : Instruction list =
    let (finalCtx, _) = compileExpression emptyContext ast
    resolveLabels finalCtx

/// Compile F# AST to machine code bytes
let compileToMachineCode (ast: FSharpAST) : byte array =
    // Apply tail call optimization before code generation
    let optimizedAst = optimizeTailCalls "" ast
    
    let instructions = compile optimizedAst
    instructions
    |> List.map encodeInstruction
    |> Array.concat

/// Convert instruction to assembly text (AT&T syntax)
let instructionToAssembly (instr: Instruction) : string =
    match instr with
    | MOV_REG_IMM64(reg, value) -> sprintf "    movq $%d, %%%s" value (reg.ToString().ToLower())
    | MOV_REG_REG(dst, src) -> sprintf "    movq %%%s, %%%s" (src.ToString().ToLower()) (dst.ToString().ToLower())
    | MOV_MEM_REG(addr, offset, src) -> 
        if offset = 0 then
            sprintf "    movq %%%s, (%%%s)" (src.ToString().ToLower()) (addr.ToString().ToLower())
        else
            sprintf "    movq %%%s, %d(%%%s)" (src.ToString().ToLower()) offset (addr.ToString().ToLower())
    | MOV_REG_MEM(dst, addr, offset) ->
        if offset = 0 then
            sprintf "    movq (%%%s), %%%s" (addr.ToString().ToLower()) (dst.ToString().ToLower())
        else
            sprintf "    movq %d(%%%s), %%%s" offset (addr.ToString().ToLower()) (dst.ToString().ToLower())
    | MOV_MEM_IMM16(addr, imm) -> sprintf "    movw $%d, (%%%s)" imm (addr.ToString().ToLower())
    | ADD_REG_REG(dst, src) -> sprintf "    addq %%%s, %%%s" (src.ToString().ToLower()) (dst.ToString().ToLower())
    | SUB_REG_REG(dst, src) -> sprintf "    subq %%%s, %%%s" (src.ToString().ToLower()) (dst.ToString().ToLower())
    | MUL_REG(reg) -> sprintf "    mulq %%%s" (reg.ToString().ToLower())
    | DIV_REG(reg) -> sprintf "    divq %%%s" (reg.ToString().ToLower())
    | CMP_REG_REG(r1, r2) -> sprintf "    cmpq %%%s, %%%s" (r2.ToString().ToLower()) (r1.ToString().ToLower())
    | JE_REL32(offset) -> sprintf "    je .+%d" offset
    | JNE_REL32(offset) -> sprintf "    jne .+%d" offset
    | JG_REL32(offset) -> sprintf "    jg .+%d" offset
    | JLE_REL32(offset) -> sprintf "    jle .+%d" offset
    | JMP_REL32(offset) -> sprintf "    jmp .+%d" offset
    | SETG_REG(reg) -> sprintf "    setg %%%s" (reg.ToString().ToLower())
    | SETL_REG(reg) -> sprintf "    setl %%%s" (reg.ToString().ToLower())
    | SETE_REG(reg) -> sprintf "    sete %%%s" (reg.ToString().ToLower())
    | SETNE_REG(reg) -> sprintf "    setne %%%s" (reg.ToString().ToLower())
    | SETGE_REG(reg) -> sprintf "    setge %%%s" (reg.ToString().ToLower())
    | SETLE_REG(reg) -> sprintf "    setle %%%s" (reg.ToString().ToLower())
    | PUSH_REG(reg) -> sprintf "    pushq %%%s" (reg.ToString().ToLower())
    | POP_REG(reg) -> sprintf "    popq %%%s" (reg.ToString().ToLower())
    | CALL_REL32(offset) -> sprintf "    call .+%d" offset
    | TAIL_JMP_REL32(offset) -> sprintf "    jmp .+%d" offset
    | RET -> "    ret"
    | SYSCALL -> "    syscall"
    | MACH_MSG(timeout) -> sprintf "    ; mach_msg timeout=%d" timeout
    | MACH_TASK_SELF -> "    ; mach_task_self"
    | VM_ALLOCATE(size) -> sprintf "    ; vm_allocate size=%d" size
    | NUMA_ALLOC(size, node) -> sprintf "    ; numa_alloc size=%d node=%d" size node
    | CPU_LOCAL_ALLOC(size, cpu) -> sprintf "    ; cpu_local_alloc size=%d cpu=%d" size cpu
    | CACHE_ALIGN_ALLOC(size) -> sprintf "    ; cache_align_alloc size=%d" size
    | INT3 -> "    int3"

/// Compile F# AST to assembly text
let compileToAssembly (ast: FSharpAST) : string =
    let instructions = compile ast
    let asmLines = instructions |> List.map instructionToAssembly
    
    // Add assembly header
    let header = [
        ".section .text"
        ".global _start"
        "_start:"
    ]
    
    String.concat "\n" (header @ asmLines)

/// Compile F# source code directly to machine code
let compileStringToMachineCode (source: string) : Result<byte array, string> =
    try
        let tokens = FSharpLexer.lex source
        if FSharpLexer.isValidTokenStream tokens then
            match FSharpParser.parse tokens with
            | Ok ast ->
                if FSharpAST.isWellFormed ast then
                    Ok (compileToMachineCode ast)
                else
                    Error "AST is not well-formed"
            | Error msg -> Error ("Parse error: " + msg)
        else
            Error "Invalid token stream"
    with
    | ex -> Error ("Compilation error: " + ex.Message)

// ==================== ELF BINARY GENERATION ====================
// Minimal ELF64 header for Linux x86-64

type ELF64Header = {
    Magic: byte array        // 0x7F + "ELF"
    Class: byte             // 64-bit
    Data: byte              // Little endian  
    Version: byte           // Current version
    OSABI: byte             // Linux
    ABIVersion: byte        // 0
    Padding: byte array     // 7 bytes padding
    Type: uint16            // Executable
    Machine: uint16         // x86-64
    Version32: uint32       // Current version
    Entry: uint64           // Entry point
    PhOff: uint64           // Program header offset
    ShOff: uint64           // Section header offset
    Flags: uint32           // Processor flags
    EhSize: uint16          // ELF header size
    PhEntSize: uint16       // Program header entry size
    PhNum: uint16           // Number of program headers
    ShEntSize: uint16       // Section header entry size
    ShNum: uint16           // Number of section headers
    ShStrNdx: uint16        // String table index
}

let createELFHeader (codeSize: int) : byte array =
    let header = [|
        // ELF magic
        0x7Fuy; 0x45uy; 0x4Cuy; 0x46uy  // 0x7F + "ELF"
        0x02uy                          // 64-bit
        0x01uy                          // Little endian
        0x01uy                          // Current version
        0x00uy                          // Linux ABI
        0x00uy                          // ABI version
        0x00uy; 0x00uy; 0x00uy; 0x00uy; 0x00uy; 0x00uy; 0x00uy  // Padding
    |]
    
    let rest = [|
        0x02uy; 0x00uy                  // Executable file
        0x3Euy; 0x00uy                  // x86-64
        0x01uy; 0x00uy; 0x00uy; 0x00uy  // Version
    |]
    
    Array.concat [| header; rest |]

/// Generate complete executable ELF binary
let generateExecutable (machineCode: byte array) : byte array =
    let elfHeader = createELFHeader machineCode.Length
    let entryPoint = 0x400000UL + uint64 elfHeader.Length
    
    // Simplified: just concatenate header + code
    // Real implementation would need proper program headers, etc.
    Array.concat [| elfHeader; machineCode |]

/// Convert instructions to machine code bytes
let emitMachineCode (instructions: Instruction list) : byte array =
    instructions
    |> List.map encodeInstruction
    |> Array.concat

// ==================== DEBUGGING AND ANALYSIS ====================

let disassembleInstruction (instr: Instruction) : string =
    match instr with
    | MOV_REG_IMM64(reg, imm) -> sprintf "mov %A, 0x%X" reg imm
    | MOV_REG_REG(dst, src) -> sprintf "mov %A, %A" dst src
    | MOV_MEM_REG(addr, offset, src) -> 
        if offset = 0 then
            sprintf "mov [%A], %A" addr src
        else
            sprintf "mov [%A+%d], %A" addr offset src
    | MOV_REG_MEM(dst, addr, offset) ->
        if offset = 0 then
            sprintf "mov %A, [%A]" dst addr
        else
            sprintf "mov %A, [%A+%d]" dst addr offset
    | MOV_MEM_IMM16(addr, imm) -> sprintf "mov word [%A], 0x%X" addr imm
    | ADD_REG_REG(dst, src) -> sprintf "add %A, %A" dst src
    | SUB_REG_REG(dst, src) -> sprintf "sub %A, %A" dst src
    | CMP_REG_REG(r1, r2) -> sprintf "cmp %A, %A" r1 r2
    | JE_REL32(offset) -> sprintf "je %+d" offset
    | JNE_REL32(offset) -> sprintf "jne %+d" offset
    | SETG_REG(reg) -> sprintf "setg %A" reg
    | SETL_REG(reg) -> sprintf "setl %A" reg
    | SETE_REG(reg) -> sprintf "sete %A" reg
    | SETNE_REG(reg) -> sprintf "setne %A" reg
    | SETGE_REG(reg) -> sprintf "setge %A" reg
    | SETLE_REG(reg) -> sprintf "setle %A" reg
    | JMP_REL32(offset) -> sprintf "jmp %+d" offset
    | PUSH_REG(reg) -> sprintf "push %A" reg
    | POP_REG(reg) -> sprintf "pop %A" reg
    | CALL_REL32(offset) -> sprintf "call %+d" offset
    | TAIL_JMP_REL32(offset) -> sprintf "jmp %+d" offset
    | RET -> "ret"
    | SYSCALL -> "syscall"
    | MACH_MSG(timeout) -> sprintf "mach_msg (timeout=%d)" timeout
    | MACH_TASK_SELF -> "mach_task_self"
    | VM_ALLOCATE(size) -> sprintf "vm_allocate (size=%d)" size
    | NUMA_ALLOC(size, node) -> sprintf "numa_alloc (size=%d, node=%d)" size node
    | CPU_LOCAL_ALLOC(size, cpu) -> sprintf "cpu_local_alloc (size=%d, cpu=%d)" size cpu
    | CACHE_ALIGN_ALLOC(size) -> sprintf "cache_align_alloc (size=%d)" size
    | MUL_REG(reg) -> sprintf "mul %A" reg
    | DIV_REG(reg) -> sprintf "div %A" reg
    | INT3 -> "int3"

let disassemble (instructions: Instruction list) : string =
    instructions
    |> List.mapi (fun i instr -> sprintf "%04d: %s" i (disassembleInstruction instr))
    |> String.concat "\n"

/// Print compilation analysis
let analyzeCompilation (source: string) : unit =
    printfn "=== F# to x86-64 Compilation Analysis ==="
    printfn "Source: %s" source
    
    match compileStringToMachineCode source with
    | Ok machineCode ->
        let tokens = FSharpLexer.lex source
        let ast = FSharpParser.parse tokens |> function | Ok ast -> ast | Error _ -> ASTLiteral(LitInt 0L)
        let instructions = compile ast
        
        printfn "\n--- Generated Assembly ---"
        printfn "%s" (disassemble instructions)
        
        printfn "\n--- Machine Code (hex) ---"
        let hexBytes = machineCode |> Array.map (sprintf "%02X") |> String.concat " "
        printfn "%s" hexBytes
        
        printfn "\n--- Statistics ---"
        printfn "Instructions: %d" instructions.Length
        printfn "Machine code size: %d bytes" machineCode.Length
        printfn "Code density: %.2f bytes/instruction" (float machineCode.Length / float instructions.Length)
        
    | Error msg ->
        printfn "❌ Compilation failed: %s" msg