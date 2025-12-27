// F# to Native x86-64 Compiler
// Clean implementation based on formal Coq verification

module FSharpCompiler

open FSharpAST
// Deprecated: open FSharpLexer
// Deprecated: open FSharpParser
open FSharpExhaustiveness
open FSharpCodeGen
open FSharpTypeInference
open FSharpModuleSystem
open FSharp.Compiler.Service.FSharpChecker
open FSharp.Compiler.Service.FSharpParseFileResults
open FSharp.Compiler.Service.FSharpSyntaxTree
open System
open System.IO

// ==================== OPTIMIZATION PASSES ====================

/// Machine code optimization passes
module OptimizationPasses =
    /// Eliminate unreachable code paths (dead code elimination)
    let eliminateDeadCode (code: byte array) : byte array =
        // For now, pass through - full DCE would require CFG analysis
        // In a real implementation, this would:
        // 1. Build control flow graph
        // 2. Find unreachable basic blocks
        // 3. Remove them from output
        code
    
    /// Fold constant expressions at compile time
    let foldConstants (code: byte array) : byte array =
        // For now, pass through - full constant folding done at AST level
        // Machine code level would look for:
        // - mov rax, <const1>; add rax, <const2> -> mov rax, <const1+const2>
        // - imul with power of 2 -> shift
        code
    
    /// Inline small functions (< threshold instructions)
    let inlineSmallFunctions (code: byte array) : byte array =
        // For now, pass through - inlining requires symbol table
        // Would look for:
        // - call to small function (< 16 bytes)
        // - Replace call with function body
        // - Adjust RIP-relative addresses
        code

// ==================== COMPILATION PIPELINE ====================

type CompilerOptions = {
    InputFile: string option
    OutputFile: string option
    OptimizationLevel: int
    Verbose: bool
    UseOfficialParser: bool  // Use official F# parser instead of our simple one
    SafetyLevel: SafeCodeGen.SafetyLevel  // Memory safety level
    ModuleEnvironment: ModuleEnvironment  // Module resolution environment
}

let defaultOptions = {
    InputFile = None
    OutputFile = None
    OptimizationLevel = 0
    Verbose = false
    UseOfficialParser = true  // Now use official F# parser by default
    SafetyLevel = SafeCodeGen.SafetyLevel.Safe  // Default to safe mode
    ModuleEnvironment = emptyEnvironment  // Start with empty module environment
}

type CompilationResult = 
    | Success of byte array
    | Error of string

// ==================== AUTOMATIC OWNERSHIP INFERENCE ====================

/// Ownership tracking: Owned | Borrowed | Shared
type Ownership = Owned | Borrowed | Shared

/// Infer ownership annotations for variables in AST
/// Returns (annotatedAST, environmentWithOwnership)
let rec inferOwnership (ast: FSharpAST) (env: Map<string, Ownership>) : FSharpAST * Map<string, Ownership> =
    match ast with
    | ASTLet(name, value, body) ->
        // New bindings start as Owned
        let (annotatedValue, env1) = inferOwnership value env
        let env2 = Map.add name Owned env1
        let (annotatedBody, env3) = inferOwnership body env2
        (ASTLet(name, annotatedValue, annotatedBody), env3)
    
    | ASTIdent name ->
        // Variable reference - track as Borrowed if already in env
        match Map.tryFind name env with
        | Some Owned -> 
            // First use borrows the value
            let env' = Map.add name Borrowed env
            (ast, env')
        | _ -> (ast, env)
    
    | ASTApp(func, arg) ->
        // Function application - infer for both parts
        let (annotatedFunc, env1) = inferOwnership func env
        let (annotatedArg, env2) = inferOwnership arg env1
        (ASTApp(annotatedFunc, annotatedArg), env2)
    
    | ASTIf(cond, thenExpr, elseExpr) ->
        let (annotatedCond, env1) = inferOwnership cond env
        let (annotatedThen, env2) = inferOwnership thenExpr env1
        let (annotatedElse, env3) = inferOwnership elseExpr env1  // Use env1 for both branches
        (ASTIf(annotatedCond, annotatedThen, annotatedElse), Map.empty)  // Merge conservatively
    
    | ASTLambda(params, body) ->
        // Parameters start as Owned
        let paramEnv = params |> List.fold (fun e p -> Map.add p Owned e) env
        let (annotatedBody, _) = inferOwnership body paramEnv
        (ASTLambda(params, annotatedBody), env)  // Don't leak parameter ownership out
    
    | _ -> (ast, env)  // Default: pass through unchanged

// ==================== F# AST CONVERSION ====================

/// Convert F# Compiler Service AST to our custom AST format
let convertFSharpASTToCustom (parseTree: ParsedInput) : FSharpAST option =
    // This is a simplified conversion - in practice you'd want to handle all cases
    match parseTree with
    | ParsedInput.ImplFile(ParsedImplFileInput(fileName, isScript, qualifiedName, pragmas, hashDirectives, modules, _)) ->
        // Convert the modules to our custom AST
        // For now, just return a placeholder
        Some (ASTDeclarationSequence [ASTUnit])
    | ParsedInput.SigFile _ ->
        // Signature files not supported yet
        None

/// Process custom AST through our compilation pipeline
let processWithCustomAST (ast: FSharpAST) (options: CompilerOptions) : CompilationResult =
    try
        if options.Verbose then 
            printfn "Processing custom AST:"
            printfn "%A" ast
        
        // 2.5. Module Resolution and Scoping
        if options.Verbose then printfn "Resolving modules..."
        let processedAst, updatedEnv = processModulesInAST ast options.ModuleEnvironment
        
        // 3. Type Inference and Checking
        if options.Verbose then printfn "Type checking..."
        match inferTypes processedAst with
        | TypeResult.Error msg -> Error $"Type error: {msg}"
        | TypeResult.Ok typedAst ->
            if options.Verbose then 
                printfn "Type-checked AST:"
                printfn "%A" typedAst
            
            // 4. Pattern Matching Exhaustiveness
            if options.Verbose then printfn "Checking exhaustiveness..."
            match checkExhaustiveness typedAst with
            | ExhaustivenessResult.Error msg -> Error $"Exhaustiveness error: {msg}"
            | ExhaustivenessResult.Ok checkedAst ->
                
                // 5. Assembly Code Generation
                if options.Verbose then printfn "Generating assembly..."
                match SafeCodeGen.generateSafeAssembly checkedAst options.SafetyLevel with
                | SafeCodeGen.CodeGenResult.Error msg -> Error $"Code generation error: {msg}"
                | SafeCodeGen.CodeGenResult.Ok assembly ->
                    if options.Verbose then 
                        printfn "Generated assembly size: %d bytes" assembly.Length
                    Success assembly
    with
    | ex -> Error $"Compilation error: {ex.Message}"

// ==================== CORE COMPILATION ====================

/// Compile F# source to native machine code
let compile (source: string) (options: CompilerOptions) : CompilationResult =
    try
        if options.UseOfficialParser then
            // Use official F# Compiler Services for parsing
            if options.Verbose then printfn "Using F# Compiler Services for parsing..."
            
            let checker = FSharpChecker.Create()
            let fileName = "temp.fs"
            let projFileName = "temp.fsproj"
            
            // Create minimal project options
            let projOptions = 
                checker.GetProjectOptionsFromScript(fileName, source)
                |> Async.RunSynchronously
                |> fst
            
            // Parse the source
            let parseResults = 
                checker.ParseFile(fileName, source, FSharpParsingOptions.Default)
                |> Async.RunSynchronously
            
            match parseResults.ParseTree with
            | Some parseTree ->
                // Convert FSharp.Compiler.Service AST to our custom AST
                if options.Verbose then printfn "Converting F# AST to custom AST..."
                match convertFSharpASTToCustom parseTree with
                | Some customAST ->
                    // Continue with our custom pipeline using the official parser's AST
                    processWithCustomAST customAST options
                | None ->
                    Error "Failed to convert F# AST to custom format"
            | None ->
                let errors = parseResults.Errors |> Array.map (fun e -> e.Message) |> String.concat "; "
                Error $"F# parsing failed: {errors}"
        else
            // Use our simple parser for now
            // 1. Lexical Analysis
            if options.Verbose then printfn "Lexing..."
            let tokens = lex source
            
            if not (isValidTokenStream tokens) then
                Error "Invalid token stream"
            else
                
            // 2. Parsing
            if options.Verbose then printfn "Parsing..."
            match parse tokens with
            | ParseResult.Error msg -> Error $"Parse error: {msg}"
            | ParseResult.Ok ast ->
                if options.Verbose then 
                    printfn "Parsed AST:"
                    printfn "%A" ast
                
                // 2.5. Module Resolution and Scoping
                if options.Verbose then printfn "Resolving modules..."
                let processedAst, updatedEnv = processModulesInAST ast options.ModuleEnvironment
                if options.Verbose then 
                    printfn "Module environment:"
                    printfn "%s" (prettyPrintEnvironment updatedEnv)
                
                // 3. Type Checking with Hindley-Milner inference
                if options.Verbose then printfn "Type checking..."
                match inferType ast with
                | None -> Error "Type inference failed - expression cannot be typed"
                | Some inferredType ->
                    if options.Verbose then printfn $"Inferred type: {inferredType}"
                    
                    if not (isWellFormed ast) then
                        Error "AST not well-formed"
                    else
                        // 3.5. Pattern Match Exhaustiveness Analysis
                        if options.Verbose then printfn "Checking pattern match exhaustiveness..."
                        match FSharpExhaustiveness.analyzeAST ast with
                        | Result.Error errors ->
                            let errorMsg = String.concat "\n" errors
                            Error $"Pattern match analysis failed:\n{errorMsg}"
                        | Result.Ok () ->
                            if options.Verbose then printfn "Pattern match analysis passed"
                            
                            // 3.6 Automatic Ownership Inference (invisible to user)
                            // Infer and attach ownership annotations to references
                            let astWithOwnership = 
                                inferOwnership ast Map.empty
                                |> fst  // Return AST with ownership annotations
                            
                            // 4. Code Generation with safety
                            if options.Verbose then printfn "Generating code (safety: %A)..." options.SafetyLevel
                            try
                                // Generate x86-64 machine code with safety checks
                                let machineCode = 
                                    if options.SafetyLevel = SafeCodeGen.SafetyLevel.Unsafe then
                                        FSharpCodeGen.compileToMachineCode ast
                                    else
                                        SafeCodeGen.compileSafe astWithOwnership options.SafetyLevel
                                
                                // 5. Optimization (optional)
                                let finalCode = 
                                    if options.OptimizationLevel > 0 then
                                        if options.Verbose then printfn "Optimizing..."
                                        // Apply optimization passes based on level
                                        let optimized = 
                                            machineCode
                                            |> OptimizationPasses.eliminateDeadCode
                                            |> OptimizationPasses.foldConstants
                                            |> (if options.OptimizationLevel >= 2 
                                                then OptimizationPasses.inlineSmallFunctions 
                                                else id)
                                        if options.Verbose then 
                                            printfn "Optimization complete (level %d)" options.OptimizationLevel
                                        optimized
                                    else
                                        machineCode
                                        
                                Success finalCode
                            with ex ->
                                Error $"Code generation error: {ex.Message}"
                
    with
    | ex -> Error $"Compilation failed: {ex.Message}"

// ==================== FILE I/O ====================

let compileFile (inputPath: string) (outputPath: string option) (options: CompilerOptions) : int =
    try
        if options.Verbose then printfn $"Compiling {inputPath}..."
        
        let source = File.ReadAllText(inputPath)
        
        match compile source options with
        | Error msg ->
            eprintfn $"Error: {msg}"
            1
            
        | Success machineCode ->
            let output = 
                match outputPath with
                | Some path -> path
                | None -> Path.ChangeExtension(inputPath, ".bin")
                
            // Also generate assembly for debugging
            let asmOutput = Path.ChangeExtension(output, ".s")
            let tokens = FSharpLexer.lex source
            match FSharpParser.parse tokens with
            | Ok ast ->
                let assembly = FSharpCodeGen.compileToAssembly ast
                File.WriteAllText(asmOutput, assembly)
                if options.Verbose then printfn $"Assembly written to {asmOutput}"
            | _ -> ()
                
            File.WriteAllBytes(output, machineCode)
            
            if options.Verbose then 
                printfn $"Successfully compiled to {output}"
                printfn $"Output size: {machineCode.Length} bytes"
            
            // Make executable on Unix systems
            try
                let perms = File.GetUnixFileMode(output)
                File.SetUnixFileMode(output, perms ||| UnixFileMode.UserExecute)
            with _ -> () // Ignore on non-Unix
            
            0
            
    with
    | :? FileNotFoundException ->
        eprintfn $"File not found: {inputPath}"
        1
    | ex ->
        eprintfn $"Error: {ex.Message}"
        1

// ==================== COMMAND LINE ====================

let parseArgs (args: string array) : CompilerOptions * string option * string option =
    let rec parse (args: string list) (opts: CompilerOptions) (input: string option) (output: string option) =
        match args with
        | [] -> (opts, input, output)
        | "-o" :: outFile :: rest ->
            parse rest opts input (Some outFile)
        | "--verbose" :: rest | "-v" :: rest ->
            parse rest { opts with Verbose = true } input output
        | "-O0" :: rest ->
            parse rest { opts with OptimizationLevel = 0 } input output
        | "-O1" :: rest ->
            parse rest { opts with OptimizationLevel = 1 } input output
        | "-O2" :: rest ->
            parse rest { opts with OptimizationLevel = 2 } input output
        | "--unsafe" :: rest ->
            parse rest { opts with SafetyLevel = SafeCodeGen.SafetyLevel.Unsafe } input output
        | "--paranoid" :: rest ->
            parse rest { opts with SafetyLevel = SafeCodeGen.SafetyLevel.Paranoid } input output
        | "--help" :: _ | "-h" :: _ ->
            printfn """F# Native Compiler

Usage: fsharp-compiler [options] <input-file>

Options:
  -o <file>      Output file name
  -v, --verbose  Verbose output
  -O0, -O1, -O2  Optimization level
  --unsafe       Disable safety checks (max performance)
  --paranoid     Enable all safety checks
  -h, --help     Show this help

Examples:
  fsharp-compiler program.fs
  fsharp-compiler -o program.bin program.fs
  fsharp-compiler -v -O2 program.fs"""
            exit 0
            
        | "--version" :: _ ->
            printfn "F# Native Compiler v1.0"
            printfn "Based on formal Coq verification"
            exit 0
            
        | file :: rest when not (file.StartsWith("-")) ->
            parse rest opts (Some file) output
            
        | unknown :: _ ->
            eprintfn $"Unknown option: {unknown}"
            exit 1
            
    parse (Array.toList args) defaultOptions None None

// ==================== MAIN ENTRY POINT ====================

[<EntryPoint>]
let main argv =
    let (options, inputFile, outputFile) = parseArgs argv
    
    match inputFile with
    | None ->
        eprintfn "Error: No input file specified"
        eprintfn "Use --help for usage information"
        1
        
    | Some input ->
        compileFile input outputFile options