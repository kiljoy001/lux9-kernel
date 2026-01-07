// F# Type Inference 
// Wrapper module that uses the full Hindley-Milner implementation

module FSharpTypeInference

open FSharpAST
open FSharpHindleyMilner

// Re-export types from FSharpHindleyMilner for compatibility
type MonoType = FSharpHindleyMilner.MonoType
type TypeScheme = FSharpHindleyMilner.TypeScheme

// ==================== COMPATIBILITY LAYER ====================

/// Convert our MonoType to a simple representation for backward compatibility
let rec monoTypeToSimple (t: MonoType) : string =
    FSharpHindleyMilner.typeToString t

/// Map binary operators to their expected types (for simple compatibility)
let mapBinaryOperator (op: Operator) : MonoType option =
    match op with
    | OpAdd | OpSubtract | OpMultiply | OpDivide | OpModulo -> Some TyInt
    | OpEqual | OpNotEqual | OpLess | OpGreater | OpLessEq | OpGreaterEq -> Some TyBool
    | OpAnd | OpOr -> Some TyBool
    | _ -> None

// ==================== PUBLIC API ====================

/// Main type inference entry point - now uses full Hindley-Milner
let inferType (expr: FSharpAST) : MonoType option =
    match FSharpHindleyMilner.infer expr with
    | Ok t -> Some t
    | Error _ -> None

/// Type check an expression against expected type
let typeCheck (expr: FSharpAST) (expectedType: MonoType) : bool =
    FSharpHindleyMilner.check expr expectedType

/// Get a string representation of a type
let typeToString (t: MonoType) : string =
    FSharpHindleyMilner.typeToString t

/// Simple inference for backward compatibility (if needed)
let inferSimple (expr: FSharpAST) : MonoType option =
    // Falls back to simplified types for quick checks
    match expr with
    | ASTLiteral(LitInt _) -> Some TyInt
    | ASTLiteral(LitBool _) -> Some TyBool
    | ASTLiteral(LitString _) -> Some TyString
    | ASTLiteral(LitUnit) -> Some TyUnit
    | ASTUnit -> Some TyUnit
    | _ -> inferType expr  // Use full inference for complex cases