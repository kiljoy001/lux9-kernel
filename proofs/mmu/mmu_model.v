(** Abstract MMU model extended with Physical Memory Map configuration. *)

From Coq Require Import List ZArith Lia Bool.
Import ListNotations.
Local Open Scope Z_scope.

(* Reuse existing MMU definitions or define new ones specific to the memory map *)
Section PhysicalMemoryMap.

(* Model of a memory region (Confmem) *)
Record Confmem := mkConfmem {
  base : Z;
  npage : Z;
  limit : Z
}.

Definition region_max (c : Confmem) : Z :=
  c.(base) + c.(npage) * 4096. (* Assuming 4096 page size *)

Definition is_valid_region (c : Confmem) : Prop :=
  c.(base) >= 0 /\ c.(npage) >= 0.

(* The list of memory regions *)
Variable conf_mem : list Confmem.

(* Calculate the maximum physical address from a list of regions *)
Fixpoint calc_max_phys (l : list Confmem) : Z :=
  match l with
  | [] => 0
  | c :: tl => Z.max (region_max c) (calc_max_phys tl)
  end.

(* Theorem: The calculated max is greater than or equal to any specific region's max *)
Theorem max_phys_is_upper_bound :
  forall l c, In c l -> region_max c <= calc_max_phys l.
Proof.
  intros l c Hin.
  induction l as [|head tail IH].
  - contradiction.
  - simpl.
    destruct Hin as [Heq | HinTail].
    + subst. apply Z.le_max_l.
    + apply Z.le_trans with (m := calc_max_phys tail).
      * apply IH; assumption.
      * apply Z.le_max_r.
Qed.

(* Theorem: If the list is empty, max is 0 *)
Theorem max_phys_empty : calc_max_phys [] = 0.
Proof. reflexivity. Qed.

(* Theorem: Adding a region strictly increases or maintains the max *)
Theorem max_phys_cons :
  forall c l, calc_max_phys (c :: l) >= calc_max_phys l.
Proof.
  intros c l.
  simpl.
  lia.
Qed.

End PhysicalMemoryMap.

(* Original content of mmu_model.v follows... *)
(* Constants taken from the C side (mem.h) but kept abstract. *)
Local Open Scope nat_scope.
Parameter KZERO : nat.
Definition BY2PG_nat : nat := 4096.

Definition addr := nat.

Record pte := {
  va : addr;
  pa : addr;
  user : bool;
  writable : bool
}.

Definition page_range (a : addr) : addr * addr := (a, a + BY2PG_nat).

Definition disjoint_range (a b : addr) : Prop :=
  a + BY2PG_nat <= b \/ b + BY2PG_nat <= a.

Definition page_aligned (a : addr) : Prop := a mod BY2PG_nat = 0.

Definition canonical_user (a : addr) : Prop := a < KZERO.
Definition canonical_kernel (a : addr) : Prop := KZERO <= a.

Definition wf_entry (e : pte) : Prop :=
  page_aligned (va e) /\ 
  page_aligned (pa e) /\ 
  (user e = true -> canonical_user (va e)) /\ 
  (user e = false -> canonical_kernel (va e)).

Definition pairwise_disjoint (l : list pte) : Prop :=
  forall e1 e2,
    In e1 l -> In e2 l -> e1 <> e2 ->
    disjoint_range (va e1) (va e2).

Definition wf (pt : list pte) : Prop :=
  Forall wf_entry pt /\ pairwise_disjoint pt.

Definition disjoint_from_va (a : addr) (e : pte) : Prop :=
  disjoint_range a (va e).

(* Address Translation Logic *)
Section AddressTranslation.
Local Open Scope Z_scope.

Parameter KZERO_Z : Z.
Parameter VMAP_Z : Z.
Parameter hhdm_base : Z.
Parameter max_physaddr_Z : Z.
Parameter limine_kernel_phys_base : Z.
Parameter end_marker : Z.
Parameter MiB_Z : Z.
Definition BY2PG_Z : Z := 4096.

(* Constraints on address space layout *)
Hypothesis hhdm_vmap_order : hhdm_base < VMAP_Z.
Hypothesis vmap_kzero_order : VMAP_Z < KZERO_Z.
Hypothesis hhdm_range : 0 < max_physaddr_Z.
Hypothesis end_in_kzero : KZERO_Z < end_marker.
Hypothesis MiB_pos : MiB_Z = 1024 * 1024.

Definition is_hhdm_va (va : Z) : bool :=
  (va >=? hhdm_base) && (va <? hhdm_base + (256 * 1024 * 1024 * 1024)).

Definition kaddr_z (pa : Z) : Z :=
  hhdm_base + pa.

Definition paddr_z (va : Z) : Z :=
  if (va >=? hhdm_base) && (va <? hhdm_base + (256 * 1024 * 1024 * 1024)) then
    va - hhdm_base
  else if (va >=? KZERO_Z) && (va <? end_marker) then
    (va - KZERO_Z) + (2 * MiB_Z)
  else if (va >=? KZERO_Z) then
    va - KZERO_Z
  else if (va >=? VMAP_Z) then
    va - VMAP_Z
  else
    0.

Theorem kaddr_paddr_inverse :
  forall pa, 
    0 <= pa -> 
    pa < (256 * 1024 * 1024 * 1024) ->
    paddr_z (kaddr_z pa) = pa.
Proof.
  intros pa Hlow Hhigh.
  unfold kaddr_z, paddr_z.
  assert (Hge: (hhdm_base + pa >=? hhdm_base) = true) by (apply Z.geb_le; lia).
  assert (Hlt: (hhdm_base + pa <? hhdm_base + 256 * 1024 * 1024 * 1024) = true) by (apply Z.ltb_lt; lia).
  rewrite Hge, Hlt.
  simpl. lia.
Qed.

Theorem paddr_kaddr_inverse_hhdm :
  forall va,
    is_hhdm_va va = true ->
    kaddr_z (paddr_z va) = va.
Proof.
  intros va Hhhdm.
  unfold is_hhdm_va in Hhhdm.
  apply andb_true_iff in Hhhdm.
  destruct Hhhdm as [Hge Hlt].
  unfold kaddr_z, paddr_z.
  rewrite Hge, Hlt.
  simpl. lia.
Qed.

Theorem no_va_aliasing_hhdm :
  forall va1 va2,
    is_hhdm_va va1 = true ->
    is_hhdm_va va2 = true ->
    paddr_z va1 = paddr_z va2 ->
    va1 = va2.
Proof.
  intros va1 va2 H1 H2 Heq.
  unfold is_hhdm_va in H1, H2.
  apply andb_true_iff in H1; destruct H1 as [Hge1 Hlt1].
  apply andb_true_iff in H2; destruct H2 as [Hge2 Hlt2].
  unfold paddr_z in Heq.
  rewrite Hge1, Hlt1, Hge2, Hlt2 in Heq.
  simpl in Heq. lia.
Qed.

End AddressTranslation.

(* Hierarchical Page Table Model *)
Section PageTableHierarchy.
Local Open Scope Z_scope.

Inductive pte_entry :=
| PTE_Invalid
| PTE_Link (pa : Z)
| PTE_Page (pa : Z) (perms : Z).

Definition PageTable := Z -> pte_entry.

(* Memory state mapping physical addresses to PageTables *)
Parameter mem_state : Z -> option PageTable.

Fixpoint lookup_level (pa : Z) (indices : list Z) : option Z :=
  match indices with
  | [] => Some pa
  | i :: rest =>
      match mem_state pa with
      | Some pt =>
          match pt i with
          | PTE_Link next_pa => lookup_level next_pa rest
          | PTE_Page base_pa _ => 
              (* Simplified: Huge pages would stop here *)
              if (length rest =? 0)%nat then Some base_pa else None
          | PTE_Invalid => None
          end
      | None => None
      end
  end.

Definition get_indices (va : Z) : list Z :=
  [ (va / (Z.shiftl 1 39)) mod 512;
    (va / (Z.shiftl 1 30)) mod 512;
    (va / (Z.shiftl 1 21)) mod 512;
    (va / (Z.shiftl 1 12)) mod 512 ].

Definition mmu_lookup (cr3 : Z) (va : Z) : option Z :=
  lookup_level cr3 (get_indices va).

(* CR3 switch preservation *)
Definition is_kernel_va (va : Z) : Prop :=
  va >= KZERO_Z.

Definition same_kernel_mappings (pa1 pa2 : Z) : Prop :=
  forall va, is_kernel_va va -> mmu_lookup pa1 va = mmu_lookup pa2 va.

Theorem cr3_switch_preserves_kernel :
  forall cr3_old cr3_new va,
    is_kernel_va va ->
    same_kernel_mappings cr3_old cr3_new ->
    mmu_lookup cr3_old va = mmu_lookup cr3_new va.
Proof.
  intros cr3_old cr3_new va Hkern Hsame.
  apply Hsame.
  assumption.
Qed.

End PageTableHierarchy.