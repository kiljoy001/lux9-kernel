/*
 * CLR VTable Runtime Implementation
 *
 * Implements virtual method dispatch, type registry, and interface lookup.
 * These are the runtime functions for callvirt and type operations.
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

#include "../../9front-pc64/mem.h"
#include "../../include/dat.h"
#include "../../include/fns.h"
#include "../../port/lib.h"

#include "clr_pebble_integration.h"

/* ========== Type Registry ========== */

/* Initialize type registry */
void clr_type_registry_init(clr_type_registry_t *registry) {
  if (!registry)
    return;
  registry->types_head = nil;
  registry->types_tail = nil;
  registry->type_count = 0;
  memset(&registry->lock, 0, sizeof(Lock));
}

/* Register a type with its VTable */
int clr_register_type(clr_type_registry_t *registry, clr_type_info_t *type) {
  if (!registry || !type)
    return -1;

  lock(&registry->lock);

  /* Check if already registered */
  clr_type_info_t *existing = registry->types_head;
  while (existing) {
    if (existing->type_token == type->type_token) {
      unlock(&registry->lock);
      return 0; /* Already registered */
    }
    existing = existing->next;
  }

  /* Add to end of list */
  type->next = nil;
  type->prev = registry->types_tail;

  if (registry->types_tail) {
    registry->types_tail->next = type;
  } else {
    registry->types_head = type;
  }
  registry->types_tail = type;
  registry->type_count++;

  unlock(&registry->lock);
  return 0;
}

/* Lookup type info by token */
clr_type_info_t *clr_lookup_type(clr_type_registry_t *registry, u32int token) {
  if (!registry)
    return nil;

  lock(&registry->lock);

  clr_type_info_t *type = registry->types_head;
  while (type) {
    if (type->type_token == token) {
      unlock(&registry->lock);
      return type;
    }
    type = type->next;
  }

  unlock(&registry->lock);
  return nil;
}

/* ========== VTable Construction ========== */

/* Build VTable for type (resolves parent chain) */
int clr_build_vtable(clr_type_registry_t *registry, clr_type_info_t *type) {
  if (!registry || !type)
    return -1;

  /* If parent exists, resolve it */
  if (type->parent_token != 0 && !type->vtable->parent) {
    clr_type_info_t *parent = clr_lookup_type(registry, type->parent_token);
    if (parent) {
      /* Ensure parent VTable is built first */
      if (parent->vtable && !parent->vtable->parent &&
          parent->parent_token != 0) {
        clr_build_vtable(registry, parent);
      }
      type->vtable->parent = parent->vtable;
    }
  }

  return 0;
}

/* ========== Virtual Method Dispatch ========== */

/* Virtual method dispatch: obj->type_info->vtable->slots[slot_index] */
void *clr_vtable_lookup(clr_object_t *obj, u32int method_token) {
  if (!obj || !obj->type_info || !obj->type_info->vtable) {
    return nil;
  }

  clr_vtable_t *vtable = obj->type_info->vtable;

  /* Search in current vtable */
  while (vtable) {
    for (ulong i = 0; i < vtable->slot_count; i++) {
      if (vtable->slots[i].method_token == method_token) {
        return vtable->slots[i].native_code;
      }
    }
    /* Walk up parent chain */
    vtable = vtable->parent;
  }

  return nil; /* Method not found */
}

/* Interface method dispatch */
void *clr_interface_lookup(clr_object_t *obj, u32int interface_token,
                           u32int slot_index) {
  if (!obj || !obj->type_info || !obj->type_info->vtable) {
    return nil;
  }

  clr_vtable_t *vtable = obj->type_info->vtable;

  /* Find interface in interface map */
  for (ulong i = 0; i < vtable->interface_count; i++) {
    if (vtable->interfaces[i].interface_token == interface_token) {
      u32int offset = vtable->interfaces[i].offset;
      if (offset + slot_index < vtable->slot_count) {
        return vtable->slots[offset + slot_index].native_code;
      }
      break;
    }
  }

  return nil; /* Interface method not found */
}

/* ========== Type Checking ========== */

/* Check if type is subtype of base (walks parent chain) */
int clr_is_subtype_of(clr_type_info_t *derived, clr_type_info_t *base) {
  if (!derived || !base)
    return 0;
  if (derived == base)
    return 1;
  if (derived->type_token == base->type_token)
    return 1;

  /* Walk parent chain */
  clr_vtable_t *vtable = derived->vtable;
  while (vtable && vtable->parent) {
    if (vtable->parent->type_token == base->type_token) {
      return 1;
    }
    vtable = vtable->parent;
  }

  /* Check interfaces */
  if (derived->vtable) {
    for (ulong i = 0; i < derived->vtable->interface_count; i++) {
      if (derived->vtable->interfaces[i].interface_token == base->type_token) {
        return 1;
      }
    }
  }

  return 0;
}

/* Check if object is instance of type (for isinst opcode) */
int clr_is_instance_of(clr_object_t *obj, u32int type_token) {
  if (!obj || !obj->type_info)
    return 0;

  /* Exact match */
  if (obj->type_info->type_token == type_token)
    return 1;

  /* Check parent chain via vtable */
  clr_vtable_t *vtable = obj->type_info->vtable;
  while (vtable) {
    if (vtable->type_token == type_token) {
      return 1;
    }
    vtable = vtable->parent;
  }

  /* Check interfaces */
  if (obj->type_info->vtable) {
    for (ulong i = 0; i < obj->type_info->vtable->interface_count; i++) {
      if (obj->type_info->vtable->interfaces[i].interface_token == type_token) {
        return 1;
      }
    }
  }

  return 0;
}

/* ========== VTable Allocation ========== */

/* Allocate a new VTable */
clr_vtable_t *clr_vtable_alloc(ulong slot_count, ulong interface_count) {
  clr_vtable_t *vtable = xalloc(sizeof(clr_vtable_t));
  if (!vtable)
    return nil;

  memset(vtable, 0, sizeof(clr_vtable_t));

  if (slot_count > 0) {
    vtable->slots = xalloc(sizeof(clr_vtable_slot_t) * slot_count);
    if (!vtable->slots) {
      xfree(vtable);
      return nil;
    }
    memset(vtable->slots, 0, sizeof(clr_vtable_slot_t) * slot_count);
    vtable->slot_count = slot_count;
  }

  if (interface_count > 0) {
    vtable->interfaces = xalloc(sizeof(struct clr_iface_map) * interface_count);
    if (!vtable->interfaces) {
      if (vtable->slots)
        xfree(vtable->slots);
      xfree(vtable);
      return nil;
    }
    memset(vtable->interfaces, 0,
           sizeof(struct clr_iface_map) * interface_count);
    vtable->interface_count = interface_count;
  }

  return vtable;
}

/* Free a VTable */
void clr_vtable_free(clr_vtable_t *vtable) {
  if (!vtable)
    return;
  if (vtable->slots)
    xfree(vtable->slots);
  if (vtable->interfaces)
    xfree(vtable->interfaces);
  xfree(vtable);
}

/* Allocate type info */
clr_type_info_t *clr_type_info_alloc(const char *ns_name, const char *name,
                                     u32int type_token, u32int parent_token,
                                     ulong field_count) {
  clr_type_info_t *type = xalloc(sizeof(clr_type_info_t));
  if (!type)
    return nil;

  memset(type, 0, sizeof(clr_type_info_t));
  type->type_token = type_token;
  type->parent_token = parent_token;
  type->ns_name = ns_name;
  type->name = name;

  if (field_count > 0) {
    type->fields = xalloc(sizeof(struct clr_field_info) * field_count);
    if (!type->fields) {
      xfree(type);
      return nil;
    }
    memset(type->fields, 0, sizeof(struct clr_field_info) * field_count);
    type->field_count = field_count;
  }

  return type;
}

/* Free type info */
void clr_type_info_free(clr_type_info_t *type) {
  if (!type)
    return;
  if (type->vtable)
    clr_vtable_free(type->vtable);
  if (type->fields)
    xfree(type->fields);
  xfree(type);
}
