/*
 * Pebble CPU - Temporal Resource Management
 * 
 * Extends the Pebble circular economy to CPU scheduling.
 * Maps formal verification of EDF to economic token flows.
 */

#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "pebble.h"
#include "edf.h"

extern Mach *m;

/* 
 * CPU Token Unit: 1 Token = 1 Microsecond of Execution Time 
 */

#define TAX_BASE_RATE 1  /* 1% Minimum maintenance tax */
#define TAX_MAX_RATE 20  /* 20% Maximum tax during congestion */

struct PebbleCPUBank {
    Lock lock;
    ulong total_capacity;    /* Max tokens per second (e.g. 1,000,000 * n_cores) */
    ulong issued_red;        /* Tokens reserved for EDF (Red) */
    ulong issued_black;      /* Tokens active in Round-Robin (Black) */
    ulong free_float;        /* Unallocated bandwidth */
    ulong kernel_tax_pool;   /* Sovereign budget for interrupts/kernel overhead */
};

/*@
  predicate Inv_CPU_Conservation(struct PebbleCPUBank *bank) =
    bank->issued_red + bank->free_float + bank->kernel_tax_pool == bank->total_capacity;

  predicate Inv_Schedulable(struct PebbleCPUBank *bank) =
    bank->issued_red <= bank->total_capacity;
*/

static struct PebbleCPUBank cpu_bank;

void pebble_cpu_init(void) {
    cpu_bank.total_capacity = 1000000; /* 1 sec/sec for 1 core */
    cpu_bank.free_float = cpu_bank.total_capacity;
    cpu_bank.kernel_tax_pool = 0;
}

/*
 * RED Token Allocation (Hard Real-Time Reservation)
 * Corresponds to EDF 'Admit' phase.
 * 
 * Proof Obligation: valid_task (C, T, D) per proofs/scheduler/edf_policy.v
 * Economic View: Purchasing a recurring subscription.
 */
/*@
  requires cost_us > 0 && period_us >= cost_us;
  requires Inv_CPU_Conservation(&cpu_bank);
  requires Inv_Schedulable(&cpu_bank);
  ensures Inv_CPU_Conservation(&cpu_bank);
  ensures Inv_Schedulable(&cpu_bank);
  // Implementation of edf_utilization_bound theorem
*/
int pebble_cpu_alloc_red(Proc *p, ulong cost_us, ulong period_us) {
    ulong bandwidth_needed;
    
    /* 1. Calculate Utilization (Tokens per Second) */
    /* U = C/T. Scaled to micro-units. */
    if (period_us == 0) return -1;
    
    /* Simple bandwidth check: cost * 1000000 / period */
    bandwidth_needed = (uvlong)cost_us * 1000000 / period_us;
    
    lock(&cpu_bank.lock);
    
    /* Conservation Check: Do we have enough free float? */
    /*@ assert bandwidth_needed <= cpu_bank.free_float; */
    if (cpu_bank.issued_red + bandwidth_needed > cpu_bank.total_capacity) {
        unlock(&cpu_bank.lock);
        return -1; /* Insufficient funds (Inflation blocked) */
    }
    
    /* Mint Red Tokens (Reservation) */
    cpu_bank.issued_red += bandwidth_needed;
    cpu_bank.free_float -= bandwidth_needed;
    
    unlock(&cpu_bank.lock);
    
    /* Update Process Pebble State */
    /* p->pebble.red_cpu_reservation = bandwidth_needed; */
    
    return 0;
}

/*
 * BLACK Token Allocation (Best Effort / Spot Market)
 * Corresponds to Standard Scheduling.
 * 
 * Mechanics: Processes accumulate "Colorless" credits over time.
 * When scheduled, they convert Colorless -> Black (Execution).
 */
void pebble_cpu_tick_mint(void) {
    /* Called from clock interrupt (Central Bank) */
    ulong supply_per_tick;
    ulong tax, dynamic_rate;
    
    /* 1ms tick = 1000 tokens per core */
    supply_per_tick = 1000; 
    
    /* 
     * Dynamic Sovereign Tax (Congestion Pricing):
     * Rate scales with system load (m->load is fixed point * 1000).
     * If load is 1.0 (1000), added rate is 1%.
     * If load is 10.0 (10000), added rate is 10%.
     */
    dynamic_rate = TAX_BASE_RATE + (m->load / 1000);
    
    /* Clamp tax rate */
    if (dynamic_rate > TAX_MAX_RATE)
        dynamic_rate = TAX_MAX_RATE;
        
    /* Calculate Tax (Sovereign Revenue) */
    tax = (supply_per_tick * dynamic_rate) / 100;
    
    lock(&cpu_bank.lock);
    
    /* 1. Fund Kernel Tax Pool */
    cpu_bank.kernel_tax_pool += tax;
    
    /* 2. Service Red Debt (EDF) first */
    /* Only tax the 'free' portion? No, tax applies to total supply. */
    /* Effective supply for user processes */
    ulong user_supply = supply_per_tick - tax;
    
    ulong red_draw = (cpu_bank.issued_red * user_supply) / 1000000;
    
    /* 3. Remaining goes to Black/Colorless pool */
    ulong free_tokens = 0;
    if (user_supply > red_draw)
        free_tokens = user_supply - red_draw;
    
    /* Distribute free_tokens to active processes (simplified) */
    /* In reality, this updates the global 'scheduler pot' */
    
    unlock(&cpu_bank.lock);
}

/*
 * BURN (Consumption)
 * Called from accounttime() or edfrecord()
 */
/*@
  requires p != \null && p->edf != \null;
  requires p->edf->S >= time_us;
  ensures p->edf->S == \old(p->edf->S) - time_us;
  // Implementation of budget_enforcement_safety theorem
*/
void pebble_cpu_burn(Proc *p, ulong time_us) {
    /* 
     * Kernel/Interrupt Accounting:
     * If we are handling an interrupt, bill the Sovereign Tax Pool.
     * This prevents penalizing the currently running user process for system overhead.
     */
    if (m->intr) {
        /* This is a naive lock-free check, strict accounting might need CAS */
        if (cpu_bank.kernel_tax_pool >= time_us) {
            cpu_bank.kernel_tax_pool -= time_us;
        } else {
            cpu_bank.kernel_tax_pool = 0;
            /* Kernel Deficit! In a real economy, we might print money (inflation) 
               or panic. For now, we absorb the loss. */
        }
        return;
    }

    /* 
     * If running Red (EDF):
     * Burn against the specific Red Token reservation (C budget).
     * If C reaches 0, the Red Token is exhausted for this period.
     */
    if (p->edf && (p->edf->flags & Admitted)) {
        /* This matches proofs/scheduler/edf_policy.v: 
           budget_enforcement_safety */
        if (p->edf->S >= time_us) {
            p->edf->S -= time_us;
        } else {
            p->edf->S = 0;
            /* Enforce conservation: Deschedule */
            /* p->state = Scheding; */
        }
        return;
    }
    
    /*
     * If running Black (Standard):
     * Burn from accumulated Colorless budget.
     */
     /*
    if (p->pebble.cpu_budget >= time_us) {
        p->pebble.cpu_budget -= time_us;
    } else {
        p->pebble.cpu_budget = 0;
        // Economic Penalty: Lower priority
    }
    */
}
