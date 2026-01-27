#!/usr/bin/env -S dotnet fsi

#r "nuget: Microsoft.Extensions.Logging, 8.0.0"
#r "nuget: Microsoft.Extensions.Logging.Console, 8.0.0"

open System
open System.IO
open System.Diagnostics
open System.Text.RegularExpressions
open Microsoft.Extensions.Logging

// Setup logging
let loggerFactory = LoggerFactory.Create(fun builder -> 
    builder.SetMinimumLevel(LogLevel.Information).AddConsole() |> ignore)
let logger = loggerFactory.CreateLogger("VMProofCompleter")

printfn "\n======================================================="
printfn "     VM-INTEROP PROOF COMPLETION WITH AUTOPROVER"
printfn "======================================================="

// Helper to run Coq and capture output
let runCoq (content: string) =
    let tempFile = Path.GetTempFileName() + ".v"
    File.WriteAllText(tempFile, content)
    try
        let psi = ProcessStartInfo("coqc", tempFile)
        psi.RedirectStandardOutput <- true
        psi.RedirectStandardError <- true
        psi.UseShellExecute <- false
        use proc = Process.Start(psi)
        let output = proc.StandardOutput.ReadToEnd()
        let error = proc.StandardError.ReadToEnd()
        proc.WaitForExit()
        (proc.ExitCode = 0, output + error)
    finally
        File.Delete(tempFile)

// Proof tactics to try
let basicTactics = [
    "reflexivity"
    "auto"
    "intuition" 
    "lia"
    "simpl; auto"
    "unfold *; auto"
    "destruct *; auto"
]

let ringBufferTactics = [
    "unfold ring_invariant, valid_indices, data_consistency in *; auto"
    "unfold is_empty, is_full in *; simpl in *; auto"
    "unfold wrap, WRAPSIZE, RINGSIZE in *; lia"
    "destruct (lock r); simpl; auto"
    "destruct (is_empty r); simpl; auto"
    "destruct (is_full r); simpl; auto"
    "apply wrap_bound"
    "unfold enqueue, dequeue, try_lock, unlock in *; auto"
]

let ninepTactics = [
    "unfold ring_empty, ring_full, ring_count in *; auto"
    "unfold acquire_lock, release_lock in *; simpl; auto"
    "destruct (lock_held r); simpl; auto"
    "destruct (buffer_data r); simpl; auto"
    "rewrite length_app; simpl; lia"
    "unfold handle_message in *; simpl; auto"
]

// Extract admit locations from a file
let extractAdmitContexts (filePath: string) =
    let content = File.ReadAllText(filePath)
    let lines = content.Split('\n')
    
    // Find admits with context
    let mutable admits = []
    for i in 0 .. lines.Length - 1 do
        if lines.[i].Contains("admit") && not (lines.[i].Contains("Admitted")) then
            // Get surrounding context
            let contextStart = max 0 (i - 10)
            let contextEnd = min (lines.Length - 1) (i + 5)
            let context = lines.[contextStart..contextEnd] |> String.concat "\n"
            
            // Try to identify the theorem/lemma name
            let mutable theoremName = "unknown"
            let mutable found = false
            for j in (i - 1) .. -1 .. max 0 (i - 20) do
                if not found && (lines.[j].Contains("Theorem") || lines.[j].Contains("Lemma")) then
                    let match' = Regex.Match(lines.[j], @"(Theorem|Lemma)\s+(\w+)")
                    if match'.Success then
                        theoremName <- match'.Groups.[2].Value
                        found <- true
            
            admits <- (theoremName, i + 1, context) :: admits
    
    List.rev admits

// Try to complete a specific admit
let tryCompleteAdmit (theoremName: string) (context: string) (tactics: string list) =
    printfn "  Trying to complete %s..." theoremName
    
    let mutable result = None
    for tactic in tactics do
        if result.IsNone then
            // Extract the goal from context and try the tactic
            let testProof = context + "\n  " + tactic + "."
            let (success, output) = runCoq testProof
            if success then
                printfn "    ✅ Success with: %s" tactic
                result <- Some tactic
    
    if result.IsNone then
        printfn "    ❌ Could not complete automatically"
    result

// Process ring_buffer_proof.v
printfn "\n📝 Processing ring_buffer_proof.v..."
let ringBufferAdmits = extractAdmitContexts "/home/scott/Repo/VM-Interop/ring_buffer_proof.v"

printfn "Found %d admits to complete:" ringBufferAdmits.Length
for (name, line, _) in ringBufferAdmits do
    printfn "  • %s (line %d)" name line

printfn "\n🔧 Attempting to complete admits..."
let mutable ringBufferCompletions = []
for (name, line, context) in ringBufferAdmits do
    match tryCompleteAdmit name context (basicTactics @ ringBufferTactics) with
    | Some tactic -> 
        ringBufferCompletions <- (name, line, tactic) :: ringBufferCompletions
    | None -> ()

// Process ninep_synthetic_proof.v
printfn "\n📝 Processing ninep_synthetic_proof.v..."
let ninepAdmits = extractAdmitContexts "/home/scott/Repo/VM-Interop/ninep_synthetic_proof.v"

printfn "Found %d admits to complete:" ninepAdmits.Length
for (name, line, _) in ninepAdmits do
    printfn "  • %s (line %d)" name line

printfn "\n🔧 Attempting to complete admits..."
let mutable ninepCompletions = []
for (name, line, context) in ninepAdmits do
    match tryCompleteAdmit name context (basicTactics @ ninepTactics) with
    | Some tactic ->
        ninepCompletions <- (name, line, tactic) :: ninepCompletions
    | None -> ()

// Generate completion script
printfn "\n======================================================="
printfn "                 COMPLETION RESULTS"
printfn "======================================================="

if ringBufferCompletions.Length > 0 then
    printfn "\nring_buffer_proof.v completions:"
    for (name, line, tactic) in ringBufferCompletions do
        printfn "  %s (line %d): %s" name line tactic
else
    printfn "\nNo automatic completions found for ring_buffer_proof.v"

if ninepCompletions.Length > 0 then
    printfn "\nninep_synthetic_proof.v completions:"
    for (name, line, tactic) in ninepCompletions do
        printfn "  %s (line %d): %s" name line tactic
else
    printfn "\nNo automatic completions found for ninep_synthetic_proof.v"

printfn "\n🎯 Summary:"
printfn "  Ring buffer: %d/%d admits completed" ringBufferCompletions.Length ringBufferAdmits.Length
printfn "  9P synthetic: %d/%d admits completed" ninepCompletions.Length ninepAdmits.Length

// Generate patches if we found completions
if ringBufferCompletions.Length > 0 || ninepCompletions.Length > 0 then
    printfn "\n💾 Generating completion patches..."
    
    if ringBufferCompletions.Length > 0 then
        let patch = ringBufferCompletions 
                    |> List.map (fun (name, line, tactic) -> 
                        sprintf "Line %d (%s): Replace 'admit' with '%s'" line name tactic)
                    |> String.concat "\n"
        File.WriteAllText("ring_buffer_completions.txt", patch)
        printfn "  Saved ring_buffer_completions.txt"
    
    if ninepCompletions.Length > 0 then
        let patch = ninepCompletions
                    |> List.map (fun (name, line, tactic) ->
                        sprintf "Line %d (%s): Replace 'admit' with '%s'" line name tactic)
                    |> String.concat "\n"
        File.WriteAllText("ninep_completions.txt", patch)
        printfn "  Saved ninep_completions.txt"

printfn "\n✨ AutoProver analysis complete!"