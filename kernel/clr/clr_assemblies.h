#ifndef _CLR_ASSEMBLIES_H_
#define _CLR_ASSEMBLIES_H_

#include "il_parser.h"

/* CLR Domain: Manages loaded assemblies */

/* Initialize the assembly catalog */
void clr_assemblies_init(void);

/* Register an assembly in the catalog.
   Takes ownership of the assembly pointer? No, usually runtime manages it.
   But catalog will index it.
*/
int clr_assemblies_add(il_assembly_t *assembly, const char *name);

/* Lookup by Name (e.g. "System.Runtime") */
il_assembly_t *clr_assemblies_lookup_by_name(const char *name);

/* Lookup by MVID */
il_assembly_t *clr_assemblies_lookup_by_mvid(const uuid_t *mvid);

#endif /* _CLR_ASSEMBLIES_H_ */
