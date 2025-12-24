/* clr_assemblies.c - Assembly Cache */
#include "clr_assemblies.h"
#include "../../port/lib.h"
#include "../include/dat.h"
#include "../include/fns.h"
#include "../include/u.h"
#include "../include/uuid.h"

/* External helpers declared in fns.h usually, but ensuring we have what we need
 */
/* Minimal Linked List for now. Hash map later if needed. */
typedef struct clr_domain_func_t {
  il_assembly_t *assembly;
  char *name;
  struct clr_domain_func_t *next;
} clr_domain_node_t;

static clr_domain_node_t *domain_head = NULL;

void clr_assemblies_init(void) { domain_head = NULL; }

int clr_assemblies_add(il_assembly_t *assembly, const char *name) {
  if (!assembly)
    return -1;

  /* Check duplicate by MVID first? Or just add.
     If we reload same assembly, MVID check is useful.
  */
  uuid_t mvid;
  if (il_get_mvid(assembly, &mvid) == 0) {
    if (clr_assemblies_lookup_by_mvid(&mvid)) {
      /* Already loaded */
      return 0;
    }
  }

  clr_domain_node_t *node = mallocz(sizeof(clr_domain_node_t), 1);
  if (!node)
    return -1;

  node->assembly = assembly;
  if (name) {
    /* Use kstrdup: void kstrdup(char **, char *) */
    /* Allocates new string into first arg */
    kstrdup(&node->name, (char *)name);
  }

  node->next = domain_head;
  domain_head = node;

  return 0;
}

il_assembly_t *clr_assemblies_lookup_by_name(const char *name) {
  if (!name)
    return NULL;
  clr_domain_node_t *curr = domain_head;
  while (curr) {
    if (curr->name && strcmp(curr->name, name) == 0) {
      return curr->assembly;
    }
    curr = curr->next;
  }
  return NULL;
}

il_assembly_t *clr_assemblies_lookup_by_mvid(const uuid_t *mvid) {
  if (!mvid)
    return NULL;
  clr_domain_node_t *curr = domain_head;
  while (curr) {
    if (curr->assembly) {
      uuid_t asm_mvid;
      if (il_get_mvid(curr->assembly, &asm_mvid) == 0) {
        if (uuid_compare(mvid, &asm_mvid) == 0) {
          return curr->assembly;
        }
      }
    }
    curr = curr->next;
  }
  return NULL;
}
