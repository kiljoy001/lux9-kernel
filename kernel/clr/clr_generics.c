/*
 * CLR Generics Runtime Implementation
 *
 * Implements generic type instantiation and caching.
 */

/* Manual Plan 9 Type Definitions */
#define _U_H_
#define nil ((void *)0)
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long uvlong;
typedef long long vlong;
typedef unsigned long usize;
typedef unsigned long uintptr;
typedef unsigned char u8int;
typedef unsigned short u16int;
typedef unsigned int u32int;
typedef unsigned long long u64int;
typedef signed char s8int;
typedef signed short s16int;
typedef signed int s32int;
typedef signed long long s64int;

typedef struct Qid Qid;
typedef struct Dir Dir;
typedef struct Waitmsg Waitmsg;
typedef struct Fmt Fmt;

#include "../9front-pc64/mem.h"
#include "../include/dat.h"
#include "../include/fns.h"
#include "../port/lib.h"

#include "clr_generics.h"

/* ========== Generics Registry ========== */

void clr_generics_init(clr_generics_registry_t *registry) {
  if (!registry)
    return;
  memset(registry, 0, sizeof(clr_generics_registry_t));
}

/* ========== Type Registration ========== */

int clr_register_generic_type(clr_generics_registry_t *registry,
                              clr_generic_typedef_t *generic) {
  if (!registry || !generic)
    return -1;

  lock(&registry->lock);

  /* Add to end of list */
  generic->next = nil;
  generic->prev = registry->types_tail;

  if (registry->types_tail) {
    registry->types_tail->next = generic;
  } else {
    registry->types_head = generic;
  }
  registry->types_tail = generic;
  registry->type_count++;

  unlock(&registry->lock);
  return 0;
}

clr_generic_typedef_t *
clr_lookup_generic_type(clr_generics_registry_t *registry, u32int type_token) {
  if (!registry)
    return nil;

  lock(&registry->lock);

  clr_generic_typedef_t *type = registry->types_head;
  while (type) {
    if (type->type_token == type_token) {
      unlock(&registry->lock);
      return type;
    }
    type =
        (clr_generic_typedef_t *)type->instances_head; /* Walk list via next */
  }

  unlock(&registry->lock);
  return nil;
}

/* ========== Type Instantiation ========== */

/* Check if instantiation matches type arguments */
static int inst_matches(clr_generic_inst_t *inst, clr_type_info_t **type_args,
                        ulong type_arg_count) {
  if (inst->type_arg_count != type_arg_count)
    return 0;

  for (ulong i = 0; i < type_arg_count; i++) {
    if (inst->type_args[i]->type_token != type_args[i]->type_token) {
      return 0;
    }
  }
  return 1;
}

/* Find existing instantiation in cache */
static clr_generic_inst_t *find_cached_inst(clr_generic_typedef_t *generic,
                                            clr_type_info_t **type_args,
                                            ulong type_arg_count) {
  clr_generic_inst_t *inst = generic->instances_head;
  while (inst) {
    if (inst_matches(inst, type_args, type_arg_count)) {
      return inst;
    }
    inst = inst->next;
  }
  return nil;
}

clr_generic_inst_t *clr_instantiate_generic(clr_generics_registry_t *registry,
                                            u32int generic_token,
                                            clr_type_info_t **type_args,
                                            ulong type_arg_count) {
  /* Find open generic type */
  clr_generic_typedef_t *generic =
      clr_lookup_generic_type(registry, generic_token);
  if (!generic)
    return nil;

  lock(&generic->lock);

  /* Check cache first */
  clr_generic_inst_t *cached =
      find_cached_inst(generic, type_args, type_arg_count);
  if (cached) {
    unlock(&generic->lock);
    return cached;
  }

  /* Create new instantiation */
  clr_generic_inst_t *inst = xalloc(sizeof(clr_generic_inst_t));
  if (!inst) {
    unlock(&generic->lock);
    return nil;
  }
  memset(inst, 0, sizeof(clr_generic_inst_t));

  inst->generic_def = generic;
  inst->type_arg_count = type_arg_count;

  /* Copy type arguments */
  inst->type_args = xalloc(sizeof(clr_type_info_t *) * type_arg_count);
  if (!inst->type_args) {
    xfree(inst);
    unlock(&generic->lock);
    return nil;
  }
  for (ulong i = 0; i < type_arg_count; i++) {
    inst->type_args[i] = type_args[i];
  }

  /* Create instantiated type info */
  inst->instantiated_type = xalloc(sizeof(clr_type_info_t));
  if (!inst->instantiated_type) {
    xfree(inst->type_args);
    xfree(inst);
    unlock(&generic->lock);
    return nil;
  }

  /* Copy base type info and substitute type parameters */
  if (generic->base_info) {
    memcpy(inst->instantiated_type, generic->base_info,
           sizeof(clr_type_info_t));
  }

  /* Generate unique token for this instantiation */
  inst->instantiated_type->type_token =
      (generic_token & 0xFF000000) |
      ((u32int)(generic->instance_count + 1) & 0x00FFFFFF);

  /* Add to cache */
  inst->next = nil;
  inst->prev = generic->instances_tail;
  if (generic->instances_tail) {
    generic->instances_tail->next = inst;
  } else {
    generic->instances_head = inst;
  }
  generic->instances_tail = inst;
  generic->instance_count++;

  unlock(&generic->lock);
  return inst;
}

/* ========== Generic Method Registration ========== */

int clr_register_generic_method(clr_generics_registry_t *registry,
                                clr_generic_methoddef_t *method) {
  if (!registry || !method)
    return -1;

  lock(&registry->lock);

  method->next = nil;
  method->prev = registry->methods_tail;

  if (registry->methods_tail) {
    registry->methods_tail->next = method;
  } else {
    registry->methods_head = method;
  }
  registry->methods_tail = method;
  registry->method_count++;

  unlock(&registry->lock);
  return 0;
}

/* Find open generic method */
static clr_generic_methoddef_t *
find_generic_method(clr_generics_registry_t *registry, u32int method_token) {
  clr_generic_methoddef_t *method = registry->methods_head;
  while (method) {
    if (method->method_token == method_token) {
      return method;
    }
    method = (clr_generic_methoddef_t *)method->instances_head; /* Walk list */
  }
  return nil;
}

clr_generic_method_inst_t *
clr_instantiate_generic_method(clr_generics_registry_t *registry,
                               u32int method_token, clr_type_info_t **type_args,
                               ulong type_arg_count) {

  clr_generic_methoddef_t *method = find_generic_method(registry, method_token);
  if (!method)
    return nil;

  lock(&method->lock);

  /* Check cache */
  clr_generic_method_inst_t *inst = method->instances_head;
  while (inst) {
    if (inst->type_arg_count == type_arg_count) {
      int match = 1;
      for (ulong i = 0; i < type_arg_count && match; i++) {
        if (inst->type_args[i]->type_token != type_args[i]->type_token) {
          match = 0;
        }
      }
      if (match) {
        unlock(&method->lock);
        return inst;
      }
    }
    inst = inst->next;
  }

  /* Create new instantiation */
  inst = xalloc(sizeof(clr_generic_method_inst_t));
  if (!inst) {
    unlock(&method->lock);
    return nil;
  }
  memset(inst, 0, sizeof(clr_generic_method_inst_t));

  inst->method_def = method;
  inst->type_arg_count = type_arg_count;
  inst->type_args = xalloc(sizeof(clr_type_info_t *) * type_arg_count);
  if (!inst->type_args) {
    xfree(inst);
    unlock(&method->lock);
    return nil;
  }
  for (ulong i = 0; i < type_arg_count; i++) {
    inst->type_args[i] = type_args[i];
  }

  /* Add to cache */
  inst->next = nil;
  inst->prev = method->instances_tail;
  if (method->instances_tail) {
    method->instances_tail->next = inst;
  } else {
    method->instances_head = inst;
  }
  method->instances_tail = inst;

  unlock(&method->lock);
  return inst;
}

/* ========== Type Parameter Utilities ========== */

clr_type_info_t *clr_get_type_arg(clr_generic_inst_t *inst, ulong index) {
  if (!inst || index >= inst->type_arg_count)
    return nil;
  return inst->type_args[index];
}

/* Check if type token refers to a type parameter */
/* ECMA-335: Type parameter tokens have high byte 0x1B (!T) or 0x1E (!!T) */
int clr_is_type_param(u32int type_token) {
  u8int table = (type_token >> 24) & 0xFF;
  return (table == 0x1B || table == 0x1E);
}

clr_type_info_t *clr_substitute_type_params(clr_type_info_t *open_type,
                                            clr_type_info_t **type_args,
                                            ulong type_arg_count) {
  if (!open_type)
    return nil;

  /* If not a type parameter, return as-is */
  if (!clr_is_type_param(open_type->type_token)) {
    return open_type;
  }

  /* Extract parameter index from token */
  u32int index = open_type->type_token & 0x00FFFFFF;
  if (index >= type_arg_count)
    return nil;

  return type_args[index];
}
