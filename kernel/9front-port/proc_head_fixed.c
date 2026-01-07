#include "../include/u.h"
#include "9p_router.h" /* For P9Control structure */
#include "dat.h"
#include "edf.h"
#include "fns.h"
#include "mem.h"
#include "portlib.h"
#include "../wasm/wasm_runtime.h"
#include "proc_packet.h" /* For EV_* events */
#include "tos.h"
#include "ureg.h"
#include <error.h>
#include <trace.h>

/* FSM Integration */
extern int proc_event(Proc *p, int event);
extern void vault_cleanup_process(int pid);

enum {
  Scaling = 2,
};
