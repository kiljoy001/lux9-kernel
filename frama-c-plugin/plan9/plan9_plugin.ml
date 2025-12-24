(* Frama-C Plan 9 Plugin
 * Enables seamless formal verification of Plan 9 C code
 *
 * This plugin:
 * - Automatically preprocesses Plan 9 code
 * - Provides missing type definitions
 * - Filters incompatible pragmas
 * - Registers Plan 9-specific options
 *)

open Cil_types
open Cil

(* ========================================================================== *)
(* Plugin Registration                                                        *)
(* ========================================================================== *)

module Self = Plugin.Register
  (struct
    let name = "Plan 9 Support"
    let shortname = "plan9"
    let help = "Enables formal verification of Plan 9 C code"
  end)

module Enabled = Self.False
  (struct
    let option_name = "-plan9"
    let help = "Enable Plan 9 compatibility mode"
  end)

module Verbose = Self.False
  (struct
    let option_name = "-plan9-verbose"
    let help = "Print Plan 9 plugin debug information"
  end)

(* ========================================================================== *)
(* Plan 9 Type Definitions                                                    *)
(* ========================================================================== *)

let plan9_missing_types = "
/* Frama-C Plan 9 Plugin - Missing Types */

/* UUID type - not in Plan 9 headers */
typedef unsigned char uuid_t[16];

/* Types excluded by #ifndef __FRAMAC__ in Plan 9 headers */
typedef struct Fmt Fmt;
typedef int (*Fmts)(Fmt *);
struct Fmt {
    unsigned char runes;
    void *start;
    void *to;
    void *stop;
    int (*flush)(Fmt *);
    void *farg;
    int nfmt;
    void *args;
    int r;
    int width;
    int prec;
    unsigned long flags;
};

typedef struct Qid Qid;
struct Qid {
    unsigned long long path;
    unsigned long vers;
    unsigned char type;
};

typedef struct Waitmsg Waitmsg;
struct Waitmsg {
    int pid;
    unsigned long time[3];
    char msg[128];
};

/* Dir structure helper */
typedef struct Dir Dir;
struct Dir {
    unsigned short type;
    unsigned int dev;
    Qid qid;
    unsigned long mode;
    unsigned long atime;
    unsigned long mtime;
    long long length;
    char *name;
    char *uid;
    char *gid;
    char *muid;
};
"

(* ========================================================================== *)
(* Pragma Filtering                                                           *)
(* ========================================================================== *)

let plan9_pragma_patterns = [
  "varargck";
  "lib";
  "src";
  "incomplete";
  "pack";
  "textflag";
  "profile";
]

let is_plan9_pragma pragma_text =
  List.exists
    (fun pattern ->
      try
        let _ = Str.search_forward (Str.regexp pattern) pragma_text 0 in
        true
      with Not_found -> false)
    plan9_pragma_patterns

(* ========================================================================== *)
(* Preprocessing Hook                                                         *)
(* ========================================================================== *)

let plan9_preprocessor input_file =
  if Enabled.get () then begin
    if Verbose.get () then
      Self.result "Preprocessing Plan 9 file: %s" input_file;

    (* Create temporary file with missing types header *)
    let temp_file = Filename.temp_file "framac_plan9_" ".c" in
    let oc = open_out temp_file in

    (* Write missing types header *)
    output_string oc plan9_missing_types;
    output_string oc "\n/* Original Plan 9 file begins here */\n\n";

    (* Append original file *)
    let ic = open_in input_file in
    begin try
      while true do
        output_string oc (input_line ic);
        output_char oc '\n'
      done
    with End_of_file -> ()
    end;
    close_in ic;
    close_out oc;

    if Verbose.get () then
      Self.result "Created preprocessed file: %s" temp_file;

    temp_file
  end else
    input_file

(* ========================================================================== *)
(* File Preprocessing                                                         *)
(* ========================================================================== *)

class plan9_visitor = object
  inherit Visitor.frama_c_inplace

  (* Remove Plan 9 pragmas during AST traversal *)
  method! vattr attr =
    match attr with
    | Attr (name, _) when is_plan9_pragma name ->
        if Verbose.get () then
          Self.result "Filtering Plan 9 pragma: %s" name;
        ChangeTo []
    | _ -> DoChildren
end

(* ========================================================================== *)
(* Plugin Initialization                                                      *)
(* ========================================================================== *)

let run () =
  if Enabled.get () then begin
    Self.result "Plan 9 compatibility mode enabled";

    (* Set required Frama-C options *)
    Kernel.CppCommand.set "gcc -E -C -D__FRAMAC__";

    (* Add Plan 9 include paths if they exist *)
    let plan9_includes = [
      "kernel/include";
      "kernel/9front-pc64";
      "kernel/9front-port";
    ] in

    List.iter (fun inc_path ->
      if Sys.file_exists inc_path then begin
        Kernel.CppExtraArgs.add_once ("-I" ^ inc_path);
        if Verbose.get () then
          Self.result "Added include path: %s" inc_path
      end
    ) plan9_includes;

    (* Apply Plan 9 visitor to clean up pragmas *)
    let visitor = new plan9_visitor in
    Visitor.visitFramacFileSameGlobals visitor (Ast.get ());

    Self.result "Plan 9 preprocessing complete"
  end

(* Register the plugin to run after file parsing *)
let () = Db.Main.extend run
