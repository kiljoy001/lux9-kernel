(** Frama-C Plan 9 Plugin Interface *)

(** {1 Plugin Information} *)

(** The Plan9 plugin enables formal verification of Plan 9 C code by:
    - Providing missing type definitions excluded by #ifndef __FRAMAC__
    - Filtering incompatible Plan 9 pragmas
    - Configuring appropriate preprocessor options *)

(** {1 Plugin Options} *)

(** Enable Plan 9 compatibility mode *)
module Enabled: Parameter_sig.Bool

(** Enable verbose debugging output *)
module Verbose: Parameter_sig.Bool

(** {1 Internal Functions} *)

(** Main plugin initialization function *)
val run: unit -> unit
