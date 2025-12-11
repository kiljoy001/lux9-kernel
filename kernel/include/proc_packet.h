/*
 * Proc-as-Packet Architecture
 *
 * Network packet-style process structure with:
 * - Bit-packed fields
 * - FSM state machine
 * - Checksum validation
 * - State trace history
 */

#ifndef _PROC_PACKET_H_
#define _PROC_PACKET_H_

/* Include Plan 9 types (uchar, ushort, ulong, etc.) */
#include <u.h>

/* Forward declaration - Proc is defined in portdat.h */
struct Proc;
/*
 * Version: Increment when header format changes
 */
#define PROC_PACKET_VERSION 1

/*
 * Process States (4-bit encoded, 0-15)
 */
enum ProcState {
  PS_Dead = 0,
  PS_Moribund,
  PS_New,
  PS_Ready,
  PS_Scheding,
  PS_Running,
  PS_Queueing,
  PS_QueueingR,
  PS_QueueingW,
  PS_Wakeme,
  PS_Broken,
  PS_Stopped,
  PS_Rendezvous,
  PS_Waitrelease,
  PS_Intr,
  PS_IntrReturn,
  PS_COUNT /* Must be <= 16 */
};

/*
 * FSM Events
 */
enum ProcEvent {
  EV_CREATE = 0,  /* Dead -> New */
  EV_READY,       /* New/Wakeme/etc -> Ready */
  EV_SCHEDULE,    /* Ready -> Running */
  EV_YIELD,       /* Running -> Ready */
  EV_SLEEP,       /* Running -> Wakeme */
  EV_WAKEUP,      /* Wakeme -> Ready */
  EV_EXIT,        /* Running -> Moribund */
  EV_REAP,        /* Moribund -> Dead */
  EV_INTERRUPT,   /* Running -> Intr */
  EV_IRETURN,     /* Intr -> IntrReturn */
  EV_RESUME,      /* IntrReturn -> Running */
  EV_QLOCK,       /* Running -> Queueing */
  EV_QUNLOCK,     /* Queueing -> Ready */
  EV_STOP,        /* Running -> Stopped */
  EV_CONT,        /* Stopped -> Ready */
  EV_BREAK,       /* * -> Broken */
  EV_RENDEZ,      /* Running -> Rendezvous */
  EV_RENDEZ_DONE, /* Rendezvous -> Ready */
  EV_COUNT
};

/*
 * Flags Field (16-bit packed)
 *
 * Bit layout:
 *   0: kp (kernel process)
 *   1: insyscall
 *   2: hang
 *   3: trace
 *   4: notepending
 *   5: fixedpri
 *   6: wired
 *   7: privatemem
 *   8: noswap
 *   9: newtlb
 *   10-13: nnote (0-15)
 *   14-15: reserved
 */
#define PF_KP (1 << 0)
#define PF_INSYSCALL (1 << 1)
#define PF_HANG (1 << 2)
#define PF_TRACE (1 << 3)
#define PF_NOTEPENDING (1 << 4)
#define PF_FIXEDPRI (1 << 5)
#define PF_WIRED (1 << 6)
#define PF_PRIVATEMEM (1 << 7)
#define PF_NOSWAP (1 << 8)
#define PF_NEWTLB (1 << 9)
#define PF_NNOTE_MASK (0xF << 10)
#define PF_NNOTE_SHIFT 10

/* Flag accessors */
#define PROC_FLAG_SET(f, b) ((f) |= (b))
#define PROC_FLAG_CLR(f, b) ((f) &= ~(b))
#define PROC_FLAG_TEST(f, b) ((f) & (b))
#define PROC_NNOTE_GET(f) (((f) & PF_NNOTE_MASK) >> PF_NNOTE_SHIFT)
#define PROC_NNOTE_SET(f, n)                                                   \
  ((f) = ((f) & ~PF_NNOTE_MASK) | (((n) & 0xF) << PF_NNOTE_SHIFT))

/*
 * State Trace (16-bit: 4 states x 4 bits each)
 *
 * Layout: [T-3:15-12] [T-2:11-8] [T-1:7-4] [T-0:3-0]
 *         (oldest)                         (current)
 */
#define STATE_CURRENT(t) ((t) & 0xF)
#define STATE_PREV(t) (((t) >> 4) & 0xF)
#define STATE_T2(t) (((t) >> 8) & 0xF)
#define STATE_T3(t) (((t) >> 12) & 0xF)

/* Push new state onto trace (shifts history left) */
#define STATE_PUSH(t, s) (((t) << 4) | ((s) & 0xF))

/*
 * Proc Packet Header (64 bytes, cache-line aligned)
 */
typedef struct ProcHeader {
  /* Byte 0-1: Version and reserved */
  uchar version;   /* PROC_PACKET_VERSION */
  uchar reserved0; /* Must be 0 */

  /* Byte 2-3: Packed flags */
  ushort flags;

  /* Byte 4-5: State trace (last 4 states) */
  ushort state_trace;

  /* Byte 6-7: Header checksum (CRC-16 of bytes 0-5) */
  ushort hdr_checksum;

  /* Byte 8-11: Process ID */
  ulong pid;

  /* Byte 12-15: Parent PID */
  ulong parent_pid;

  /* Byte 16-23: Machine pointer */
  Mach *mach;

  /* Byte 24-39: Scheduler label (SP, PC for context switch) */
  Label sched;

  /* Byte 40-63: Reserved for future use */
  uchar reserved1[24];
} __attribute__((packed, aligned(64))) ProcHeader;

/*
 * State name table (for debugging)
 */
extern char *proc_state_names[PS_COUNT];

/*
 * CRC-16 (CCITT polynomial)
 */
ushort proc_crc16(uchar *data, int len);

/*
 * FSM API
 */

/* Initialize FSM tables (call once at boot) */
void proc_fsm_init(void);

/* Transition process state via event. Returns new state or -1 on error. */
int proc_event(Proc *p, int event);

/* Get current state from trace */
#define proc_state(p) STATE_CURRENT((p)->state_trace)

/* Validate header checksum. Returns 1 if valid, 0 if corrupt. */
int proc_verify(Proc *p);

/* Update header checksum after modifications. */
void proc_seal(Proc *p);

/* Dump state trace for debugging */
void proc_trace_dump(Proc *p);

/*
 * FSM Transition Guard
 * Returns 1 if transition is allowed, 0 if denied.
 * Sets *reason to explanation string on denial.
 */
typedef int (*ProcGuardFn)(Proc *p, const char **reason);

/*
 * FSM Transition Table Entry
 */
typedef struct ProcTransition {
  int from_state;
  int event;
  int to_state;
  ProcGuardFn guard; /* Optional guard function, nil if none */
} ProcTransition;

#endif /* _PROC_PACKET_H_ */
