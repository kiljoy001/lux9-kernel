(** Process WASM lifecycle verification
    Formal model of WASM process initialization and cleanup in proc.c and
    wasm_runtime.c. Proves cleanup invariants for exit and destroy paths. *)

Require Import Coq.Bool.Bool.
Require Import Coq.ZArith.ZArith.
Require Import Lia.

Require Import proc.proc_state_dag.

Open Scope Z_scope.

Definition Ptr := option unit.

Record WasmState := mkWasm {
  initialized : bool;
  runtime : Ptr;
  module : Ptr;
  env : Ptr;
  wasi_ctx : Ptr;
  linear_memory : Ptr;
  memory_size : Z;
  memory_pages : Z;
  linear_charged : Z;
  branch_tokens : Z;
}.

Definition WasmValid (w : WasmState) : Prop :=
  0 <= memory_size w /\
  0 <= memory_pages w /\
  0 <= linear_charged w /\
  0 <= branch_tokens w.

Definition WasmClean (w : WasmState) : Prop :=
  initialized w = false /\
  runtime w = None /\
  module w = None /\
  env w = None /\
  wasi_ctx w = None /\
  linear_memory w = None /\
  memory_size w = 0 /\
  memory_pages w = 0 /\
  linear_charged w = 0 /\
  branch_tokens w = 0.

Definition wasm_runtime_cleanup_process (w : WasmState) : WasmState :=
  if initialized w then
    mkWasm true (runtime w) (module w) None None None 0 0 0
           (branch_tokens w + linear_charged w)
  else
    w.

Definition wasm_destroy (w : WasmState) : WasmState :=
  if initialized w then
    mkWasm false None None None None None 0 0 0 0
  else
    w.

Definition wasm_exec_compile_fail (w : WasmState) (branch_inited : bool)
  : WasmState :=
  let drained :=
      if branch_inited then 0 else branch_tokens w in
  mkWasm false None None None None None 0 0 0 drained.

Definition wasm_exec_compile_success (w : WasmState) (mem_size mem_pages : Z)
  : WasmState :=
  mkWasm true (Some tt) (Some tt) (Some tt) (wasi_ctx w) (Some tt)
         mem_size mem_pages (linear_charged w) (branch_tokens w).

Lemma wasm_runtime_cleanup_zeroes_memory : forall w,
  initialized w = true ->
  memory_size (wasm_runtime_cleanup_process w) = 0 /\
  memory_pages (wasm_runtime_cleanup_process w) = 0 /\
  linear_charged (wasm_runtime_cleanup_process w) = 0 /\
  linear_memory (wasm_runtime_cleanup_process w) = None /\
  env (wasm_runtime_cleanup_process w) = None /\
  wasi_ctx (wasm_runtime_cleanup_process w) = None.
Proof.
  intros w Hinit.
  unfold wasm_runtime_cleanup_process.
  rewrite Hinit.
  simpl; repeat split; reflexivity.
Qed.

Lemma wasm_runtime_cleanup_preserves_valid : forall w,
  WasmValid w ->
  WasmValid (wasm_runtime_cleanup_process w).
Proof.
  intros w Hvalid.
  unfold WasmValid in *.
  destruct Hvalid as [Hmem [Hpages [Hlin Hbranch]]].
  unfold wasm_runtime_cleanup_process.
  destruct (initialized w) eqn:Hinit.
  - simpl. repeat split; try lia.
  - simpl. repeat split; assumption.
Qed.

Lemma wasm_destroy_clean : forall w,
  initialized w = true ->
  WasmClean (wasm_destroy w).
Proof.
  intros w Hinit.
  unfold wasm_destroy.
  rewrite Hinit.
  simpl; repeat split; reflexivity.
Qed.

Lemma wasm_exec_compile_fail_clean : forall w,
  wasm_exec_compile_fail w true = mkWasm false None None None None None 0 0 0 0.
Proof.
  intros w.
  unfold wasm_exec_compile_fail.
  simpl.
  reflexivity.
Qed.

Lemma wasm_exec_compile_success_valid : forall w mem_size mem_pages,
  WasmValid w ->
  0 <= mem_size ->
  0 <= mem_pages ->
  WasmValid (wasm_exec_compile_success w mem_size mem_pages).
Proof.
  intros w mem_size mem_pages Hvalid Hmem Hpages.
  unfold WasmValid in *.
  destruct Hvalid as [_ [_ [Hlin Hbranch]]].
  unfold wasm_exec_compile_success.
  simpl.
  repeat split; try lia.
Qed.

Lemma wasm_exec_compile_success_not_clean : forall w mem_size mem_pages,
  WasmClean (wasm_exec_compile_success w mem_size mem_pages) -> False.
Proof.
  intros w mem_size mem_pages Hclean.
  unfold WasmClean in Hclean.
  destruct Hclean as [Hinit _].
  discriminate.
Qed.

Lemma wasm_destroy_noop : forall w,
  initialized w = false ->
  wasm_destroy w = w.
Proof.
  intros w Hinit.
  unfold wasm_destroy.
  rewrite Hinit.
  reflexivity.
Qed.

Lemma wasm_clean_implies_valid : forall w,
  WasmClean w -> WasmValid w.
Proof.
  intros w Hclean.
  unfold WasmClean in Hclean.
  destruct Hclean as [_ [_ [_ [_ [_ [_ [Hmem [Hpages [Hlin Hbranch]]]]]]]]].
  unfold WasmValid.
  subst; repeat split; lia.
Qed.

Record ProcWasm := mkProcWasm {
  pw_state : ProcState;
  pw_wasm : WasmState;
}.

Definition ProcWasmInvariant (p : ProcWasm) : Prop :=
  WasmValid (pw_wasm p) /\
  (initialized (pw_wasm p) = false -> WasmClean (pw_wasm p)) /\
  (pw_state p = ProcDead -> WasmClean (pw_wasm p)).

Definition proc_exit (p : ProcWasm) : ProcWasm :=
  mkProcWasm ProcDead (wasm_destroy (pw_wasm p)).

Lemma proc_exit_preserves_invariant : forall p,
  ProcWasmInvariant p ->
  ProcWasmInvariant (proc_exit p).
Proof.
  intros p [Hvalid [Hclean_init Hdead]].
  unfold ProcWasmInvariant.
  split.
  - apply wasm_clean_implies_valid.
    destruct (initialized (pw_wasm p)) eqn:Hinit.
    + apply wasm_destroy_clean; exact Hinit.
    + unfold proc_exit.
      rewrite (wasm_destroy_noop _ Hinit).
      apply Hclean_init; reflexivity.
  - split.
    + intro Hinit_dead.
      destruct (initialized (pw_wasm p)) eqn:Hinit.
      * unfold proc_exit.
        apply wasm_destroy_clean; exact Hinit.
      * unfold proc_exit.
        rewrite (wasm_destroy_noop _ Hinit).
        apply Hclean_init; reflexivity.
    + intro Hdead_state.
      destruct (initialized (pw_wasm p)) eqn:Hinit.
      * apply wasm_destroy_clean; exact Hinit.
      * unfold proc_exit.
        rewrite (wasm_destroy_noop _ Hinit).
        apply Hclean_init; reflexivity.
Qed.
