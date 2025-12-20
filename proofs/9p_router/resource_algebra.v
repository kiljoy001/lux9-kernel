(** * Resource Algebra Interface
    * Abstracts the "Token" and "Permission" concepts for the 9P Router.
    * Allows instantiating the router with Lux9 Tokens (Pebble) or generic Unit tokens.
    *)

Require Import Coq.ZArith.ZArith.
Require Import Coq.Bool.Bool.

Module Type ResourceAlgebra.
  (** The type representing an access token (e.g., Pebble, Capability, or Unit) *)
  Parameter Token : Type.

  (** The type representing required permissions (e.g., Read/Write bits) *)
  Parameter Permissions : Type.

  (** Validation function:
      Given a token, the current system time, and the required permissions,
      returns true if access should be granted. *)
  Parameter validate : Token -> Z -> Permissions -> bool.

  (** Invariant: The zero/null token should usually be invalid for privileged operations,
      though this depends on the specific policy. *)
End ResourceAlgebra.
