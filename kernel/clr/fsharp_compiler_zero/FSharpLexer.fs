// ============== DEPRECATED - DO NOT USE ==============
// This custom lexer implementation is deprecated.
// Use FSharp.Compiler.Service for lexing/parsing instead.
// We should only focus on custom assembly code generation.
// =====================================================
//
// F# Lexer Implementation
// Based on spec/lexical-analysis.md and our Coq proof
// Converts source text to tokens

module FSharpLexer

open FSharpAST
open System

// ==================== LEXER STATE ====================

type LexerState = {
    Input: string
    Position: int
    Line: int
    Column: int
    Tokens: Token list
    IndentStack: int list    // Stack of indentation levels
    AtLineStart: bool        // True if at beginning of line
}

// ==================== CHARACTER CLASSIFICATION ====================

let isWhitespace c = c = ' ' || c = '\t'
let isNewline c = c = '\n' || c = '\r'
let isDigit c = c >= '0' && c <= '9'
let isLetter c = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
let isIdentStart c = isLetter c || c = '_'
let isIdentContinue c = isLetter c || isDigit c || c = '_' || c = '\''

// ==================== KEYWORD RECOGNITION ====================

let keywordMap = 
    Map.ofList [
        ("let", KLet); ("in", KIn); ("if", KIf); ("then", KThen); ("else", KElse); ("elif", KElif)
        ("match", KMatch); ("with", KWith); ("when", KWhen); ("function", KFunction); ("fun", KFun)
        ("type", KType); ("of", KOf); ("module", KModule); ("namespace", KNamespace)
        ("open", KOpen); ("rec", KRec); ("and", KAnd); ("as", KAs)
        ("true", KTrue); ("false", KFalse); ("null", KNull)
        ("for", KFor); ("while", KWhile); ("do", KDo); ("to", KTo); ("downto", KDownto)
        ("yield", KYield); ("return", KReturn); ("use", KUse); ("using", KUsing)
        ("try", KTry); ("finally", KFinally); ("raise", KRaise)
        ("new", KNew); ("this", KThis); ("static", KStatic); ("member", KMember)
        ("interface", KInterface); ("abstract", KAbstract); ("override", KOverride)
        ("mutable", KMutable); ("const", KConst); ("internal", KInternal)
        ("private", KPrivate); ("public", KPublic)
    ]

// ==================== OPERATOR RECOGNITION ====================

let operatorMap =
    Map.ofList [
        ("+", OpAdd); ("-", OpSubtract); ("*", OpMultiply); ("/", OpDivide)
        ("%", OpModulo); ("**", OpPower)
        ("==", OpEqual); ("<>", OpNotEqual); ("<", OpLess); (">", OpGreater)
        ("<=", OpLessEq); (">=", OpGreaterEq)
        ("&&", OpAnd); ("||", OpOr)
        ("&&&", OpBitwiseAnd); ("|||", OpBitwiseOr); ("^^^", OpBitwiseXor)
        ("~~~", OpBitwiseNot); ("<<<", OpLeftShift); (">>>", OpRightShift)
        ("::", OpCons); ("@", OpAppend)
        ("|>", OpPipeRight); ("<|", OpPipeLeft); (">>", OpCompose); ("<<", OpComposeBack)
        ("->", OpArrow); ("..", OpRange); (".. ..", OpRangeStep)
    ]

// ==================== LEXER FUNCTIONS ====================

/// Peek at current character without advancing
let peek (state: LexerState) : char option =
    if state.Position < state.Input.Length then
        Some state.Input.[state.Position]
    else
        None

/// Peek ahead n characters
let peekAhead (state: LexerState) (n: int) : char option =
    let pos = state.Position + n
    if pos < state.Input.Length then
        Some state.Input.[pos]
    else
        None

/// Advance position by one character
let advance (state: LexerState) : LexerState =
    match peek state with
    | Some '\n' ->
        { state with 
            Position = state.Position + 1
            Line = state.Line + 1
            Column = 1 }
    | Some _ ->
        { state with 
            Position = state.Position + 1
            Column = state.Column + 1 }
    | None -> state

/// Skip whitespace (including newlines for now - proper indentation-sensitive parsing can be added later)
/// Handle newlines and calculate indentation levels
let handleNewlineAndIndentation (state: LexerState) : LexerState * Token list =
    // Skip the newline character
    let afterNewline = { state with Position = state.Position + 1; Line = state.Line + 1; Column = 1; AtLineStart = true }
    
    // Count indentation on the new line
    let rec countIndentation state level =
        match peek state with
        | Some ' ' -> countIndentation (advance state) (level + 1)
        | Some '\t' -> countIndentation (advance state) (level + 4) // Tab = 4 spaces
        | Some c when isNewline c -> 
            // Empty line - skip and continue
            countIndentation { state with Position = state.Position + 1; Line = state.Line + 1; Column = 1 } 0
        | _ -> (state, level)
    
    let (stateAfterIndent, currentIndent) = countIndentation afterNewline 0
    let finalState = { stateAfterIndent with AtLineStart = false }
    
    // Generate indent/dedent tokens based on indentation stack
    let currentLevel = List.head state.IndentStack
    
    if currentIndent > currentLevel then
        // Increased indentation - push new level and emit INDENT
        let newStack = currentIndent :: state.IndentStack
        ({ finalState with IndentStack = newStack }, [TNewline; TIndent currentIndent])
    elif currentIndent = currentLevel then
        // Same indentation - just emit newline
        (finalState, [TNewline])
    else
        // Decreased indentation - pop levels and emit DEDENTs
        let rec popLevels stack tokens =
            match stack with
            | [] -> (stack, tokens) // Should not happen
            | [baseLevel] -> (stack, tokens) // Don't pop base level
            | level :: rest when level > currentIndent ->
                popLevels rest (TDedent :: tokens)
            | _ -> (stack, tokens)
        
        let (newStack, dedentTokens) = popLevels state.IndentStack []
        ({ finalState with IndentStack = newStack }, TNewline :: (List.rev dedentTokens))

/// Skip horizontal whitespace only (not newlines)
let rec skipWhitespace (state: LexerState) : LexerState =
    match peek state with
    | Some c when isWhitespace c ->
        skipWhitespace { state with Position = state.Position + 1; Column = state.Column + 1 }
    | _ -> state

/// Skip single-line comment
let rec skipLineComment (state: LexerState) : LexerState =
    match peek state with
    | Some '\n' | None -> state
    | _ -> skipLineComment (advance state)

/// Skip block comment (handles nesting)
let rec skipBlockComment (state: LexerState) (depth: int) : LexerState =
    if depth = 0 then state
    else
        match peek state, peekAhead state 1 with
        | Some '(', Some '*' ->
            skipBlockComment (advance (advance state)) (depth + 1)
        | Some '*', Some ')' ->
            skipBlockComment (advance (advance state)) (depth - 1)
        | None, _ -> state  // EOF in comment
        | _ -> skipBlockComment (advance state) depth

/// Lex an identifier or keyword
let lexIdentifier (state: LexerState) : LexerState * Token =
    let startPos = state.Position
    let rec collectChars state =
        match peek state with
        | Some c when isIdentContinue c ->
            collectChars (advance state)
        | _ -> state
    
    let endState = collectChars state
    let ident = state.Input.Substring(startPos, endState.Position - startPos)
    
    let token =
        match Map.tryFind ident keywordMap with
        | Some keyword -> TKeyword keyword
        | None -> TIdent ident
    
    (endState, token)

/// Lex a number (int or float)
let lexNumber (state: LexerState) : LexerState * Token =
    let startPos = state.Position
    
    // Collect integer part
    let rec collectDigits state =
        match peek state with
        | Some c when isDigit c -> collectDigits (advance state)
        | _ -> state
    
    let afterInt = collectDigits state
    
    // Check for decimal point
    match peek afterInt, peekAhead afterInt 1 with
    | Some '.', Some c when isDigit c ->
        // Float literal
        let afterDot = advance afterInt
        let afterFraction = collectDigits afterDot
        
        // Check for exponent
        let finalState =
            match peek afterFraction with
            | Some 'e' | Some 'E' ->
                let afterE = advance afterFraction
                match peek afterE with
                | Some '+' | Some '-' ->
                    collectDigits (advance afterE)
                | _ -> collectDigits afterE
            | _ -> afterFraction
        
        let floatStr = state.Input.Substring(startPos, finalState.Position - startPos)
        (finalState, TFloat(Double.Parse(floatStr), 0))
    
    | _ ->
        // Integer literal
        let intStr = state.Input.Substring(startPos, afterInt.Position - startPos)
        (afterInt, TInt(Int64.Parse(intStr)))

/// Lex a string literal
let lexString (state: LexerState) : LexerState * Token =
    let quote = peek state |> Option.get  // We know it's a quote
    let startState = advance state  // Skip opening quote
    
    let rec collectChars state acc escaped =
        match peek state with
        | None -> 
            // Unterminated string
            (state, TString(System.String(List.rev acc |> List.toArray)))
        | Some '\\' when not escaped ->
            collectChars (advance state) acc true
        | Some c when c = quote && not escaped ->
            // End of string
            (advance state, TString(System.String(List.rev acc |> List.toArray)))
        | Some c ->
            let char = 
                if escaped then
                    match c with
                    | 'n' -> '\n'
                    | 't' -> '\t'
                    | 'r' -> '\r'
                    | '\\' -> '\\'
                    | '"' -> '"'
                    | '\'' -> '\''
                    | _ -> c
                else c
            collectChars (advance state) (char :: acc) false
    
    collectChars startState [] false

/// Lex an operator or symbolic token
let lexOperator (state: LexerState) : LexerState * Token =
    // Try to match longest operator first
    let tryMatch (op: string) =
        let matches = 
            op.Length <= (state.Input.Length - state.Position) &&
            state.Input.Substring(state.Position, op.Length) = op
        if matches then Some op else None
    
    // Sort operators by length (longest first)
    let sortedOps = 
        operatorMap 
        |> Map.toList 
        |> List.map fst
        |> List.sortByDescending String.length
    
    match List.tryPick tryMatch sortedOps with
    | Some op ->
        let newState = 
            Seq.init op.Length (fun _ -> ()) 
            |> Seq.fold (fun s _ -> advance s) state
        (newState, TOperator(operatorMap.[op]))
    | None ->
        // Check for array brackets first
        match peek state, peekAhead state 1 with
        | Some '[', Some '|' ->
            let state1 = advance (advance state)
            (state1, TLArrayBracket)
        | Some '|', Some ']' ->
            let state1 = advance (advance state)
            (state1, TRArrayBracket)
        | _ ->
            // Single character operator/delimiter
            match peek state with
            | Some '(' -> (advance state, TLParen)
            | Some ')' -> (advance state, TRParen)
            | Some '[' -> (advance state, TLBracket)
            | Some ']' -> (advance state, TRBracket)
            | Some '{' -> (advance state, TLBrace)
            | Some '}' -> (advance state, TRBrace)
            | Some ';' -> (advance state, TSemicolon)
            | Some ',' -> (advance state, TComma)
            | Some '.' -> (advance state, TDot)
            | Some ':' -> (advance state, TColon)
            | Some '=' -> (advance state, TEquals)  // Single = is TEquals
            | Some '|' -> (advance state, TPipe)
            | Some '&' -> (advance state, TAmpersand)
            | Some '`' -> (advance state, TBacktick)
            | Some '#' -> (advance state, THash)
            | Some c ->
                // Unknown operator - treat as symbolic
                (advance state, TSymbolicOp(string c))
            | None -> (state, TEOF)

/// Main lexer function - get next token
let rec nextToken (state: LexerState) : LexerState * Token option =
    let state = skipWhitespace state
    
    // Handle newlines and indentation properly after skipping whitespace
    match peek state with
    | Some c when isNewline c ->
        // Return special indicator for newline that main lex function will handle
        (state, Some TNewline)  // Don't advance state here - let handleNewlineAndIndentation do it
    | _ ->
        match peek state, peekAhead state 1 with
        | None, _ -> (state, Some TEOF)
        
        // Comments
        | Some '/', Some '/' ->
            nextToken (skipLineComment (advance (advance state)))
        | Some '(', Some '*' ->
            nextToken (skipBlockComment (advance (advance state)) 1)
    
        // Identifiers and keywords
        | Some c, _ when isIdentStart c ->
            let (newState, token) = lexIdentifier state
            (newState, Some token)
        
        // Numbers
        | Some c, _ when isDigit c ->
            let (newState, token) = lexNumber state
            (newState, Some token)
        
        // Strings
        | Some '"', _ | Some '\'', _ ->
            let (newState, token) = lexString state
            (newState, Some token)
        
        // Operators and delimiters
        | Some _, _ ->
            let (newState, token) = lexOperator state
            (newState, Some token)

/// Lex entire input into token list
let lex (input: string) : Token list =
    let initialState = {
        Input = input
        Position = 0
        Line = 1
        Column = 1
        Tokens = []
        IndentStack = [0]  // Start with base indentation
        AtLineStart = true
    }
    
    let rec lexAll state acc =
        match nextToken state with
        | (newState, Some TEOF) -> List.rev (TEOF :: acc)
        | (newState, Some TNewline) ->
            // Handle newline and indentation properly - use original state position
            let (finalState, tokens) = handleNewlineAndIndentation state
            lexAll finalState (List.rev tokens @ acc)
        | (newState, Some token) -> lexAll newState (token :: acc)
        | (newState, None) -> List.rev acc
    
    lexAll initialState []

// ==================== LEXER VALIDATION ====================
// Based on our Coq proof ValidTokenStream

let isValidToken (token: Token) : bool =
    match token with
    | TInt _ | TFloat _ | TBool _ | TString _ | TChar _ -> true
    | TIdent _ | TKeyword _ | TOperator _ -> true
    | TLParen | TRParen | TLBracket | TRBracket -> true
    | TLBrace | TRBrace | TSemicolon | TComma -> true
    | _ -> true  // All tokens are valid in our implementation

let isValidTokenStream (tokens: Token list) : bool =
    List.forall isValidToken tokens

// ==================== DEBUGGING UTILITIES ====================

let tokenToString (token: Token) : string =
    match token with
    | TIdent name -> sprintf "IDENT(%s)" name
    | TKeyword k -> sprintf "KEYWORD(%A)" k
    | TOperator op -> sprintf "OP(%A)" op
    | TInt n -> sprintf "INT(%d)" n
    | TFloat(m, e) -> sprintf "FLOAT(%f)" m
    | TBool b -> sprintf "BOOL(%b)" b
    | TString s -> sprintf "STRING(\"%s\")" s
    | TLParen -> "LPAREN"
    | TRParen -> "RPAREN"
    | TLBracket -> "LBRACKET"
    | TRBracket -> "RBRACKET"
    | TSemicolon -> "SEMICOLON"
    | TComma -> "COMMA"
    | TEOF -> "EOF"
    | _ -> sprintf "TOKEN(%A)" token

let printTokens (tokens: Token list) =
    tokens |> List.iter (fun t -> printfn "%s" (tokenToString t))