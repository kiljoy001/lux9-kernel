// ============== DEPRECATED - DO NOT USE ==============
// This custom parser implementation is deprecated.
// Use FSharp.Compiler.Service for parsing instead.
// We should only focus on custom assembly code generation.
// =====================================================
//
// F# Parser Implementation  
// Based on spec/expressions.md and our Coq proof
// Builds AST from token stream

module FSharpParser

open FSharpAST
open FSharpLexer

// ==================== PARSER STATE ====================

type ParseResult<'T> = Result<'T, string>

type ParserState = {
    Tokens: Token list
    Position: int
    Errors: string list
}

// ==================== PARSER COMBINATORS ====================

/// Peek at current token without consuming
let peek (state: ParserState) : Token option =
    if state.Position < state.Tokens.Length then
        Some state.Tokens.[state.Position]
    else
        None

/// Advance to next token
let advance (state: ParserState) : ParserState =
    { state with Position = state.Position + 1 }

/// Skip newline tokens and return the state pointing to the next non-newline token
let rec skipNewlines (state: ParserState) : ParserState =
    match peek state with
    | Some TNewline -> 
        skipNewlines (advance state)
    | Some (TIndent _) | Some TDedent ->
        // Also skip indentation tokens if present
        skipNewlines (advance state)
    | _ -> 
        state

/// Check if current token matches expected
let expect (state: ParserState) (expected: Token) : bool =
    match peek state with
    | Some token -> token = expected
    | None -> false

/// Consume expected token or return error
let consume (state: ParserState) (expected: Token) : ParseResult<ParserState> =
    match peek state with
    | Some token when token = expected ->
        Ok (advance state)
    | Some token ->
        Error (sprintf "Expected %A but got %A at position %d" expected token state.Position)
    | None ->
        Error (sprintf "Unexpected end of input, expected %A" expected)

/// Try to consume token, return new state if successful
let tryConsume (state: ParserState) (token: Token) : ParserState option =
    if expect state token then
        Some (advance state)
    else
        None

/// Parse with error recovery
let tryParse (parser: ParserState -> ParseResult<'T * ParserState>) (state: ParserState) : ParseResult<'T * ParserState> =
    parser state

// ==================== LITERAL PARSING ====================

let parseLiteral (state: ParserState) : ParseResult<Literal * ParserState> =
    match peek state with
    | Some (TInt n) ->
        Ok (LitInt n, advance state)
    | Some (TFloat (m, e)) ->
        Ok (LitFloat (m, e), advance state)  
    | Some (TBool b) ->
        Ok (LitBool b, advance state)
    | Some (TString s) ->
        Ok (LitString s, advance state)
    | Some (TChar c) ->
        Ok (LitChar c, advance state)
    | Some TUnit ->
        Ok (LitUnit, advance state)
    | Some (TKeyword KNull) ->
        Ok (LitNull, advance state)
    | Some (TKeyword KTrue) ->
        Ok (LitBool true, advance state)
    | Some (TKeyword KFalse) ->
        Ok (LitBool false, advance state)
    | Some token ->
        Error (sprintf "Expected literal but got %A" token)
    | None ->
        Error "Unexpected end of input in literal"

// ==================== PATTERN PARSING ====================

let rec parsePattern (state: ParserState) : ParseResult<Pattern * ParserState> =
    parseOrPattern state

and parseOrPattern (state: ParserState) : ParseResult<Pattern * ParserState> =
    // Parse first pattern
    match parseConsPattern state with
    | Error msg -> Error msg
    | Ok (firstPat, state1) ->
        // Check for or pattern (|)
        match peek state1 with
        | Some TPipe ->
            // Parse: pattern1 | pattern2
            let state2 = advance state1
            match parseOrPattern state2 with
            | Error msg -> Error msg
            | Ok (rightPat, state3) ->
                Ok (PatOr (firstPat, rightPat), state3)
        | _ ->
            Ok (firstPat, state1)

and parseConsPattern (state: ParserState) : ParseResult<Pattern * ParserState> =
    // Parse first pattern
    match parseAsPattern state with
    | Error msg -> Error msg
    | Ok (firstPat, state1) ->
        // Check for cons pattern (::)
        match peek state1 with
        | Some (TOperator OpCons) ->
            // Parse: head :: tail
            let state2 = advance state1
            match parseConsPattern state2 with
            | Error msg -> Error msg
            | Ok (tailPat, state3) ->
                Ok (PatCons (firstPat, tailPat), state3)
        | _ ->
            Ok (firstPat, state1)

and parseConstructorArgs (state: ParserState) (ctorName: string) (acc: Pattern list) : ParseResult<Pattern * ParserState> =
    // Parse patterns in constructor arguments
    match parsePattern state with
    | Error msg -> Error msg
    | Ok (pat, state1) ->
        let newAcc = pat :: acc
        match peek state1 with
        | Some TComma ->
            parseConstructorArgs (advance state1) ctorName newAcc
        | Some TRParen ->
            // Create a "constructor pattern" - for now using PatTuple as placeholder
            Ok (PatTuple (PatVar ctorName :: List.rev newAcc), advance state1)
        | _ ->
            Error "Expected ',' or ')' in constructor arguments"

and parseAsPattern (state: ParserState) : ParseResult<Pattern * ParserState> =
    // Parse first pattern
    match parseBasicPattern state with
    | Error msg -> Error msg
    | Ok (pat, state1) ->
        // Check for as pattern
        match peek state1 with
        | Some (TKeyword KAs) ->
            let state2 = advance state1
            match peek state2 with
            | Some (TIdent name) ->
                Ok (PatAs (pat, name), advance state2)
            | _ ->
                Error "Expected identifier after 'as'"
        | _ ->
            Ok (pat, state1)

and parseBasicPattern (state: ParserState) : ParseResult<Pattern * ParserState> =
    let state = skipNewlines state
    match peek state with
    | Some (TIdent "_") ->
        // Wildcard pattern
        Ok (PatWildcard, advance state)
    
    | Some (TIdent name) ->
        // Check if it's a constructor pattern (starts with uppercase)
        if name.Length > 0 && System.Char.IsUpper(name.[0]) then
            // Constructor pattern: None or Some(x)
            let state1 = advance state
            match peek state1 with
            | Some TLParen ->
                // Constructor with arguments: Some(x)
                parseConstructorArgs (advance state1) name []
            | _ ->
                // Nullary constructor: None
                Ok (PatVar name, state1)  // For now, treat as variable
        else
            // Variable pattern
            Ok (PatVar name, advance state)
    
    | Some (TInt _) | Some (TFloat _) | Some (TBool _) | Some (TString _) | Some (TChar _) | Some TUnit 
    | Some (TKeyword KTrue) | Some (TKeyword KFalse) | Some (TKeyword KNull) ->
        // Literal pattern
        match parseLiteral state with
        | Ok (lit, newState) -> Ok (PatLiteral lit, newState)
        | Error msg -> Error msg
    
    | Some TLParen ->
        // Tuple pattern or parenthesized pattern
        parseTuplePattern (advance state)
    
    | Some TLBracket ->
        // List pattern
        parseListPattern (advance state)
    
    | _ ->
        Error "Expected pattern"

and parseTuplePattern (state: ParserState) : ParseResult<Pattern * ParserState> =
    // Parse first pattern (call the full pattern parser to handle all cases)
    match parseOrPattern state with
    | Error msg -> Error msg
    | Ok (firstPat, state1) ->
        // Check if it's just parenthesized or a tuple
        match peek state1 with
        | Some TComma ->
            // It's a tuple - parse remaining patterns
            parsePatternList (advance state1) [firstPat]
        | Some TRParen ->
            // Just parenthesized pattern
            Ok (firstPat, advance state1)
        | _ ->
            Error "Expected ',' or ')' in tuple pattern"

and parsePatternList (state: ParserState) (acc: Pattern list) : ParseResult<Pattern * ParserState> =
    match parseOrPattern state with
    | Error msg -> Error msg
    | Ok (pat, state1) ->
        let newAcc = pat :: acc
        match peek state1 with
        | Some TComma ->
            parsePatternList (advance state1) newAcc
        | Some TRParen ->
            Ok (PatTuple (List.rev newAcc), advance state1)
        | _ ->
            Error "Expected ',' or ')' in pattern list"

and parseListPattern (state: ParserState) : ParseResult<Pattern * ParserState> =
    // Parse patterns separated by semicolons
    let rec parseListItems state acc =
        match peek state with
        | Some TRBracket ->
            Ok (PatList (List.rev acc), advance state)
        | _ ->
            match parseOrPattern state with
            | Error msg -> Error msg
            | Ok (pat, state1) ->
                let newAcc = pat :: acc
                match peek state1 with
                | Some TSemicolon ->
                    parseListItems (advance state1) newAcc
                | Some TRBracket ->
                    Ok (PatList (List.rev newAcc), advance state1)
                | _ ->
                    Error "Expected ';' or ']' in list pattern"
    
    parseListItems state []

// ==================== TYPE PARSING ====================

/// Parse type expression (e.g., int, string list, 'a -> 'b)
let rec parseType (state: ParserState) : ParseResult<FSharpType * ParserState> =
    match peek state with
    | Some (TIdent name) when name.StartsWith("'") ->
        // Type variable: 'a
        Ok (TypeVar name, advance state)
    | Some (TIdent name) ->
        // Type constant or type application
        let state1 = advance state
        match peek state1 with
        | Some (TOperator OpLess) ->
            // Type application: List<'a>
            let state2 = advance state1
            parseTypeArgs state2 [] name
        | Some (TKeyword KList) ->
            // Special case: int list
            Ok (TypeList (TypeConst name), advance state1)
        | Some TLBracket when peek (advance state1) = Some TRBracket ->
            // Array type: int[]
            Ok (TypeArray (TypeConst name), advance (advance state1))
        | _ ->
            Ok (TypeConst name, state1)
    | Some TLParen ->
        // Tuple or function type
        parseTupleOrFunctionType (advance state)
    | _ ->
        Error "Expected type expression"

and parseTypeArgs (state: ParserState) (acc: FSharpType list) (typeName: string) : ParseResult<FSharpType * ParserState> =
    match parseType state with
    | Error msg -> Error msg
    | Ok (typ, state1) ->
        let newAcc = typ :: acc
        match peek state1 with
        | Some TComma ->
            parseTypeArgs (advance state1) newAcc typeName
        | Some (TOperator OpGreater) ->
            Ok (TypeApp (TypeConst typeName, List.rev newAcc), advance state1)
        | _ ->
            Error "Expected ',' or '>' in type arguments"

and parseTupleOrFunctionType (state: ParserState) : ParseResult<FSharpType * ParserState> =
    match parseType state with
    | Error msg -> Error msg
    | Ok (firstType, state1) ->
        match peek state1 with
        | Some (TOperator OpMultiply) ->
            // Tuple type: t1 * t2
            parseTupleTypeRest [firstType] (advance state1)
        | Some TRParen ->
            // Just parenthesized type
            Ok (firstType, advance state1)
        | Some (TOperator OpArrow) ->
            // Function type inside parens
            let state2 = advance state1
            match parseType state2 with
            | Error msg -> Error msg
            | Ok (retType, state3) ->
                match consume state3 TRParen with
                | Error msg -> Error msg
                | Ok state4 -> Ok (TypeFunction (firstType, retType), state4)
        | _ ->
            Error "Expected '*', '->' or ')' in type"

and parseTupleTypeRest (acc: FSharpType list) (state: ParserState) : ParseResult<FSharpType * ParserState> =
    match parseType state with
    | Error msg -> Error msg
    | Ok (typ, state1) ->
        let newAcc = typ :: acc
        match peek state1 with
        | Some (TOperator OpMultiply) ->
            parseTupleTypeRest newAcc (advance state1)
        | Some TRParen ->
            Ok (TypeTuple (List.rev newAcc), advance state1)
        | _ ->
            Error "Expected '*' or ')' in tuple type"

/// Parse union case: | Name | Name of type
let rec parseUnionCase (state: ParserState) : ParseResult<UnionCase * ParserState> =
    match peek state with
    | Some TPipe ->
        let state1 = advance state
        match peek state1 with
        | Some (TIdent name) ->
            let state2 = advance state1
            match peek state2 with
            | Some (TKeyword KOf) ->
                // Case with data: | Some of 'a
                let state3 = advance state2
                parseUnionCaseFields state3 name []
            | _ ->
                // Nullary case: | None
                Ok (UnionCase (name, []), state2)
        | _ ->
            Error "Expected constructor name after '|'"
    | _ ->
        Error "Expected '|' to start union case"

and parseUnionCaseFields (state: ParserState) (caseName: string) (acc: FSharpType list) : ParseResult<UnionCase * ParserState> =
    match parseType state with
    | Error msg -> Error msg
    | Ok (typ, state1) ->
        let newAcc = typ :: acc
        match peek state1 with
        | Some (TOperator OpMultiply) ->
            // More fields in the case
            parseUnionCaseFields (advance state1) caseName newAcc
        | _ ->
            // Done with this case
            Ok (UnionCase (caseName, List.rev newAcc), state1)

/// Parse type definition
let rec parseTypeDefinition (state: ParserState) : ParseResult<TypeDefinition * ParserState> =
    match consume state (TKeyword KType) with
    | Error msg -> Error msg
    | Ok state1 ->
        match peek state1 with
        | Some (TIdent typeName) ->
            let state2 = advance state1
            // Parse type parameters if present
            let (typeParams, state3) = 
                match peek state2 with
                | Some (TOperator OpLess) ->
                    // Generic type parameters
                    let rec parseTypeParams state acc =
                        match peek state with
                        | Some (TIdent name) when name.StartsWith("'") ->
                            let state1 = advance state
                            let newAcc = name :: acc
                            match peek state1 with
                            | Some TComma ->
                                parseTypeParams (advance state1) newAcc
                            | Some (TOperator OpGreater) ->
                                (List.rev newAcc, advance state1)
                            | _ ->
                                ([], state2)  // Error recovery
                        | _ ->
                            ([], state2)  // Error recovery
                    
                    let state3 = advance state2
                    parseTypeParams state3 []
                | _ ->
                    ([], state2)
            
            match consume state3 TEquals with
            | Error msg -> Error msg
            | Ok state4 ->
                // Check if it's a union, record, or type alias
                match peek state4 with
                | Some TPipe ->
                    // Discriminated union starting with pipe
                    parseUnionCases state4 [] typeName typeParams
                | Some TLBrace ->
                    // Record type
                    parseRecordType (advance state4) [] typeName typeParams
                | Some (TIdent ctorName) ->
                    // Could be a union without initial pipe: Option = None | Some of int
                    // Try to parse as union case
                    let state5 = advance state4
                    match peek state5 with
                    | Some TPipe | Some (TKeyword KOf) ->
                        // It's a union - backtrack and parse properly
                        parseUnionCasesNoPipe state4 [] typeName typeParams
                    | _ ->
                        // It's a type alias
                        match parseType state4 with
                        | Error msg -> Error msg
                        | Ok (aliasType, state6) ->
                            Ok (TypeAlias (typeName, typeParams, aliasType), state6)
                | _ ->
                    // Type alias
                    match parseType state4 with
                    | Error msg -> Error msg
                    | Ok (aliasType, state5) ->
                        Ok (TypeAlias (typeName, typeParams, aliasType), state5)
        | _ ->
            Error "Expected type name after 'type'"

and parseUnionCases (state: ParserState) (acc: UnionCase list) (typeName: string) (typeParams: string list) : ParseResult<TypeDefinition * ParserState> =
    match parseUnionCase state with
    | Error msg when List.isEmpty acc -> Error msg
    | Error _ -> 
        // Finished parsing cases
        Ok (TypeUnion (typeName, typeParams, List.rev acc), state)
    | Ok (case, state1) ->
        parseUnionCases state1 (case :: acc) typeName typeParams

and parseUnionCasesNoPipe (state: ParserState) (acc: UnionCase list) (typeName: string) (typeParams: string list) : ParseResult<TypeDefinition * ParserState> =
    // Parse first case without pipe
    match peek state with
    | Some (TIdent name) ->
        let state1 = advance state
        match peek state1 with
        | Some (TKeyword KOf) ->
            // Case with data
            let state2 = advance state1
            match parseUnionCaseFields state2 name [] with
            | Error msg -> Error msg
            | Ok (case, state3) ->
                // Now continue with regular union case parsing (with pipes)
                parseUnionCases state3 [case] typeName typeParams
        | Some TPipe ->
            // Nullary case followed by more cases
            let case = UnionCase (name, [])
            parseUnionCases state1 [case] typeName typeParams
        | _ ->
            // Just a single nullary case
            Ok (TypeUnion (typeName, typeParams, [UnionCase (name, [])]), state1)
    | _ ->
        Error "Expected constructor name"

and parseRecordType (state: ParserState) (acc: (string * FSharpType) list) (typeName: string) (typeParams: string list) : ParseResult<TypeDefinition * ParserState> =
    match peek state with
    | Some TRBrace ->
        Ok (TypeRecord (typeName, typeParams, List.rev acc), advance state)
    | Some (TIdent fieldName) ->
        let state1 = advance state
        match consume state1 TColon with
        | Error msg -> Error msg
        | Ok state2 ->
            match parseType state2 with
            | Error msg -> Error msg
            | Ok (fieldType, state3) ->
                let newAcc = (fieldName, fieldType) :: acc
                match peek state3 with
                | Some TSemicolon ->
                    parseRecordType (advance state3) newAcc typeName typeParams
                | Some TRBrace ->
                    Ok (TypeRecord (typeName, typeParams, List.rev newAcc), advance state3)
                | _ ->
                    Error "Expected ';' or '}' in record type"
    | _ ->
        Error "Expected field name or '}' in record type"

// ==================== MODULE PARSING ====================
// Based on our Coq proof module_system_formal.v

/// Parse qualified name like "System.Collections.Generic"
let rec parseQualifiedName (state: ParserState) : ParseResult<string list * ParserState> =
    match peek state with
    | Some (TIdent name) ->
        let state' = advance state
        match peek state' with
        | Some TDot ->
            let state'' = advance state'
            match parseQualifiedName state'' with
            | Ok (rest, finalState) -> Ok (name :: rest, finalState)
            | Error msg -> Error msg
        | _ -> Ok ([name], state')
    | Some token -> Error (sprintf "Expected identifier for qualified name, got %A" token)
    | None -> Error "Expected identifier for qualified name, got EOF"

/// Parse 'open System.Collections'
and parseOpen (state: ParserState) : ParseResult<FSharpAST * ParserState> =
    match peek state with
    | Some (TKeyword KOpen) ->
        let state' = advance state
        match parseQualifiedName state' with
        | Ok (path, finalState) -> Ok (ASTOpen path, finalState)
        | Error msg -> Error msg
    | _ -> Error "Expected 'open' keyword"

/// Parse module declaration
and parseModuleDeclaration (state: ParserState) : ParseResult<ModuleDeclaration * ParserState> =
    match peek state with
    | Some (TKeyword KModule) ->
        let state1 = advance state
        match peek state1 with
        | Some (TIdent moduleName) ->
            let state2 = advance state1
            match consume state2 TEquals with
            | Ok state3 ->
                // Parse module members recursively
                parseModuleMembers state3 moduleName [] []
            | Error msg -> Error msg
        | Some token -> Error (sprintf "Expected module name after 'module', got %A" token)
        | None -> Error "Expected module name after 'module', got EOF"
    | _ -> Error "Expected 'module' keyword"

and parseModuleMembers (state: ParserState) (moduleName: string) (members: ModuleMember list) (opens: string list list) : ParseResult<ModuleDeclaration * ParserState> =
    // Parse module members until we hit EOF or another top-level construct
    match peek state with
    | Some TEOF | None ->
        // End of file - return the module
        Ok (Module(moduleName, List.rev members, List.rev opens), state)
    | Some TDedent ->
        // Decreased indentation - end of module
        Ok (Module(moduleName, List.rev members, List.rev opens), state)
    | Some TNewline ->
        // Skip newlines and continue parsing
        parseModuleMembers (advance state) moduleName members opens
    | Some (TIndent _) ->
        // Skip indentation tokens
        parseModuleMembers (advance state) moduleName members opens
    | Some (TKeyword KModule) | Some (TKeyword KNamespace) ->
        // Hit another module/namespace - stop parsing this module
        Ok (Module(moduleName, List.rev members, List.rev opens), state)
    | Some (TKeyword KOpen) ->
        // Parse open statement
        let state1 = advance state
        match parseQualifiedName state1 with
        | Ok (path, state2) ->
            parseModuleMembers state2 moduleName members (path :: opens)
        | Error msg -> Error msg
    | Some (TKeyword KLet) ->
        // Parse let binding as module member
        match parseLetExpression state with
        | Ok (letExpr, state1) ->
            // Extract the binding from the let expression
            match letExpr with
            | ASTLet(name, value, _) ->
                let member' = MemberValue(name, value, true)
                parseModuleMembers state1 moduleName (member' :: members) opens
            | ASTLetRec(bindings, _) ->
                // Add all recursive bindings as members
                let newMembers = bindings |> List.map (fun (name, value) -> MemberValue(name, value, true))
                parseModuleMembers state1 moduleName (newMembers @ members) opens
            | _ ->
                // Other forms - add as expression member
                let member' = MemberValue("_", letExpr, true)
                parseModuleMembers state1 moduleName (member' :: members) opens
        | Error msg -> Error msg
    | Some (TKeyword KType) ->
        // Parse type definition as module member
        match parseTypeDefinition state with
        | Ok (typeDef, state1) ->
            let member' = MemberType(typeDef, true)
            parseModuleMembers state1 moduleName (member' :: members) opens
        | Error msg -> Error msg
    | _ ->
        // Unknown token - could be end of module
        Ok (Module(moduleName, List.rev members, List.rev opens), state)

/// Parse namespace declaration  
and parseNamespace (state: ParserState) : ParseResult<FSharpAST * ParserState> =
    match peek state with
    | Some (TKeyword KNamespace) ->
        let state1 = advance state
        match parseQualifiedName state1 with
        | Ok (namespacePath, state2) ->
            let namespaceName = String.concat "." namespacePath
            // Simplified - just return empty namespace
            Ok (ASTNamespace(namespaceName, []), state2)
        | Error msg -> Error msg
    | _ -> Error "Expected 'namespace' keyword"

// ==================== EXPRESSION PARSING ====================

and parseExpression (state: ParserState) : ParseResult<FSharpAST * ParserState> =
    // Skip whitespace tokens (newlines, indents, dedents)
    let rec skipWhitespace state =
        match peek state with
        | Some TNewline | Some (TIndent _) | Some TDedent -> skipWhitespace (advance state)
        | _ -> state
    
    let state = skipWhitespace state
    
    // Check for top-level constructs first
    match peek state with
    | Some (TKeyword KLet) ->
        parseLetExpression state
    | Some (TKeyword KNamespace) ->
        parseNamespace state
    | Some (TKeyword KModule) ->
        match parseModuleDeclaration state with
        | Ok (moduleDecl, finalState) -> Ok (ASTModule moduleDecl, finalState)
        | Error msg -> Error msg
    | Some (TKeyword KOpen) ->
        parseOpen state
    | Some (TKeyword KType) ->
        parseTypeExpression state
    | Some (TKeyword KMatch) ->
        parseMatchExpression state
    | Some (TKeyword KFor) | Some (TKeyword KWhile) ->
        parseLoopExpression state
    | Some (TKeyword KTry) ->
        parseTryExpression state
    | _ ->
        parseIfExpression state

and parseTypeExpression (state: ParserState) : ParseResult<FSharpAST * ParserState> =
    match parseTypeDefinition state with
    | Error msg -> Error msg
    | Ok (typeDef, state1) ->
        // Parse optional 'in' and body
        match peek state1 with
        | Some (TKeyword KIn) ->
            let state2 = advance state1
            match parseExpression state2 with
            | Error msg -> Error msg
            | Ok (body, state3) ->
                Ok (ASTTypeDefinition (typeDef, body), state3)
        | _ ->
            // No body, just the type definition
            Ok (ASTTypeDefinition (typeDef, ASTUnit), state1)

and parseIfExpression (state: ParserState) : ParseResult<FSharpAST * ParserState> =
    match peek state with
    | Some (TKeyword KIf) ->
        // Parse: if condition then thenExpr else elseExpr
        let state1 = advance state
        match parseExpression state1 with
        | Error msg -> Error msg
        | Ok (cond, state2) ->
            match consume state2 (TKeyword KThen) with
            | Error msg -> Error msg
            | Ok state3 ->
                match parseExpression state3 with
                | Error msg -> Error msg
                | Ok (thenExpr, state4) ->
                    match consume state4 (TKeyword KElse) with
                    | Error msg -> Error msg
                    | Ok state5 ->
                        match parseExpression state5 with
                        | Error msg -> Error msg
                        | Ok (elseExpr, state6) ->
                            Ok (ASTIf (cond, thenExpr, elseExpr), state6)
    | _ ->
        parseMatchExpression state

and parseMatchExpression (state: ParserState) : ParseResult<FSharpAST * ParserState> =
    match peek state with
    | Some (TKeyword KMatch) ->
        // Parse: match expr with | pattern -> expr | ...
        let state1 = advance state
        match parseApplicationExpression state1 with
        | Error msg -> Error msg
        | Ok (expr, state2) ->
            match consume state2 (TKeyword KWith) with
            | Error msg -> Error msg
            | Ok state3 ->
                parseMatchCases state3 [] expr
    | _ ->
        parseTryExpression state

and parseTryExpression (state: ParserState) : ParseResult<FSharpAST * ParserState> =
    match peek state with
    | Some (TKeyword KTry) ->
        // Parse: try expr with | pattern -> expr | ... finally expr
        let state1 = advance state
        match parseExpression state1 with
        | Error msg -> Error msg
        | Ok (tryExpr, state2) ->
            match peek state2 with
            | Some (TKeyword KWith) ->
                let state3 = advance state2
                // Parse exception handlers
                let rec parseHandlers state acc isFirst =
                    let state = skipNewlines state // Skip newlines at the start
                    match peek state with
                    | Some (TKeyword KFinally) | Some TEOF ->
                        Ok (List.rev acc, state)
                    | None ->
                        if List.isEmpty acc then
                            Error "Expected exception handler after 'with'"
                        else
                            Ok (List.rev acc, state)
                    | Some TPipe ->
                        let state1 = advance state
                        match parsePattern (skipNewlines state1) with
                            | Error msg -> Error msg
                            | Ok (pattern, state2) ->
                                match consume state2 (TOperator OpArrow) with
                                | Error msg -> Error msg
                                | Ok state3 ->
                                    match parseExpression state3 with
                                    | Error msg -> Error msg
                                    | Ok (handlerExpr, state4) ->
                                        let handler = Case (pattern, None, handlerExpr) in
                                        parseHandlers state4 (handler :: acc) false
                    | _ ->
                        // Pattern without | (either first case or continuation)
                        match parsePattern (skipNewlines state) with
                        | Error msg -> Error msg
                        | Ok (pattern, state1) ->
                            match consume state1 (TOperator OpArrow) with
                            | Error msg -> Error msg
                            | Ok state2 ->
                                match parseExpression state2 with
                                | Error msg -> Error msg
                                | Ok (handlerExpr, state3) ->
                                    let handler = Case (pattern, None, handlerExpr) in
                                    parseHandlers state3 (handler :: acc) false
                
                match parseHandlers state3 [] true with
                | Error msg -> Error msg
                | Ok (handlers, state4) ->
                        // Check for finally block
                        match peek state4 with
                        | Some (TKeyword KFinally) ->
                            let state5 = advance state4
                            match parseExpression state5 with
                            | Error msg -> Error msg
                            | Ok (finallyExpr, state6) ->
                                Ok (ASTTry (tryExpr, handlers, Some finallyExpr), state6)
                        | _ ->
                            Ok (ASTTry (tryExpr, handlers, None), state4)
            | _ ->
                Error "Expected 'with' after 'try' expression"
    
    | Some (TKeyword KRaise) ->
        // Parse: raise expr
        let state1 = advance state
        match parseExpression state1 with
        | Error msg -> Error msg
        | Ok (expr, state2) ->
            Ok (ASTRaise expr, state2)
    
    | _ ->
        parseLoopExpression state

and parseLoopExpression (state: ParserState) : ParseResult<FSharpAST * ParserState> =
    match peek state with
    | Some (TKeyword KFor) ->
        // Parse: for var = start to/downto stop do body
        let state1 = advance state
        match peek state1 with
        | Some (TIdent var) ->
            let state2 = advance state1
            match peek state2 with
            | Some TEquals | Some (TOperator OpEqual) ->
                let state3 = advance state2
                match parseExpression state3 with
                | Error msg -> Error msg
                | Ok (start, state4) ->
                    // Check for 'to' or 'downto'
                    match peek state4 with
                    | Some (TKeyword KTo) ->
                        let state5 = advance state4
                        match parseExpression state5 with
                        | Error msg -> Error msg
                        | Ok (stop, state6) ->
                            match consume state6 (TKeyword KDo) with
                            | Error msg -> Error msg
                            | Ok state7 ->
                                match parseExpression state7 with
                                | Error msg -> Error msg
                                | Ok (body, state8) ->
                                    Ok (ASTFor (var, start, stop, body), state8)
                    | Some (TKeyword KDownto) ->
                        // For downto, we'll invert the loop
                        let state5 = advance state4
                        match parseExpression state5 with
                        | Error msg -> Error msg
                        | Ok (stop, state6) ->
                            match consume state6 (TKeyword KDo) with
                            | Error msg -> Error msg
                            | Ok state7 ->
                                match parseExpression state7 with
                                | Error msg -> Error msg
                                | Ok (body, state8) ->
                                    // Represent downto as negative step
                                    Ok (ASTFor (var, start, stop, body), state8)
                    | _ ->
                        Error "Expected 'to' or 'downto' in for loop"
            | _ ->
                Error "Expected '=' after identifier in for loop"
        | _ ->
            Error "Expected identifier after 'for'"
    
    | Some (TKeyword KWhile) ->
        // Parse: while condition do body
        let state1 = advance state
        match parseExpression state1 with
        | Error msg -> Error msg
        | Ok (cond, state2) ->
            match consume state2 (TKeyword KDo) with
            | Error msg -> Error msg
            | Ok state3 ->
                match parseExpression state3 with
                | Error msg -> Error msg
                | Ok (body, state4) ->
                    Ok (ASTWhile (cond, body), state4)
    
    | _ ->
        parseLetExpression state

and parseMatchCases (state: ParserState) (acc: MatchCase list) (matchExpr: FSharpAST) : ParseResult<FSharpAST * ParserState> =
    // Skip newlines before checking for pattern
    let state = skipNewlines state
    match peek state with
    | Some TPipe ->
        // Parse: | pattern -> expr
        let state1 = advance state
        match parsePattern state1 with
        | Error msg -> Error msg
        | Ok (pattern, state2) ->
            // Check for optional guard clause (when ...)
            match peek state2 with
            | Some (TKeyword KWhen) ->
                // Parse guard expression
                let state2' = advance state2
                match parseExpression state2' with
                | Error msg -> Error msg
                | Ok (guardExpr, state3) ->
                    match consume state3 (TOperator OpArrow) with
                    | Error msg -> Error msg
                    | Ok state4 ->
                        match parseExpression state4 with
                        | Error msg -> Error msg
                        | Ok (caseExpr, state5) ->
                            let case = Case (pattern, Some guardExpr, caseExpr)
                            let newAcc = case :: acc
                            // Check if there are more cases (skip newlines first)
                            let state5' = skipNewlines state5
                            match peek state5' with
                            | Some TPipe ->
                                parseMatchCases state5' newAcc matchExpr
                            | _ ->
                                Ok (ASTMatch (matchExpr, List.rev newAcc), state5')
            | _ ->
                // No guard clause
                match consume state2 (TOperator OpArrow) with
                | Error msg -> Error msg
                | Ok state3 ->
                    match parseExpression state3 with
                    | Error msg -> Error msg
                    | Ok (caseExpr, state4) ->
                        let case = Case (pattern, None, caseExpr)
                        let newAcc = case :: acc
                        // Check if there are more cases (skip newlines first)
                        let state4' = skipNewlines state4
                        match peek state4' with
                        | Some TPipe ->
                            parseMatchCases state4' newAcc matchExpr
                        | _ ->
                            Ok (ASTMatch (matchExpr, List.rev newAcc), state4')
    | _ ->
        // No more cases or first case without |
        match parsePattern state with
        | Error msg -> Error msg
        | Ok (pattern, state1) ->
            // Check for optional guard clause (when ...)
            match peek state1 with
            | Some (TKeyword KWhen) ->
                // Parse guard expression
                let state1' = advance state1
                match parseExpression state1' with
                | Error msg -> Error msg
                | Ok (guardExpr, state2) ->
                    match consume state2 (TOperator OpArrow) with
                    | Error msg -> Error msg
                    | Ok state3 ->
                        match parseExpression state3 with
                        | Error msg -> Error msg
                        | Ok (caseExpr, state4) ->
                            let case = Case (pattern, Some guardExpr, caseExpr)
                            let newAcc = case :: acc
                            // Check if there are more cases (skip newlines first)
                            let state4' = skipNewlines state4
                            match peek state4' with
                            | Some TPipe ->
                                parseMatchCases state4' newAcc matchExpr
                            | _ ->
                                Ok (ASTMatch (matchExpr, List.rev newAcc), state4')
            | _ ->
                // No guard clause
                match consume state1 (TOperator OpArrow) with
                | Error msg -> Error msg
                | Ok state2 ->
                    match parseExpression state2 with
                    | Error msg -> Error msg
                    | Ok (caseExpr, state3) ->
                        let case = Case (pattern, None, caseExpr)
                        let newAcc = case :: acc
                        // Check if there are more cases (skip newlines first)
                        let state3' = skipNewlines state3
                        match peek state3' with
                        | Some TPipe ->
                            parseMatchCases state3' newAcc matchExpr
                        | _ ->
                            Ok (ASTMatch (matchExpr, List.rev newAcc), state3')

and parseLetExpression (state: ParserState) : ParseResult<FSharpAST * ParserState> =
    match peek state with
    | Some (TKeyword KLet) ->
        let state1 = advance state
        // Check for 'rec' keyword
        let (isRecursive, state2) = 
            match peek state1 with
            | Some (TKeyword KRec) -> (true, advance state1)
            | _ -> (false, state1)
        
        match peek state2 with
        | Some (TIdent name) ->
            let state3 = advance state2
            
            // Parse optional parameters: let name param1 param2 = body or let name () = body
            let rec parseParameters state acc =
                match peek state with
                | Some (TIdent paramName) ->
                    parseParameters (advance state) (paramName :: acc)
                | Some TLParen ->
                    // Handle unit parameter: ()
                    let state1 = advance state
                    match peek state1 with
                    | Some TRParen ->
                        let state2 = advance state1
                        // Unit parameter becomes empty parameter list or special marker
                        parseParameters state2 acc
                    | _ ->
                        // Could be more complex pattern in future
                        (List.rev acc, state)
                | Some TEquals | Some (TOperator OpEqual) -> 
                    (List.rev acc, state)
                | _ -> 
                    (List.rev acc, state)
            
            let (parameters, state4) = parseParameters state3 []
            
            // Expect '=' 
            match peek state4 with
            | Some TEquals | Some (TOperator OpEqual) -> 
                let state5 = advance state4
                // Skip newlines after the = sign to handle multi-line expressions
                let state5' = skipNewlines state5
                match parseExpression state5' with
                | Error msg -> Error msg
                | Ok (valueExpr, state6) ->
                    // Create lambda if there are parameters: let f x y = body becomes let f = fun x y -> body
                    let finalValue = 
                        if List.isEmpty parameters then
                            valueExpr
                        else
                            ASTLambda(parameters, valueExpr)
                    
                    // For recursive bindings, parse 'and' clauses
                    if isRecursive then
                        let rec parseAndClauses state acc =
                            match peek state with
                            | Some (TKeyword KAnd) ->
                                let state1 = advance state
                                match peek state1 with
                                | Some (TIdent otherName) ->
                                    let state2 = advance state1
                                    let (params, state3) = parseParameters state2 []
                                    match peek state3 with
                                    | Some TEquals | Some (TOperator OpEqual) ->
                                        let state4 = advance state3
                                        match parseExpression state4 with
                                        | Error msg -> Error msg
                                        | Ok (otherValue, state5) ->
                                            let otherFinalValue = 
                                                if List.isEmpty params then
                                                    otherValue
                                                else
                                                    ASTLambda(params, otherValue)
                                            parseAndClauses state5 ((otherName, otherFinalValue) :: acc)
                                    | _ -> Error "Expected '=' in recursive 'and' binding"
                                | _ -> Error "Expected identifier after 'and'"
                            | _ -> Ok (List.rev acc, state)
                        
                        match parseAndClauses state6 [(name, finalValue)] with
                        | Error msg -> Error msg
                        | Ok (bindings, state7) ->
                            // Check for 'in' keyword for let rec...in expressions
                            match peek state7 with
                            | Some (TKeyword KIn) ->
                                let state8 = advance state7
                                match parseExpression state8 with
                                | Error msg -> Error msg
                                | Ok (body, state9) ->
                                    Ok (ASTLetRec (bindings, body), state9)
                            | _ ->
                                // Top-level let rec binding without 'in'
                                Ok (ASTLetRec (bindings, ASTUnit), state7)
                    else
                        // Non-recursive let binding
                        // Check for 'in' keyword for let...in expressions
                        match peek state6 with
                        | Some (TKeyword KIn) ->
                            let state7 = advance state6
                            match parseExpression state7 with
                            | Error msg -> Error msg
                            | Ok (body, state8) ->
                                Ok (ASTLet (name, finalValue, body), state8)
                        | _ ->
                            // Top-level let binding without 'in' - return the binding itself
                            Ok (ASTLet (name, finalValue, ASTUnit), state6)
            | _ -> Error "Expected '=' after function name and parameters"
        | _ ->
            Error "Expected identifier after 'let'"
    | _ ->
        parseLambdaExpression state

and parseLambdaExpression (state: ParserState) : ParseResult<FSharpAST * ParserState> =
    match peek state with
    | Some (TKeyword KFun) ->
        // Parse: fun param1 param2 -> body
        let state1 = advance state
        
        let rec parseParams state acc =
            match peek state with
            | Some (TIdent name) ->
                parseParams (advance state) (name :: acc)
            | Some (TOperator OpArrow) ->
                (List.rev acc, advance state)
            | _ ->
                (List.rev acc, state)
        
        let (parameters, state2) = parseParams state1 []
        
        match parseExpression state2 with
        | Error msg -> Error msg
        | Ok (body, state3) ->
            Ok (ASTLambda (parameters, body), state3)
    | _ ->
        parseApplicationExpression state

and parseApplicationExpression (state: ParserState) : ParseResult<FSharpAST * ParserState> =
    // First try to parse as binary operation expression
    parseBinaryExpression state

and parseBinaryExpression (state: ParserState) : ParseResult<FSharpAST * ParserState> =
    // Parse additive expressions (+ -)
    parseAdditiveExpression state

and parseAdditiveExpression (state: ParserState) : ParseResult<FSharpAST * ParserState> =
    match parseMultiplicativeExpression state with
    | Error msg -> Error msg
    | Ok (left, state1) ->
        let rec parseRest left state =
            match peek state with
            | Some (TOperator OpAdd) ->
                let state2 = advance state
                match parseMultiplicativeExpression state2 with
                | Error msg -> Error msg
                | Ok (right, state3) ->
                    parseRest (ASTBinaryOp(OpAdd, left, right)) state3
            | Some (TOperator OpSubtract) ->
                let state2 = advance state
                match parseMultiplicativeExpression state2 with
                | Error msg -> Error msg
                | Ok (right, state3) ->
                    parseRest (ASTBinaryOp(OpSubtract, left, right)) state3
            | _ -> Ok (left, state)
        parseRest left state1

and parseMultiplicativeExpression (state: ParserState) : ParseResult<FSharpAST * ParserState> =
    match parsePrimaryExpression state with
    | Error msg -> Error msg
    | Ok (left, state1) ->
        let rec parseRest left state =
            match peek state with
            | Some (TOperator OpMultiply) ->
                let state2 = advance state
                match parsePrimaryExpression state2 with
                | Error msg -> Error msg
                | Ok (right, state3) ->
                    parseRest (ASTBinaryOp(OpMultiply, left, right)) state3
            | Some (TOperator OpDivide) ->
                let state2 = advance state
                match parsePrimaryExpression state2 with
                | Error msg -> Error msg
                | Ok (right, state3) ->
                    parseRest (ASTBinaryOp(OpDivide, left, right)) state3
            | _ -> Ok (left, state)
        parseRest left state1

and parsePrimaryExpression (state: ParserState) : ParseResult<FSharpAST * ParserState> =
    // Parse atomic expression with potential field access
    match parseAtomicExpression state with
    | Error msg -> Error msg
    | Ok (expr, state1) ->
        // Parse field access (e.field.field2...) or qualified names (A.x)
        let rec parseFieldAccess expr state accPath =
            match peek state with
            | Some TDot ->
                let state2 = advance state
                match peek state2 with
                | Some TLBracket ->
                    // Array/list indexing: e.[index]
                    let state3 = advance state2
                    match parseExpression state3 with
                    | Error msg -> Error msg
                    | Ok (indexExpr, state4) ->
                        match consume state4 TRBracket with
                        | Error msg -> Error msg
                        | Ok state5 ->
                            // Continue parsing postfix operations
                            parseFieldAccess (ASTIndexer(expr, indexExpr)) state5 []
                | Some (TIdent fieldName) ->
                    let state3 = advance state2
                    // Check if initial expr is just an identifier (potential module name)
                    match expr with
                    | ASTIdent moduleName when List.isEmpty accPath ->
                        // Could be a qualified name like A.x or A.B.x
                        parseFieldAccess expr state3 (moduleName :: fieldName :: [])
                    | _ when not (List.isEmpty accPath) ->
                        // Continue building qualified path
                        parseFieldAccess expr state3 (accPath @ [fieldName])
                    | _ ->
                        // Regular field access on expression
                        parseFieldAccess (ASTDotAccess (expr, fieldName)) state3 []
                | _ ->
                    Error "Expected field name or '[' after '.'"
            | Some TLBracket ->
                // Direct array indexing without dot: e[index]
                let state2 = advance state
                match parseExpression state2 with
                | Error msg -> Error msg
                | Ok (indexExpr, state3) ->
                    match consume state3 TRBracket with
                    | Error msg -> Error msg
                    | Ok state4 ->
                        // Continue parsing postfix operations
                        parseFieldAccess (ASTIndexer(expr, indexExpr)) state4 []
            | _ ->
                // Done parsing dots and indexers
                if not (List.isEmpty accPath) && accPath.Length >= 2 then
                    // We have a qualified name path
                    Ok (ASTLongIdent accPath, state)
                else
                    Ok (expr, state)
        
        match parseFieldAccess expr state1 [] with
        | Error msg -> Error msg
        | Ok (exprWithAccess, state2) ->
            // Now parse function applications
            let rec parseApplications func state =
                // Check if next token could start an argument (not an operator)
                match peek state with
                | Some (TOperator _) | Some TComma | Some TRParen | Some TRBracket 
                | Some TSemicolon | Some (TKeyword KIn) | Some (TKeyword KThen) 
                | Some (TKeyword KElse) | Some TDot | Some TEOF | None ->
                    // These tokens cannot start an argument, stop parsing applications
                    (func, state)
                | _ ->
                    // Try to parse an argument
                    match parseAtomicExpression state with
                    | Ok (arg, state2) ->
                        parseApplications (ASTApp (func, arg)) state2
                    | Error _ ->
                        (func, state)
            
            let (result, finalState) = parseApplications exprWithAccess state2
            Ok (result, finalState)

and parseConstructorExpArgs (state: ParserState) (ctorName: string) (acc: FSharpAST list) : ParseResult<FSharpAST * ParserState> =
    // Parse arguments in constructor call
    match parseExpression state with
    | Error msg -> Error msg
    | Ok (arg, state1) ->
        let newAcc = arg :: acc
        match peek state1 with
        | Some TComma ->
            parseConstructorExpArgs (advance state1) ctorName newAcc
        | Some TRParen ->
            // Create a constructor expression
            Ok (ASTConstructor (ctorName, List.rev newAcc), advance state1)
        | _ ->
            Error "Expected ',' or ')' in constructor arguments"

and parseAtomicExpression (state: ParserState) : ParseResult<FSharpAST * ParserState> =
    match peek state with
    | Some (TInt _) | Some (TFloat _) | Some (TBool _) | Some (TString _) | Some (TChar _)
    | Some (TKeyword KTrue) | Some (TKeyword KFalse) | Some (TKeyword KNull) ->
        // Literal expression
        match parseLiteral state with
        | Ok (lit, newState) -> Ok (ASTLiteral lit, newState)
        | Error msg -> Error msg
    
    | Some TUnit ->
        // Unit literal
        Ok (ASTUnit, advance state)
    
    | Some (TIdent name) ->
        // Check if it's a constructor (starts with uppercase)
        if name.Length > 0 && System.Char.IsUpper(name.[0]) then
            // Might be a constructor: None or Some(x)
            let state1 = advance state
            match peek state1 with
            | Some TLParen when not (match peek (advance state1) with Some TRParen -> true | _ -> false) ->
                // Constructor with arguments: Some(x)
                parseConstructorExpArgs (advance state1) name []
            | _ ->
                // Nullary constructor or just an identifier
                Ok (ASTIdent name, state1)
        else
            // Variable reference
            Ok (ASTIdent name, advance state)
    
    | Some TLParen ->
        // Parenthesized expression or tuple
        parseParenthesizedExpression (advance state)
    
    | Some TLBracket ->
        // List expression
        parseListExpression (advance state)
    
    | Some TLArrayBracket ->
        // Array expression
        parseArrayExpression (advance state)
    
    | Some TLBrace ->
        // Record expression
        parseRecordExpression (advance state)
    
    | _ ->
        Error "Expected atomic expression"

and parseParenthesizedExpression (state: ParserState) : ParseResult<FSharpAST * ParserState> =
    // Check for empty parentheses first (unit expression)
    match peek state with
    | Some TRParen ->
        // Unit expression: ()
        Ok (ASTUnit, advance state)
    | _ ->
        // Parse expression inside parentheses
        match parseExpression state with
        | Error msg -> Error msg
        | Ok (expr, state1) ->
            match peek state1 with
            | Some TComma ->
                // Tuple expression
                parseTupleExpression (advance state1) [expr]
            | Some TRParen ->
                // Just parenthesized
                Ok (expr, advance state1)
            | _ ->
                Error "Expected ',' or ')' in parenthesized expression"

and parseTupleExpression (state: ParserState) (acc: FSharpAST list) : ParseResult<FSharpAST * ParserState> =
    match parseExpression state with
    | Error msg -> Error msg
    | Ok (expr, state1) ->
        let newAcc = expr :: acc
        match peek state1 with
        | Some TComma ->
            parseTupleExpression (advance state1) newAcc
        | Some TRParen ->
            Ok (ASTTuple (List.rev newAcc), advance state1)
        | _ ->
            Error "Expected ',' or ')' in tuple expression"

and parseListExpression (state: ParserState) : ParseResult<FSharpAST * ParserState> =
    let rec parseListItems state acc =
        match peek state with
        | Some TRBracket ->
            Ok (ASTList (List.rev acc), advance state)
        | _ ->
            match parseExpression state with
            | Error msg -> Error msg
            | Ok (expr, state1) ->
                let newAcc = expr :: acc
                match peek state1 with
                | Some TSemicolon ->
                    parseListItems (advance state1) newAcc
                | Some TRBracket ->
                    Ok (ASTList (List.rev newAcc), advance state1)
                | _ ->
                    Error "Expected ';' or ']' in list expression"
    
    parseListItems state []

and parseArrayExpression (state: ParserState) : ParseResult<FSharpAST * ParserState> =
    // Parse array elements separated by semicolons: [|e1; e2; ...|]
    let rec parseArrayItems state acc =
        match peek state with
        | Some TRArrayBracket ->
            Ok (ASTArray (List.rev acc), advance state)
        | _ ->
            match parseExpression state with
            | Error msg -> Error msg
            | Ok (expr, state1) ->
                let newAcc = expr :: acc
                match peek state1 with
                | Some TSemicolon ->
                    parseArrayItems (advance state1) newAcc
                | Some TRArrayBracket ->
                    Ok (ASTArray (List.rev newAcc), advance state1)
                | _ ->
                    Error "Expected ';' or '|]' in array expression"
    
    parseArrayItems state []

and parseRecordExpression (state: ParserState) : ParseResult<FSharpAST * ParserState> =
    // Parse field = value pairs separated by semicolons
    let rec parseFields state acc =
        match peek state with
        | Some TRBrace ->
            Ok (ASTRecord (List.rev acc), advance state)
        | Some (TIdent fieldName) ->
            let state1 = advance state
            match peek state1 with
            | Some TEquals | Some (TOperator OpEqual) ->
                let state2 = advance state1
                match parseExpression state2 with
                | Error msg -> Error msg
                | Ok (value, state3) ->
                    let newAcc = (fieldName, value) :: acc
                    match peek state3 with
                    | Some TSemicolon ->
                        parseFields (advance state3) newAcc
                    | Some TRBrace ->
                        Ok (ASTRecord (List.rev newAcc), advance state3)
                    | _ ->
                        Error "Expected ';' or '}' in record expression"
            | _ ->
                Error "Expected '=' after field name in record"
        | _ ->
            if List.isEmpty acc then
                Error "Expected field name or '}' in record expression"
            else
                Error "Expected field name, ';' or '}' in record expression"
    
    parseFields state []

// ==================== MAIN PARSING FUNCTIONS ====================

/// Parse token stream into AST
/// Parse declaration sequences (multiple declarations)
let rec parseDeclarationSequence (state: ParserState) : ParseResult<FSharpAST list * ParserState> =
    let rec loop acc currentState =
        // Skip newlines between declarations
        let currentState = skipNewlines currentState
        match peek currentState with
        | Some TEOF | None -> Ok (List.rev acc, currentState)
        | _ ->
            match parseExpression currentState with
            | Ok (ast, nextState) -> loop (ast :: acc) nextState
            | Error msg -> 
                // If we have at least one declaration, return what we have
                if List.isEmpty acc then Error msg
                else Ok (List.rev acc, currentState)
    loop [] state

let parse (tokens: Token list) : ParseResult<FSharpAST> =
    let initialState = {
        Tokens = tokens
        Position = 0
        Errors = []
    }
    
    // Try parsing as declaration sequence first
    match parseDeclarationSequence initialState with
    | Ok (declarations, finalState) ->
        // Skip any trailing newlines before checking for EOF
        let finalStateAfterNewlines = skipNewlines finalState
        match peek finalStateAfterNewlines with
        | Some TEOF | None ->
            // If we have multiple declarations, wrap in ASTDeclarationSequence
            match declarations with
            | [single] -> Ok single
            | multiple -> Ok (ASTDeclarationSequence multiple)
        | Some token ->
            Error (sprintf "Unexpected tokens remaining at position %d: %A" finalStateAfterNewlines.Position token)
    | Error msg ->
        Error msg

/// Parse string input directly
let parseString (input: string) : ParseResult<FSharpAST> =
    let tokens = lex input
    if isValidTokenStream tokens then
        parse tokens
    else
        Error "Invalid token stream"


// ==================== PARSER VALIDATION ====================
// Based on our Coq proof parser_preserves_wellformedness

let validateParsedAST (ast: FSharpAST) : bool =
    isWellFormed ast

/// Parse with validation
let parseWithValidation (tokens: Token list) : ParseResult<FSharpAST> =
    match parse tokens with
    | Ok ast ->
        if validateParsedAST ast then
            Ok ast
        else
            Error "Parsed AST is not well-formed"
    | Error msg -> Error msg

// ==================== DEBUGGING UTILITIES ====================

let parseDebug (input: string) : unit =
    printfn "=== LEXING ==="
    let tokens = lex input
    printTokens tokens
    
    printfn "\n=== PARSING ==="
    match parseString input with
    | Ok ast ->
        printfn "Successfully parsed:"
        printfn "%s" (prettyPrint ast 0)
        printfn "\nAST size: %d" (astSize ast)
        printfn "Well-formed: %b" (isWellFormed ast)
    | Error msg ->
        printfn "Parse error: %s" msg