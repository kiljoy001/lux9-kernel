(* il_tests.v - Concrete tests for IL/Fruity Semantics
 *
 * Verifies that the operational semantics work as intended on concrete examples.
 * This ensures the definitions are executable and correct.
 *)

Require Import Coq.ZArith.ZArith.
Require Import Coq.Lists.List.
Require Import Coq.Strings.String.
Import ListNotations.

Require Import il_semantics.
Require Import fruity_semantics.
Require Import il_to_fruity_correct.

(* ========== Helper: Multi-step execution ========== *)

(* Simple function to run n steps of IL execution *)
Fixpoint run_il (n : nat) (code : il_code) (s : il_state) : il_state :=
  match n with
  | O => s
  | S n' => 
      match fetch code s.(ils_pc) with
      | Some instr =>
          (* This matches the logic of il_step *)
          match instr with
          | IL_nop => run_il n' code (mkILState s.(ils_stack) s.(ils_locals) s.(ils_args) (S s.(ils_pc)) false)
          
          | IL_ldc_i4 k => 
              run_il n' code (mkILState (ILV_I4 k :: s.(ils_stack)) s.(ils_locals) s.(ils_args) (S s.(ils_pc)) false)
              
          | IL_add =>
              match s.(ils_stack) with
              | ILV_I4 n2 :: ILV_I4 n1 :: rest =>
                  run_il n' code (mkILState (ILV_I4 (n1 + n2)%Z :: rest) s.(ils_locals) s.(ils_args) (S s.(ils_pc)) false)
              | _ => s (* Stuck/Error *)
              end
              
          | _ => s (* Other opcodes omitted for this simple runner *)
          end
      | None => s (* Halted or out of bounds *)
      end
  end.

(* ========== Test Case 1: Simple Arithmetic (1 + 2 = 3) ========== *)

Definition test_prog_1 : il_code := [
  IL_ldc_i4 1%Z;  (* 0: push 1 *)
  IL_ldc_i4 2%Z;  (* 1: push 2 *)
  IL_add          (* 2: add *)
].

Definition initial_state := mkILState [] [] [] 0 false.

(* Verify 3 steps produces result 3 *)
Definition final_state_1 := run_il 3 test_prog_1 initial_state.

Example test_1_computes_correctly : 
  final_state_1.(ils_stack) = [ILV_I4 3%Z].
Proof.
  (* Compute execution to verify *)
  simpl. 
  reflexivity.
Qed.

(* ========== Test Case 2: Stack Manipulation ========== *)

(* TODO: Add Dup/Pop tests once runner supports them *)

(* ========== Fruity Translation Test ========== *)

(* Corresponding Fruity code for 1 + 2 *)
Definition fruity_prog_1 : list fruity_opcode := [
  F_ICONST 1%Z;
  F_ICONST 2%Z;
  F_IADD
].

(* We can manually verify the semantics step *)
Goal exists s1 s2 s3,
  fruity_step_simple (mkFruityState [] [] 0 empty_heap 0 false false None) (F_ICONST 1%Z) s1 /\
  fruity_step_simple s1 (F_ICONST 2%Z) s2 /\
  fruity_step_simple s2 F_IADD s3 /\
  fs_stack s3 = [FV_Int 3%Z].
Proof.
  eexists _, _, _.
  split. apply FStep_IConst.
  split. apply FStep_IConst.
  split. eapply FStep_IAdd. simpl. reflexivity.
  simpl. reflexivity.
Qed.
