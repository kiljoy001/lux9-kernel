(* Complete F# Abstract Syntax Tree *)
(* Based on F# Language Specification 4.1 *)

Require Import Coq.Lists.List.
Require Import Coq.Strings.String.
Require Import Coq.Arith.Arith.
Require Import Coq.Bool.Bool.
Import ListNotations.

(* ==================== COMPLETE F# TYPES ==================== *)

Inductive FType : Type :=
  (* Primitive types *)
  | TUnit : FType
  | TBool : FType
  | TInt : FType
  | TInt8 : FType
  | TInt16 : FType
  | TInt32 : FType  
  | TInt64 : FType
  | TUInt8 : FType
  | TUInt16 : FType
  | TUInt32 : FType
  | TUInt64 : FType
  | TFloat : FType
  | TFloat32 : FType
  | TDouble : FType
  | TDecimal : FType
  | TChar : FType
  | TString : FType
  
  (* Composite types *)
  | TArray : FType -> FType
  | TList : FType -> FType
  | TOption : FType -> FType
  | TTuple : list FType -> FType
  | TRecord : list (string * FType) -> FType
  | TUnion : list (string * list FType) -> FType
  
  (* Function types *)
  | TFun : FType -> FType -> FType
  
  (* Generic types *)
  | TVar : string -> FType
  | TGeneric : string -> list FType -> FType.

(* ==================== COMPLETE F# LITERALS ==================== *)

Inductive Literal : Type :=
  | LUnit : Literal
  | LBool : bool -> Literal
  | LInt : Z -> Literal
  | LInt8 : Z -> Literal
  | LInt16 : Z -> Literal  
  | LInt32 : Z -> Literal
  | LInt64 : Z -> Literal
  | LUInt8 : nat -> Literal
  | LUInt16 : nat -> Literal
  | LUInt32 : nat -> Literal
  | LUInt64 : nat -> Literal
  | LFloat : Z -> Z -> Literal  (* mantissa, exponent *)
  | LFloat32 : Z -> Z -> Literal
  | LDouble : Z -> Z -> Literal
  | LDecimal : Z -> Z -> Literal
  | LChar : nat -> Literal
  | LString : string -> Literal.

(* ==================== COMPLETE F# EXPRESSIONS ==================== *)

Inductive Pattern : Type :=
  | PWild : Pattern
  | PVar : string -> Pattern
  | PLit : Literal -> Pattern
  | PTuple : list Pattern -> Pattern
  | PList : list Pattern -> Pattern
  | PCons : Pattern -> Pattern -> Pattern
  | PArray : list Pattern -> Pattern
  | PRecord : list (string * Pattern) -> Pattern
  | PUnion : string -> list Pattern -> Pattern
  | PAs : Pattern -> string -> Pattern
  | POr : Pattern -> Pattern -> Pattern
  | PGuarded : Pattern -> FExpr -> Pattern
  
with FExpr : Type :=
  (* Literals and variables *)
  | ELit : Literal -> FExpr
  | EVar : string -> FExpr
  | ELongVar : list string -> FExpr  (* Module.function *)
  
  (* Let bindings *)
  | ELet : string -> FExpr -> FExpr -> FExpr
  | ELetRec : list (string * FExpr) -> FExpr -> FExpr
  
  (* Functions *)
  | EFun : list string -> FExpr -> FExpr
  | EApp : FExpr -> FExpr -> FExpr
  | EPartialApp : FExpr -> list FExpr -> FExpr
  
  (* Control flow *)
  | EIf : FExpr -> FExpr -> FExpr -> FExpr
  | EMatch : FExpr -> list MatchCase -> FExpr
  | ETry : FExpr -> list MatchCase -> option FExpr -> FExpr
  
  (* Loops *)
  | EFor : string -> FExpr -> FExpr -> FExpr -> FExpr
  | EForEach : string -> FExpr -> FExpr -> FExpr
  | EWhile : FExpr -> FExpr -> FExpr
  
  (* Data structures *)
  | ETuple : list FExpr -> FExpr
  | EList : list FExpr -> FExpr
  | EArray : list FExpr -> FExpr
  | ERecord : list (string * FExpr) -> FExpr
  | ERecordWith : FExpr -> list (string * FExpr) -> FExpr
  | EUnion : string -> list FExpr -> FExpr
  
  (* Operations *)
  | EBinOp : BinOp -> FExpr -> FExpr -> FExpr
  | EUnOp : UnOp -> FExpr -> FExpr
  
  (* Access *)
  | EField : FExpr -> string -> FExpr
  | EIndex : FExpr -> FExpr -> FExpr
  | ESlice : FExpr -> option FExpr -> option FExpr -> FExpr
  
  (* Type operations *)
  | ETypeAnnot : FExpr -> FType -> FExpr
  | ETypeCast : FExpr -> FType -> FExpr
  | ETypeTest : FExpr -> FType -> FExpr
  
  (* Advanced features *)
  | ESequence : FExpr -> FExpr -> FExpr
  | ELazy : FExpr -> FExpr
  | EForce : FExpr -> FExpr
  | EQuote : FExpr -> FExpr
  | EUnquote : FExpr -> FExpr
  
  (* Async/computation expressions *)
  | EAsync : FExpr -> FExpr
  | EAwait : FExpr -> FExpr
  | ECompExpr : string -> list CompClause -> FExpr
  
  (* Object-oriented features *)
  | ENew : FType -> list FExpr -> FExpr
  | EMethod : FExpr -> string -> list FExpr -> FExpr
  | EProperty : FExpr -> string -> FExpr
  
  (* Exception handling *)
  | ERaise : FExpr -> FExpr
  | EReraise : FExpr
  | EFailwith : FExpr -> FExpr
  | EInvalidArg : FExpr -> FExpr -> FExpr

with MatchCase : Type :=
  | MCase : Pattern -> option FExpr -> FExpr -> MatchCase

with CompClause : Type :=
  | CFor : string -> FExpr -> CompClause
  | CYield : FExpr -> CompClause  
  | CYieldFrom : FExpr -> CompClause
  | CReturn : FExpr -> CompClause
  | CReturnFrom : FExpr -> CompClause
  | CDo : FExpr -> CompClause
  | CDoYield : FExpr -> CompClause
  | CWhere : FExpr -> CompClause

with BinOp : Type :=
  (* Arithmetic *)
  | OpAdd | OpSub | OpMul | OpDiv | OpMod | OpPow
  
  (* Bitwise *)  
  | OpBitAnd | OpBitOr | OpBitXor | OpShiftL | OpShiftR
  
  (* Comparison *)
  | OpEq | OpNeq | OpLt | OpGt | OpLe | OpGe
  
  (* Logical *)
  | OpAnd | OpOr
  
  (* String/List *)
  | OpConcat | OpCons | OpAppend
  
  (* Pipe *)
  | OpPipe | OpBackPipe | OpCompose | OpComposeBack
  
  (* Assignment *)
  | OpAssign | OpAddAssign | OpSubAssign | OpMulAssign | OpDivAssign

with UnOp : Type :=
  | OpNot | OpNeg | OpBitNot | OpRef | OpDeref | OpAddr.

(* ==================== COMPLETE F# DECLARATIONS ==================== *)

Inductive FDecl : Type :=
  | DLet : string -> list string -> FExpr -> FDecl
  | DLetRec : list (string * list string * FExpr) -> FDecl
  | DType : string -> list string -> TypeDefn -> FDecl
  | DException : string -> list FType -> FDecl
  | DModule : string -> list FDecl -> FDecl
  | DNamespace : string -> list FDecl -> FDecl
  | DOpen : list string -> FDecl
  | DVal : string -> FType -> FDecl  (* External declaration *)
  | DExternal : string -> FType -> string -> FDecl

with TypeDefn : Type :=
  | TDRecord : list (string * FType) -> TypeDefn
  | TDUnion : list (string * list FType) -> TypeDefn
  | TDAlias : FType -> TypeDefn
  | TDClass : list ClassMember -> TypeDefn
  | TDInterface : list InterfaceMember -> TypeDefn
  | TDDelegate : list FType -> FType -> TypeDefn

with ClassMember : Type :=
  | CMField : string -> FType -> FExpr -> ClassMember
  | CMMethod : string -> list string -> list (string * FType) -> FType -> FExpr -> ClassMember
  | CMProperty : string -> FType -> option FExpr -> option FExpr -> ClassMember
  | CMConstructor : list (string * FType) -> FExpr -> ClassMember
  | CMInherit : FType -> list FExpr -> ClassMember
  | CMInterface : FType -> list InterfaceMember -> ClassMember

with InterfaceMember : Type :=  
  | IMMethod : string -> list FType -> FType -> InterfaceMember
  | IMProperty : string -> FType -> bool -> bool -> InterfaceMember.

(* ==================== WELL-FORMEDNESS PREDICATES ==================== *)

Inductive WellFormedType : FType -> Prop :=
  | WF_TUnit : WellFormedType TUnit
  | WF_TBool : WellFormedType TBool
  | WF_TInt : WellFormedType TInt
  | WF_TString : WellFormedType TString
  | WF_TArray : forall t, WellFormedType t -> WellFormedType (TArray t)
  | WF_TList : forall t, WellFormedType t -> WellFormedType (TList t)
  | WF_TOption : forall t, WellFormedType t -> WellFormedType (TOption t)
  | WF_TTuple : forall ts, 
      (forall t, In t ts -> WellFormedType t) -> 
      length ts >= 2 -> 
      WellFormedType (TTuple ts)
  | WF_TFun : forall t1 t2, 
      WellFormedType t1 -> WellFormedType t2 -> 
      WellFormedType (TFun t1 t2)
  | WF_TVar : forall s, s <> EmptyString -> WellFormedType (TVar s).

Inductive WellFormedExpr : FExpr -> Prop :=
  | WF_ELit : forall lit, WellFormedLiteral lit -> WellFormedExpr (ELit lit)
  | WF_EVar : forall x, x <> EmptyString -> WellFormedExpr (EVar x)
  | WF_ELet : forall x e1 e2,
      x <> EmptyString ->
      WellFormedExpr e1 ->
      WellFormedExpr e2 ->
      WellFormedExpr (ELet x e1 e2)
  | WF_EFun : forall params body,
      params <> [] ->
      (forall p, In p params -> p <> EmptyString) ->
      NoDup params ->
      WellFormedExpr body ->
      WellFormedExpr (EFun params body)
  | WF_EApp : forall f arg,
      WellFormedExpr f ->
      WellFormedExpr arg ->
      WellFormedExpr (EApp f arg)
  | WF_EIf : forall cond then_e else_e,
      WellFormedExpr cond ->
      WellFormedExpr then_e ->
      WellFormedExpr else_e ->
      WellFormedExpr (EIf cond then_e else_e)
  | WF_ETuple : forall es,
      length es >= 2 ->
      (forall e, In e es -> WellFormedExpr e) ->
      WellFormedExpr (ETuple es)
  | WF_EBinOp : forall op e1 e2,
      WellFormedExpr e1 ->
      WellFormedExpr e2 ->
      WellFormedExpr (EBinOp op e1 e2)

with WellFormedLiteral : Literal -> Prop :=
  | WF_LUnit : WellFormedLiteral LUnit
  | WF_LBool : forall b, WellFormedLiteral (LBool b)
  | WF_LInt : forall n, WellFormedLiteral (LInt n)
  | WF_LString : forall s, WellFormedLiteral (LString s)
  | WF_LChar : forall c, c < 256 -> WellFormedLiteral (LChar c).

(* ==================== SCOPING AND VARIABLE BINDING ==================== *)

Fixpoint free_vars (e : FExpr) : list string :=
  match e with
  | ELit _ => []
  | EVar x => [x]
  | ELet x e1 e2 => free_vars e1 ++ (filter (fun y => negb (string_dec x y)) (free_vars e2))
  | EFun params body => 
      fold_left (fun acc p => filter (fun y => negb (string_dec p y)) acc) 
                params (free_vars body)
  | EApp f arg => free_vars f ++ free_vars arg
  | EIf c t e => free_vars c ++ free_vars t ++ free_vars e
  | EBinOp _ e1 e2 => free_vars e1 ++ free_vars e2
  | ETuple es => fold_left (fun acc e => acc ++ free_vars e) es []
  | _ => [] (* Simplified for other cases *)
  end.

Definition closed (e : FExpr) : Prop := free_vars e = [].

(* ==================== TYPE ENVIRONMENT ==================== *)

Definition TypeEnv := list (string * FType).

Fixpoint lookup_type (env : TypeEnv) (x : string) : option FType :=
  match env with
  | [] => None
  | (y, t) :: rest => if string_dec x y then Some t else lookup_type rest x
  end.

Definition extend_env (env : TypeEnv) (x : string) (t : FType) : TypeEnv :=
  (x, t) :: env.

Definition extend_env_list (env : TypeEnv) (bindings : list (string * FType)) : TypeEnv :=
  bindings ++ env.

(* ==================== BASIC PROPERTIES ==================== *)

Theorem well_formed_closed_expr : forall e,
  WellFormedExpr e -> closed e -> 
  exists t, exists env, lookup_type env "dummy" = Some t.
Proof.
  intros e Hwf Hclosed.
  (* This is a placeholder - real type checking comes next *)
  exists TUnit, [("dummy", TUnit)].
  simpl. reflexivity.
Qed.

Print WellFormedExpr.
Print free_vars.
Print TypeEnv.