#r "nuget: Microsoft.Extensions.Logging, 8.0.0"
#r "nuget: Microsoft.Extensions.Logging.Console, 8.0.0"

#load "../AutoProver/AutoProver.Learning/CompilationFeedbackLoop.fs"

open System
open System.IO
open System.Diagnostics
open AutoProver.Learning
open Microsoft.Extensions.Logging

printfn "\n======================================================="
printfn "     FIXING VM-INTEROP PROOFS WITH AUTOPROVER"
printfn "======================================================="

// Setup
let loggerFactory = LoggerFactory.Create(fun builder -> 
    builder.SetMinimumLevel(LogLevel.Information).AddConsole() |> ignore)
let logger = loggerFactory.CreateLogger<CompilationFeedbackLoop>()
let feedbackLoop = CompilationFeedbackLoop(logger)

// Load previous learning
feedbackLoop.LoadLearning("/home/scott/Repo/AutoProver/compilation_errors.json")

// Function to extract admits from a file
let extractAdmits (filePath: string) =
    let content = File.ReadAllText(filePath)
    let lines = content.Split('\n')
    lines 
    |> Array.mapi (fun i line -> (i + 1, line))
    |> Array.filter (fun (_, line) -> line.Contains("admit") || line.Contains("Admitted"))
    |> Array.map (fun (lineNum, line) -> sprintf "Line %d: %s" lineNum (line.Trim()))
    |> Array.toList

// Function to try to prove a theorem using common tactics
let generateProofAttempt (theoremName: string) (theoremStatement: string) =
    let tactics = [
        "reflexivity"
        "auto"
        "intuition"
        "lia"
        "nia"
        "simpl; auto"
        "unfold *; auto"
        "intros; simpl; auto"
        "intros; unfold *; simpl; auto"
        "intros; destruct *; auto"
        "intros; induction *; auto"
        "intros; apply *; auto"
    ]
    
    tactics 
    |> List.map (fun tactic -> 
        sprintf "Proof.\n  %s.\nQed." tactic)

// Process ring_buffer_proof.v
printfn "\n📝 Processing ring_buffer_proof.v..."
let ringBufferPath = "/home/scott/Repo/VM-Interop/ring_buffer_proof.v"
let ringBufferAdmits = extractAdmits ringBufferPath

printfn "Found %d admits/axioms:" ringBufferAdmits.Length
ringBufferAdmits |> List.iter (printfn "  • %s")

// Try to fix compilation errors
printfn "\n🔧 Running compilation feedback loop on ring_buffer_proof.v..."
let (rbSuccess, rbAttempts, rbFixes) = feedbackLoop.RunFeedbackLoop(ringBufferPath, 3)

if rbSuccess then
    printfn "✅ Fixed compilation issues after %d attempts" rbAttempts
else
    printfn "⚠️ Some issues remain after %d attempts" rbAttempts

// Process ninep_synthetic_proof.v
printfn "\n📝 Processing ninep_synthetic_proof.v..."
let ninepPath = "/home/scott/Repo/VM-Interop/ninep_synthetic_proof.v"
let ninepAdmits = extractAdmits ninepPath

printfn "Found %d admits:" ninepAdmits.Length
ninepAdmits |> List.iter (printfn "  • %s")

// Try to fix compilation errors
printfn "\n🔧 Running compilation feedback loop on ninep_synthetic_proof.v..."
let (npSuccess, npAttempts, npFixes) = feedbackLoop.RunFeedbackLoop(ninepPath, 3)

if npSuccess then
    printfn "✅ Fixed compilation issues after %d attempts" npAttempts
else
    printfn "⚠️ Some issues remain after %d attempts" npAttempts

// Generate a summary report
printfn "\n======================================================="
printfn "                    SUMMARY REPORT"
printfn "======================================================="
printfn "ring_buffer_proof.v:"
printfn "  - Admits/Axioms: %d" ringBufferAdmits.Length
printfn "  - Compilation: %s" (if rbSuccess then "SUCCESS" else "NEEDS WORK")
printfn ""
printfn "ninep_synthetic_proof.v:"
printfn "  - Admits: %d" ninepAdmits.Length
printfn "  - Compilation: %s" (if npSuccess then "SUCCESS" else "NEEDS WORK")

// Save learning for future use
feedbackLoop.SaveLearning("/home/scott/Repo/VM-Interop/vm_interop_learning.json")
printfn "\n💾 Saved learning data to vm_interop_learning.json"

printfn "\n🎯 Next steps:"
printfn "  1. Review the fixed files"
printfn "  2. Manually complete remaining admits"
printfn "  3. Run formal verification"