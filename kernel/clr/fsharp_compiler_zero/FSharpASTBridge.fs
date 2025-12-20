// Bridge between our simplified F# AST and x86-64 code generator
// Flow: Our Parser -> FSharpAST -> Our CodeGen -> x86-64 assembly
// Future: Official F# Parser -> SynExpr -> Our CodeGen

module FSharpASTBridge

open FSharpAST
open FSharpCodeGen

/// Convert our simplified AST to x86 instructions
let rec bridgeToOfficialAST (ast: FSharpAST) : Instruction list =
    match ast with
    | ASTLiteral(LitInt value) ->
        [MOV_REG_IMM64(Register.RAX, value)]
    | ASTLiteral(LitBool true) ->
        [MOV_REG_IMM64(Register.RAX, 1L)]
    | ASTLiteral(LitBool false) ->
        [MOV_REG_IMM64(Register.RAX, 0L)]
    | ASTLiteral(LitUnit) ->
        [MOV_REG_IMM64(Register.RAX, 0L)]
    | ASTIdent(name) ->
        // Need proper variable environment - for now just placeholder
        [MOV_REG_REG(Register.RAX, Register.RBX)]  
    | ASTLet(name, value, body) ->
        let valueCode = bridgeToOfficialAST value
        let bodyCode = bridgeToOfficialAST body
        // Simple stack-based binding
        valueCode @ [PUSH_REG(Register.RAX)] @ bodyCode @ [POP_REG(Register.RBX)]
    | ASTBinaryOp(op, left, right) ->
        let leftCode = bridgeToOfficialAST left
        let rightCode = bridgeToOfficialAST right
        let opCode = 
            match op with
            | OpAdd -> [ADD_REG_REG(Register.RAX, Register.RBX)]
            | OpSubtract -> [SUB_REG_REG(Register.RAX, Register.RBX)]
            | OpMultiply -> [MUL_REG(Register.RBX)]  // Unsigned multiply: RAX * RBX -> RDX:RAX
            | OpDivide -> [DIV_REG(Register.RBX)]     // Unsigned divide: RDX:RAX / RBX -> RAX (quotient), RDX (remainder)
            | OpGreater -> 
                [CMP_REG_REG(Register.RAX, Register.RBX); SETG_REG(Register.RAX)]
            | OpLess -> 
                [CMP_REG_REG(Register.RAX, Register.RBX); SETL_REG(Register.RAX)]
            | OpEqual -> 
                [CMP_REG_REG(Register.RAX, Register.RBX); SETE_REG(Register.RAX)]
            | OpNotEqual -> 
                [CMP_REG_REG(Register.RAX, Register.RBX); SETNE_REG(Register.RAX)]
            | OpGreaterEq -> 
                [CMP_REG_REG(Register.RAX, Register.RBX); SETGE_REG(Register.RAX)]
            | OpLessEq -> 
                [CMP_REG_REG(Register.RAX, Register.RBX); SETLE_REG(Register.RAX)]
            | _ -> failwithf "Unsupported operator: %A" op
        leftCode @ [PUSH_REG(Register.RAX)] @ rightCode @ [MOV_REG_REG(Register.RBX, Register.RAX)] @ [POP_REG(Register.RAX)] @ opCode
    | ASTIf(cond, thenExpr, elseExpr) ->
        let condCode = bridgeToOfficialAST cond
        let thenCode = bridgeToOfficialAST thenExpr
        let elseCode = bridgeToOfficialAST elseExpr
        
        // Use compare and conditional jumps instead of labels
        // Compare result with 0 (false)
        condCode @
        [CMP_REG_REG(Register.RAX, Register.RAX)] @  // Sets zero flag if RAX = 0
        [JE_REL32(8)] @  // Jump 8 bytes forward if zero (skip then branch)
        thenCode @
        [JMP_REL32(4)] @  // Jump past else branch
        elseCode
    | ASTLambda(params, body) ->
        // For now, just compile the body - real impl needs closure support
        bridgeToOfficialAST body
    | ASTApp(func, arg) ->
        let funcCode = bridgeToOfficialAST func
        let argCode = bridgeToOfficialAST arg
        // Simple function call - for now use a dummy address
        funcCode @ [PUSH_REG(Register.RAX)] @ argCode @ [POP_REG(Register.RDI)] @ [CALL_REL32(0)]
    | ASTMatch(expr, cases) ->
        // Simplified: just compile first matching case
        let exprCode = bridgeToOfficialAST expr
        match cases with
        | [] -> exprCode
        | Case(_, _, resultExpr) :: _ -> 
            exprCode @ bridgeToOfficialAST resultExpr
    | ASTTuple(elements) ->
        // Compile each element and push to stack
        elements 
        |> List.collect (fun e -> bridgeToOfficialAST e @ [PUSH_REG(Register.RAX)])
        |> fun code -> code @ [MOV_REG_REG(Register.RAX, Register.RSP)]  // Return tuple pointer
    | ASTList(elements) ->
        // Compile list elements
        elements 
        |> List.collect (fun e -> bridgeToOfficialAST e @ [PUSH_REG(Register.RAX)])
        |> fun code -> code @ [MOV_REG_REG(Register.RAX, Register.RSP)]  // Return list pointer
    | ASTArray(elements) ->
        // Compile array elements
        elements 
        |> List.collect (fun e -> bridgeToOfficialAST e @ [PUSH_REG(Register.RAX)])
        |> fun code -> code @ [MOV_REG_REG(Register.RAX, Register.RSP)]  // Return array pointer
    | ASTSequence(first, second) ->
        let code1 = bridgeToOfficialAST first
        let code2 = bridgeToOfficialAST second
        code1 @ code2
    | ASTUnit ->
        [MOV_REG_IMM64(Register.RAX, 0L)]
    | _ ->
        failwithf "Unsupported AST node: %A" ast