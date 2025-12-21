Require Import Coq.Lists.List.
Require Import Coq.Arith.Arith.
Require Import Coq.Bool.Bool.
Require Import Lia.

Record DAGState := {
  nodes : list nat;
  edges : nat -> nat -> bool
}.

(* Inductive definition of reachability (transitive closure of edges) *)
Inductive reachable (dag : DAGState) : nat -> nat -> Prop :=
  | reach_edge : forall x y, dag.(edges) x y = true -> reachable dag x y
  | reach_trans : forall x y z, reachable dag x y -> reachable dag y z -> reachable dag x z.

(* Acyclicity: No node can reach itself *)
Definition acyclic (dag : DAGState) : Prop :=
  forall x, In x dag.(nodes) -> ~ reachable dag x x.

Lemma edges_to_new_must_be_loop : forall dag new_node parents u,
  (forall m, dag.(edges) m new_node = false) ->
  let new_dag := {| nodes := new_node :: dag.(nodes);
                    edges := fun i j => if Nat.eqb i new_node then existsb (Nat.eqb j) parents else dag.(edges) i j |} in
  new_dag.(edges) u new_node = true -> u = new_node.
Proof.
  intros. unfold new_dag in H0. simpl in H0.
  destruct (Nat.eq_dec u new_node).
  - auto.
  - apply Nat.eqb_neq in n. rewrite n in H0.
    rewrite H in H0. discriminate.
Qed.

Lemma only_new_can_reach_new : forall dag new_node parents x,
  (forall m, dag.(edges) m new_node = false) ->
  let new_dag := {| nodes := new_node :: dag.(nodes);
                    edges := fun i j => if Nat.eqb i new_node then existsb (Nat.eqb j) parents else dag.(edges) i j |} in
  reachable new_dag x new_node -> x = new_node.
Proof.
  intros dag new_node parents x Hfresh new_dag Hreach.
  assert (Himpl: new_node = new_node -> x = new_node).
  {
    revert x Hreach. intros x Hreach.
    apply reachable_ind with (dag := new_dag) (P := fun u v => v = new_node -> u = new_node).
    - (* Edge *) 
      intros u v Hedge Heq. subst. 
      apply edges_to_new_must_be_loop with (dag:=dag) (parents:=parents) (u:=u); auto.
    - (* Trans *) 
      intros u v w Hjuv IH1 Hvw IH2 Heq. subst.
      apply IH1. apply IH2. reflexivity.
    - exact Hreach.
  }
  apply Himpl. reflexivity.
Qed.

Theorem dag_acyclicity_preserved :
  forall (dag : DAGState) (new_node : nat) (parents : list nat),
    acyclic dag ->
    ~In new_node dag.(nodes) ->
    (forall p, In p parents -> In p dag.(nodes)) ->
    (forall m, In m dag.(nodes) -> dag.(edges) m new_node = false) -> 
    (forall x y, dag.(edges) x y = true -> In x dag.(nodes) /\ In y dag.(nodes)) -> (* Closed graph *)
    let new_dag := {|
      nodes := new_node :: dag.(nodes);
      edges := fun i j =>
        if Nat.eqb i new_node then existsb (Nat.eqb j) parents
        else dag.(edges) i j
    |} in
    acyclic new_dag.
Proof.
  intros dag new_node parents Hacyclic Hnew Hparents Hfresh Hclosed new_dag.
  unfold acyclic. intros z Hz.
  unfold new_dag in *. simpl in Hz.
  destruct Hz as [Heq | Hin].
  - (* z = new_node *)
    subst z. intro Hloop.
    (* Cycle at new_node means reachable new new. *)
    (* Inversion on reachable new new *)
    (* If direct edge: new->new. *)
    (* If transitive: new->y->new. *)
    (* Case 1: Direct edge. *)
    (* edge new new = existsb (eq new) parents. *)
    (* If true, new in parents. False. *)
    (* Case 2: Transitive. new -> y -> new. *)
    (* y->new means y=new (only_new_can_reach_new). *)
    (* So new->new. Same as Case 1. *)
    (* But `only_new_can_reach_new` requires global freshness. *)
    (* Hfresh is constrained to `In m dag.nodes`. *)
    (* We need global freshness proof from Hclosed. *)
    assert (Hglobal: forall m, edges dag m new_node = false).
    { intros m. destruct (edges dag m new_node) eqn:E; auto.
      pose proof E as E_copy. apply Hclosed in E_copy. destruct E_copy as [Hin _].
      rewrite (Hfresh m Hin) in E. discriminate. }
    
    (* Now apply only_new_can_reach_new *)
    assert (Htarget: reachable new_dag new_node new_node -> new_node = new_node). auto.
    
    (* Induction on loop to find the first step from new_node to y *)
    (* Induction on loop to find the first step from new_node to y *)
    assert (Himpl: (new_node = new_node -> new_node = new_node -> False) -> False).
    { intro H. apply H; reflexivity. }
    apply Himpl; clear Himpl.
    
    apply reachable_ind with (dag:=new_dag) (P:=fun x y => x=new_node -> y=new_node -> False).
    { (* Edge *) intros u v Hedge Hu Hv. subst. unfold new_dag in Hedge. simpl in Hedge.
      destruct (Nat.eqb new_node new_node) eqn:Eeq.
      2: { apply Nat.eqb_neq in Eeq. contradiction Eeq; reflexivity. }
      apply existsb_exists in Hedge. destruct Hedge as [p [Hp Heq]].
      apply Nat.eqb_eq in Heq. subst.
      apply Hparents in Hp. apply Hnew in Hp. contradiction. }
    { (* Trans *) intros u v w Hjuv IH1 Hvw IH2 Hu Hw. subst.
      (* Hvw: reach v new. imply v=new. *)
      assert (Hv: v = new_node).
      { apply only_new_can_reach_new with (dag:=dag) (parents:=parents) in Hvw; auto. }
      subst.
      apply IH1; reflexivity. }
    { exact Hloop. }

  - (* z is in old nodes *)
    intro Hloop.
    (* reachable z z. Proving it holds in old DAG if z <> new *)
    (* First establish global freshness *)
    assert (Hglobal: forall m, edges dag m new_node = false).
    { intros m. destruct (edges dag m new_node) eqn:E; auto.
      pose proof E as E_copy. apply Hclosed in E_copy. destruct E_copy as [Hxin _].
      rewrite (Hfresh m Hxin) in E. discriminate. }
    
    assert (Hz_neq: z <> new_node).
    { intro. subst. apply Hnew in Hin. contradiction. }
    
    (* Key insight: for old nodes, paths in new_dag that don't touch new_node 
       are exactly paths in old dag. And paths that touch new_node would require
       reaching new_node, which by only_new_can_reach_new means z = new_node. *)
    
    assert (Hold: reachable dag z z).
    {
      (* We prove a more general statement by induction *)
      assert (Hgen: forall x y, reachable new_dag x y -> 
                    x <> new_node -> y <> new_node -> reachable dag x y).
      {
        intros x y Hreach.
        induction Hreach; intros Hx_neq Hy_neq.
        - (* Edge *)
          apply reach_edge.
          unfold new_dag in H. simpl in H.
          apply Nat.eqb_neq in Hx_neq. rewrite Hx_neq in H. exact H.
        - (* Trans x -> y0 -> z0 *)
          destruct (Nat.eq_dec y new_node).
          + (* y = new_node *)
            subst.
            (* x reaches new_node, so x = new_node by only_new_can_reach_new *)
            apply only_new_can_reach_new with (dag:=dag) (parents:=parents) in Hreach1; auto.
            subst. contradiction.
          + (* y <> new_node *)
            apply reach_trans with y.
            * apply IHHreach1; auto.
            * apply IHHreach2; auto.
      }
      apply Hgen with (x:=z) (y:=z); auto.
    }
    apply Hacyclic in Hin. contradiction.
Qed.
