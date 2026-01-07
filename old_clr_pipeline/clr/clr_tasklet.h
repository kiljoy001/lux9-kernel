/*
 * CLR Tasklet System Header - Memory-Efficient Pooled Design
 *
 * Tasklets are stackless cooperative threads packed into exchange pages.
 * 16 tasklet slots (256B each) fit in one 4KB exchange page.
 */

#ifndef CLR_TASKLET_H
#define CLR_TASKLET_H

/* Tasklet states */
enum {
  TASKLET_FREE = 0,
  TASKLET_CREATED,
  TASKLET_READY,
  TASKLET_RUNNING,
  TASKLET_BLOCKED,
  TASKLET_DONE
};

/* Tasklet flags */
enum {
  TASKLET_FLAG_OVERFLOW = 1, /* Has overflow heap */
};

#define TASKLET_MAGIC 0x544C4554 /* "TLET" */
#define TASKLET_SLOTS_PER_PAGE 16
#define TASKLET_INLINE_LOCALS 8
#define TASKLET_INLINE_STACK 16

/*
 * TaskletSlot - 256 bytes, packed into exchange pages
 * 16 slots per 4KB page
 *
 * Note: Types like ulong, uintptr, uchar are defined in the .c file
 * via kernel includes. This header just declares the structures.
 */
typedef struct TaskletSlot TaskletSlot;
struct TaskletSlot {
  unsigned int magic;                             /*   4B */
  unsigned int id;                                /*   4B */
  unsigned short state;                           /*   2B */
  unsigned short priority;                        /*   2B */
  unsigned int assembly_id;                       /*   4B */
  unsigned int method_token;                      /*   4B */
  unsigned short ip;                              /*   2B */
  unsigned short stack_top;                       /*   2B */
  unsigned short locals_count;                    /*   2B */
  unsigned short flags;                           /*   2B */
  unsigned long locals[TASKLET_INLINE_LOCALS];    /*  64B */
  unsigned long eval_stack[TASKLET_INLINE_STACK]; /* 128B */
  void *overflow_heap;                            /*   8B */
  TaskletSlot *next;                              /*   8B */
  void *blocked_on;                               /*   8B */
  unsigned int pool_index;                        /*   4B */
  unsigned int slot_index;                        /*   4B */
  unsigned char reserved[8];                      /*   8B */
}; /* Total: 256 bytes */

/*
 * TaskletPool - One exchange page with 16 tasklet slots
 */
typedef struct TaskletPool TaskletPool;
struct TaskletPool {
  void *exchange;                 /* ExchangeHandle - opaque */
  void *cap;                      /* UserCapability - opaque */
  TaskletSlot *slots;             /* Pointer to mapped page */
  volatile unsigned int freemask; /* Bitmap: 1=free, 0=used */
  unsigned int pool_id;
  void *lock;        /* Lock - opaque */
  TaskletPool *next; /* Chain of pools */
};

/* Tasklet API */
void clr_tasklet_init(void);
TaskletSlot *clr_tasklet_create(unsigned long assembly_id,
                                unsigned long method_token, int priority);
void clr_tasklet_ready(TaskletSlot *t);
void clr_tasklet_yield(void);
void clr_tasklet_block(void *channel);
void clr_tasklet_resume(TaskletSlot *t);
void clr_tasklet_schedule(void);
TaskletSlot *clr_tasklet_current(void);
void clr_tasklet_destroy(TaskletSlot *t);

/* Stack operations */
int clr_tasklet_push(TaskletSlot *t, unsigned long value);
unsigned long clr_tasklet_pop(TaskletSlot *t);

/* Local variable operations */
int clr_tasklet_setlocal(TaskletSlot *t, int index, unsigned long value);
unsigned long clr_tasklet_getlocal(TaskletSlot *t, int index);

/* Statistics */
void clr_tasklet_stats(unsigned long *total, unsigned long *ready,
                       unsigned long *running, unsigned long *pools);

#endif /* CLR_TASKLET_H */
