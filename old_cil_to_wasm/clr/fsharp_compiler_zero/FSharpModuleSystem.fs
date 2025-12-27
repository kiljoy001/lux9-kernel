// F# Module System Implementation
// Based on formal Coq proof module_system_formal.v
// Implements name resolution, scoping, and module lookup

module FSharpModuleSystem

open FSharpAST

// ==================== MODULE ENVIRONMENT ====================
// Direct translation from Coq proof

type ModuleEnvironment = {
    Modules: ModuleDeclaration list
    CurrentModule: ModuleDeclaration option
    OpenedPaths: string list list  // List of opened qualified names
    NameBindings: Map<string, ModuleDeclaration * string>  // name -> (module, qualified_path)
}

let emptyEnvironment = {
    Modules = []
    CurrentModule = None
    OpenedPaths = []
    NameBindings = Map.empty
}

// ==================== NAME RESOLUTION ====================
// Based on resolve_name function from Coq proof

/// Find a member by name in a module's members
let rec findMember (members: ModuleMember list) (name: string) : ModuleMember option =
    match members with
    | [] -> None
    | mem :: rest ->
        let memberName = 
            match mem with
            | MemberValue(n, _, _) -> n
            | MemberFunction(n, _, _, _) -> n
            | MemberType(TypeUnion(n, _, _), _) -> n
            | MemberType(TypeAlias(n, _, _), _) -> n
            | MemberType(TypeRecord(n, _, _), _) -> n
            | MemberModule(Module(n, _, _), _) -> n
        if memberName = name then 
            Some mem
        else 
            findMember rest name

/// Find a module by name in a list of modules
let rec findModuleByName (modules: ModuleDeclaration list) (name: string) : ModuleDeclaration option =
    match modules with
    | [] -> None
    | (Module(modName, members, opens) as m) :: rest ->
        if modName = name then Some m  // Return the actual module with its members
        else findModuleByName rest name

/// Find module by qualified path like ["System"; "Collections"; "Generic"]
let rec findModuleByPath (modules: ModuleDeclaration list) (path: string list) : ModuleDeclaration option =
    match path with
    | [] -> None
    | [name] -> findModuleByName modules name
    | name :: rest ->
        match findModuleByName modules name with
        | Some (Module(_, members, _)) ->
            // Extract nested modules from members
            let nestedModules = members |> List.choose (fun m -> 
                match m with 
                | MemberModule(nested, _) -> Some nested 
                | _ -> None)
            findModuleByPath nestedModules rest
        | None -> None

/// Check if member is public
let isPublicMember (mem: ModuleMember) : bool =
    match mem with
    | MemberValue(_, _, isPublic) -> isPublic
    | MemberFunction(_, _, _, isPublic) -> isPublic
    | MemberType(_, isPublic) -> isPublic
    | MemberModule(_, isPublic) -> isPublic

/// Resolve name in opened modules
let rec resolveInOpened (opened: string list list) (modules: ModuleDeclaration list) (name: string) 
    : (ModuleMember * string list) option =
    match opened with
    | [] -> None
    | path :: rest ->
        match findModuleByPath modules path with
        | Some (Module(_, members, _)) ->
            match findMember members name with
            | Some mem when isPublicMember mem -> Some (mem, path)
            | _ -> resolveInOpened rest modules name
        | None -> resolveInOpened rest modules name

/// Resolve qualified name like "A.x" or "System.Console.WriteLine"
let resolveQualifiedName (env: ModuleEnvironment) (path: string list) : (ModuleMember * string list) option =
    match path with
    | [] -> None
    | [name] -> 
        // Simple name - use standard resolution
        match env.CurrentModule with
        | None -> 
            // No current module, try global resolution
            resolveInOpened env.OpenedPaths env.Modules name
        | Some (Module(moduleName, members, _)) ->
            // First check current module
            match findMember members name with
            | Some mem -> Some (mem, [moduleName])
            | None ->
                // Then check opened modules
                resolveInOpened env.OpenedPaths env.Modules name
    | moduleName :: memberName :: _ ->
        // Qualified name like "A.x"
        match findModuleByName env.Modules moduleName with
        | Some (Module(_, members, _)) ->
            match findMember members memberName with
            | Some mem when isPublicMember mem -> Some (mem, [moduleName])
            | _ -> None
        | None -> None

/// Main name resolution function - implements resolve_name from Coq proof
let resolveName (env: ModuleEnvironment) (name: string) : (ModuleMember * string list) option =
    resolveQualifiedName env [name]

// ==================== SCOPE MANAGEMENT ====================
// Based on scope management functions from Coq proof

/// Check if a name is in scope
let nameInScope (env: ModuleEnvironment) (name: string) : bool =
    match resolveName env name with
    | Some _ -> true
    | None -> false

/// Add a module to environment
let addModule (env: ModuleEnvironment) (moduleDecl: ModuleDeclaration) : ModuleEnvironment =
    { env with Modules = moduleDecl :: env.Modules }

/// Open a module (add to opened list)
let openModule (env: ModuleEnvironment) (path: string list) : ModuleEnvironment =
    { env with OpenedPaths = path :: env.OpenedPaths }

/// Enter a module (set as current)
let enterModule (env: ModuleEnvironment) (moduleDecl: ModuleDeclaration) : ModuleEnvironment =
    let (Module(_, _, moduleOpens)) = moduleDecl
    { env with 
        CurrentModule = Some moduleDecl
        OpenedPaths = moduleOpens @ env.OpenedPaths }

// ==================== MODULE WELL-FORMEDNESS ====================
// Based on module_well_formed from Coq proof

/// Check if member names are unique in a list
let rec namesUnique (members: ModuleMember list) : bool =
    let rec getNames acc = function
        | [] -> acc
        | mem :: rest ->
            let name = 
                match mem with
                | MemberValue(n, _, _) -> n
                | MemberFunction(n, _, _, _) -> n
                | MemberType(TypeUnion(n, _, _), _) -> n
                | MemberType(TypeAlias(n, _, _), _) -> n
                | MemberType(TypeRecord(n, _, _), _) -> n
                | MemberModule(Module(n, _, _), _) -> n
            getNames (name :: acc) rest
    
    let names = getNames [] members
    names.Length = (Set.ofList names).Count

/// Check if all opened modules exist
let allOpenedExist (env: ModuleEnvironment) (opens: string list list) : bool =
    opens |> List.forall (fun path -> 
        findModuleByPath env.Modules path |> Option.isSome)

/// Check if a module is well-formed
let moduleWellFormed (env: ModuleEnvironment) (Module(_, members, opens) as moduleDecl) : bool =
    // All member names are unique
    namesUnique members &&
    // All opened modules exist  
    allOpenedExist env opens

// ==================== DEBUGGING AND UTILITIES ====================

/// Pretty print module environment for debugging
let prettyPrintEnvironment (env: ModuleEnvironment) : string =
    let moduleNames = env.Modules |> List.map (fun (Module(name, _, _)) -> name)
    let currentName = 
        match env.CurrentModule with 
        | Some (Module(name, _, _)) -> name
        | None -> "None"
    let openedPaths = env.OpenedPaths |> List.map (String.concat ".") 
    
    sprintf "Environment:\n  Modules: [%s]\n  Current: %s\n  Opened: [%s]"
        (String.concat "; " moduleNames)
        currentName
        (String.concat "; " openedPaths)

/// Create a module with validation
let createModule (env: ModuleEnvironment) (name: string) (members: ModuleMember list) (opens: string list list) 
    : Result<ModuleDeclaration, string> =
    let moduleDecl = Module(name, members, opens)
    if moduleWellFormed env moduleDecl then
        Ok moduleDecl
    else
        Error (sprintf "Module %s is not well-formed" name)

/// Batch resolve multiple names
let resolveNames (env: ModuleEnvironment) (names: string list) : Map<string, ModuleMember * string list> =
    names
    |> List.choose (fun name -> 
        match resolveName env name with
        | Some result -> Some (name, result)
        | None -> None)
    |> Map.ofList

// ==================== AST PROCESSING ====================

/// Process modules in an AST and update environment
let rec processModulesInAST (ast: FSharpAST) (env: ModuleEnvironment) : FSharpAST * ModuleEnvironment =
    match ast with
    | ASTDeclarationSequence declarations ->
        // Process each declaration in sequence, threading the environment
        let processedDecls, finalEnv = 
            List.fold (fun (accDecls, accEnv) decl ->
                let processedDecl, newEnv = processModulesInAST decl accEnv
                (processedDecl :: accDecls, newEnv)
            ) ([], env) declarations
        (ASTDeclarationSequence(List.rev processedDecls), finalEnv)
    | ASTModule moduleDecl ->
        // Add module to environment and validate
        let updatedEnv = addModule env moduleDecl
        if moduleWellFormed updatedEnv moduleDecl then
            (ast, updatedEnv)
        else
            failwith (sprintf "Module %A is not well-formed" moduleDecl)
    
    | ASTOpen pathList ->
        // Open module in environment
        let updatedEnv = openModule env pathList
        (ast, updatedEnv)
    
    | ASTNamespace(name, modules) ->
        // Namespace contains ModuleDeclaration list, not FSharpAST list
        // Process each module declaration and add to environment
        let finalEnv = 
            List.fold (fun accEnv moduleDecl ->
                let updatedEnv = addModule accEnv moduleDecl
                if moduleWellFormed updatedEnv moduleDecl then updatedEnv
                else failwith (sprintf "Module %A is not well-formed" moduleDecl)
            ) env modules
        (ast, finalEnv)
    
    | ASTLet(name, value, next) ->
        // Process let binding value and continuation
        let processedValue, envAfterValue = processModulesInAST value env
        let processedNext, finalEnv = processModulesInAST next envAfterValue
        (ASTLet(name, processedValue, processedNext), finalEnv)
    
    | ASTApp(func, arg) ->
        // Process function and argument
        let processedFunc, envAfterFunc = processModulesInAST func env
        let processedArg, finalEnv = processModulesInAST arg envAfterFunc
        (ASTApp(processedFunc, processedArg), finalEnv)
    
    | ASTIf(cond, thenBranch, elseBranch) ->
        // Process all three branches
        let processedCond, envAfterCond = processModulesInAST cond env
        let processedThen, envAfterThen = processModulesInAST thenBranch envAfterCond
        let processedElse, finalEnv = processModulesInAST elseBranch envAfterThen
        (ASTIf(processedCond, processedThen, processedElse), finalEnv)
    
    | ASTMatch(expr, cases) ->
        // Process match expression and all cases
        let processedExpr, envAfterExpr = processModulesInAST expr env
        let processedCases, finalEnv = 
            List.fold (fun (accCases, accEnv) case ->
                match case with
                | Case(pattern, guard, body) ->
                    let processedBody, newEnv = processModulesInAST body accEnv
                    let processedGuard, finalEnv = 
                        match guard with
                        | Some g -> 
                            let pg, fEnv = processModulesInAST g newEnv
                            (Some pg, fEnv)
                        | None -> (None, newEnv)
                    (Case(pattern, processedGuard, processedBody) :: accCases, finalEnv)
            ) ([], envAfterExpr) cases
        (ASTMatch(processedExpr, List.rev processedCases), finalEnv)
    
    | ASTTuple elements ->
        // Process tuple elements
        let processedElements, finalEnv = 
            List.fold (fun (accElems, accEnv) elem ->
                let processedElem, newEnv = processModulesInAST elem accEnv
                (processedElem :: accElems, newEnv)
            ) ([], env) elements
        (ASTTuple(List.rev processedElements), finalEnv)
    
    | ASTList elements ->
        // Process list elements
        let processedElements, finalEnv = 
            List.fold (fun (accElems, accEnv) elem ->
                let processedElem, newEnv = processModulesInAST elem accEnv
                (processedElem :: accElems, newEnv)
            ) ([], env) elements
        (ASTList(List.rev processedElements), finalEnv)
    
    | ASTRecord fields ->
        // Process record field values
        let processedFields, finalEnv = 
            List.fold (fun (accFields, accEnv) (name, value) ->
                let processedValue, newEnv = processModulesInAST value accEnv
                ((name, processedValue) :: accFields, newEnv)
            ) ([], env) fields
        (ASTRecord(List.rev processedFields), finalEnv)
    
    // Leaf nodes - no processing needed, return as-is
    | ASTIdent _ | ASTLiteral _ | ASTUnit | ASTLongIdent _ ->
        (ast, env)
    
    // Catch-all for any other AST nodes not explicitly handled
    | _ -> (ast, env)