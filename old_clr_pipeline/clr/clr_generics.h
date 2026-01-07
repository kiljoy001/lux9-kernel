/*
 * CLR Generics Runtime Support
 *
 * Implements generic type instantiation and method specialization.
 * Required for List<T>, Dictionary<K,V>, and generic method calls.
 */

#ifndef CLR_GENERICS_H
#define CLR_GENERICS_H

#include "../include/u.h"
#include "clr-kernel/clr_pebble_integration.h"

/* ========== Generic Type Parameter ========== */

typedef struct clr_type_param {
  u32int index;            /* Parameter index (0-based) */
  const char *name;        /* Parameter name (e.g., "T", "TKey") */
  u32int constraint_token; /* Constraint type token (0 = no constraint) */
  u32int flags;            /* Variance flags (covariant/contravariant) */
} clr_type_param_t;

/* ========== Generic Type Definition ========== */

typedef struct clr_generic_typedef {
  u32int type_token;   /* Open generic type token (e.g., List`1) */
  const char *ns_name; /* Namespace */
  const char *name;    /* Name with arity (e.g., "List`1") */

  clr_type_param_t *params; /* Type parameters */
  ulong param_count;        /* Number of type parameters */

  /* Base type info (used as template) */
  clr_type_info_t *base_info;

  /* Instantiation cache (intrusive list) */
  struct clr_generic_inst *instances_head;
  struct clr_generic_inst *instances_tail;
  ulong instance_count;

  Lock lock;
} clr_generic_typedef_t;

/* ========== Generic Instantiation ========== */

typedef struct clr_generic_inst {
  clr_generic_typedef_t *generic_def; /* The open generic type */

  /* Type arguments */
  clr_type_info_t **type_args; /* Array of concrete type arguments */
  ulong type_arg_count;

  /* Instantiated type info */
  clr_type_info_t *instantiated_type;

  /* Instantiated VTable */
  clr_vtable_t *instantiated_vtable;

  /* Intrusive list for caching */
  struct clr_generic_inst *next;
  struct clr_generic_inst *prev;
} clr_generic_inst_t;

/* ========== Generic Method Definition ========== */

typedef struct clr_generic_methoddef {
  u32int method_token; /* Open generic method token */
  const char *name;    /* Method name */

  clr_type_param_t *params; /* Method type parameters */
  ulong param_count;

  /* Method instantiation cache */
  struct clr_generic_method_inst *instances_head;
  struct clr_generic_method_inst *instances_tail;

  Lock lock;
} clr_generic_methoddef_t;

/* ========== Generic Method Instantiation ========== */

typedef struct clr_generic_method_inst {
  clr_generic_methoddef_t *method_def;

  clr_type_info_t **type_args;
  ulong type_arg_count;

  /* Compiled native code for this instantiation */
  void *native_code;
  u32int instantiated_token;

  struct clr_generic_method_inst *next;
  struct clr_generic_method_inst *prev;
} clr_generic_method_inst_t;

/* ========== Generics Registry ========== */

typedef struct clr_generics_registry {
  /* Open generic types */
  clr_generic_typedef_t *types_head;
  clr_generic_typedef_t *types_tail;
  ulong type_count;

  /* Open generic methods */
  clr_generic_methoddef_t *methods_head;
  clr_generic_methoddef_t *methods_tail;
  ulong method_count;

  Lock lock;
} clr_generics_registry_t;

/* ========== API Functions ========== */

/* Initialize generics registry */
void clr_generics_init(clr_generics_registry_t *registry);

/* Register an open generic type (e.g., List`1) */
int clr_register_generic_type(clr_generics_registry_t *registry,
                              clr_generic_typedef_t *generic);

/* Lookup open generic type by token */
clr_generic_typedef_t *
clr_lookup_generic_type(clr_generics_registry_t *registry, u32int type_token);

/* Instantiate generic type with concrete type arguments */
/* Returns cached instantiation if already exists */
clr_generic_inst_t *clr_instantiate_generic(clr_generics_registry_t *registry,
                                            u32int generic_token,
                                            clr_type_info_t **type_args,
                                            ulong type_arg_count);

/* Register an open generic method */
int clr_register_generic_method(clr_generics_registry_t *registry,
                                clr_generic_methoddef_t *method);

/* Instantiate generic method with concrete type arguments */
clr_generic_method_inst_t *
clr_instantiate_generic_method(clr_generics_registry_t *registry,
                               u32int method_token, clr_type_info_t **type_args,
                               ulong type_arg_count);

/* Get type argument by index in current generic context */
clr_type_info_t *clr_get_type_arg(clr_generic_inst_t *inst, ulong index);

/* Check if type token refers to a type parameter (!T or !!T) */
int clr_is_type_param(u32int type_token);

/* Substitute type parameters with actual type arguments */
clr_type_info_t *clr_substitute_type_params(clr_type_info_t *open_type,
                                            clr_type_info_t **type_args,
                                            ulong type_arg_count);

#endif /* CLR_GENERICS_H */
