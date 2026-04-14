#pragma once
#include "u.h"

/*
 * The Ring contains Page Handles.
 * The Pages contain Batches of Messages.
 */

#define RING_SIZE 128
#define RING_MASK 127

/* The Ring Structure (Shared Memory) */
typedef struct IpcPageRing {
  volatile u32int head; /* Kernel Read Index */
  volatile u32int tail; /* User Write Index */
  u32int mask;          /* Size - 1 */
  u32int flags;

  /* The Ring Data: Handles to Pages (User Virtual Addresses) */
  u64int pages[RING_SIZE];
} IpcPageRing;

/* The Channel Control Page */
typedef struct IpcChannel {
  u32int magic; /* 0x52494E47 "RING" */
  u32int status;

  /* Two Rings: Submission (User->Kern), Completion (Kern->User) */
  IpcPageRing submission;
  IpcPageRing completion;
} IpcChannel;

/* The Batch Page Layout (Inside the 4KB Page) */
#define BATCH_PAGE_MAGIC 0xB47C4831 /* "BTCH1" */

typedef struct BatchHeader {
  u16int num_messages; /* Count of messages in this batch */
  u16int used_bytes;   /* Byte offset to free space/end of data */
  u32int magic;        /* Verification Magic */
  u64int nonce;        /* Security Nonce (Two-Factor Auth) */
  uuid_t uuids[120];   /* Per-message caller identity */

  /* Data follows immediately after this header */
  /* Layout: [Len u16][Msg Body...] [Len u16][Msg Body...] */
} BatchHeader;

/* Helper to calculate data start */
#define BATCH_DATA_START sizeof(BatchHeader)
#define BATCH_CAPACITY (4096 - sizeof(BatchHeader))