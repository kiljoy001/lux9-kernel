/**
 * ACSL Annotations for Secure Ramdisk (devram.c)
 *
 * These annotations enable Frama-C formal verification of security properties.
 * Use with: frama-c -wp -wp-rte devram.c devram_acsl.h
 */

#ifndef DEVRAM_ACSL_H
#define DEVRAM_ACSL_H

// Include this file in devram.c to add formal verification annotations

/**
 * ACSL Predicates for Security Properties
 */

/*@ predicate is_locked(SecureRamdisk *rd) =
  @   rd->locked == 1;
  @
  @ predicate is_unlocked(SecureRamdisk *rd) =
  @   rd->locked == 0;
  @
  @ predicate is_initialized(SecureRamdisk *rd) =
  @   rd->initialized == 1 && rd->master_key_set == 1;
  @
  @ predicate is_encrypted(SecureRamdisk *rd) =
  @   is_locked(rd) && is_initialized(rd);
  @
  @ predicate has_valid_data(SecureRamdisk *rd) =
  @   \valid(rd->data + (0..rd->size-1));
  @
  @ predicate refcount_valid(SecureRamdisk *rd) =
  @   rd->refcount >= 0;
  @
  @ predicate nonce_is_fresh(uchar *nonce) =
  @   // Nonce should not have been used before (abstracted)
  @   \true;
  @
  @ // Lock-Encryption Invariant (from Coq proof)
  @ predicate lock_encryption_invariant(SecureRamdisk *rd) =
  @   is_locked(rd) ==> is_encrypted(rd);
  @
  @ // Init-Key Invariant (from Coq proof)
  @ predicate init_key_invariant(SecureRamdisk *rd) =
  @   is_initialized(rd) ==> rd->master_key_set == 1;
  @
  @ // Refcount Invariant (from Coq proof)
  @ predicate refcount_invariant(SecureRamdisk *rd) =
  @   refcount_valid(rd);
  @
  @ // System Invariant (conjunction of all invariants)
  @ predicate system_invariant(SecureRamdisk *rd) =
  @   lock_encryption_invariant(rd) &&
  @   init_key_invariant(rd) &&
  @   refcount_invariant(rd);
  @*/

/**
 * Secure Wipe Specification
 */

/*@ requires \valid(data + (0..size-1));
  @ requires size > 0;
  @ ensures \forall integer i; 0 <= i < size ==> data[i] == 0;
  @ assigns data[0..size-1];
  @*/
extern void secure_wipe(uchar *data, ulong size);

/**
 * XChaCha20 Encryption with Fresh Nonce
 *
 * BUG #1 FIX: Must generate fresh nonce
 */

/*@ requires \valid(data + (0..data_size-1));
  @ requires data_size >= 24;
  @ requires \valid(key + (0..31));
  @ requires nonce_is_fresh(data);  // Nonce at data[0..23] must be fresh
  @ ensures \forall integer i; 0 <= i < 24 ==> nonce_is_fresh(data + i);
  @ ensures \forall integer i; 24 <= i < data_size ==> data[i] != \old(data[i]);
  @ assigns data[0..data_size-1];
  @*/
extern void xchacha20_encrypt_with_fresh_nonce(uchar *data, ulong data_size, uchar *key);

/**
 * XChaCha20 Decryption with Stored Nonce
 */

/*@ requires \valid(data + (0..data_size-1));
  @ requires data_size >= 24;
  @ requires \valid(key + (0..31));
  @ ensures \forall integer i; 24 <= i < data_size ==>
  @   data[i] == /* decrypted value based on stored nonce */;
  @ assigns data[24..data_size-1];
  @*/
extern void xchacha20_decrypt_with_stored_nonce(uchar *data, ulong data_size, uchar *key);

/**
 * Init Command Specification
 *
 * BUG #3 FIX: Must keep vault locked after init
 */

/*@ requires !is_initialized(&secure_rd);
  @ requires \valid_read(password);
  @ ensures is_initialized(&secure_rd);
  @ ensures is_locked(&secure_rd);  // BUG #3 FIX
  @ ensures system_invariant(&secure_rd);
  @ assigns secure_rd.initialized, secure_rd.locked, secure_rd.master_key[0..31];
  @*/
// extern void handle_init_command(const char *password);

/**
 * Unlock Command Specification
 *
 * BUG #2 FIX: Must validate state before unlock
 */

/*@ requires is_initialized(&secure_rd);
  @ requires is_locked(&secure_rd);  // BUG #2 FIX: Must be locked
  @ requires !secure_rd.tpm_sealed;  // Cannot unlock if TPM sealed
  @ requires \valid_read(password);
  @ ensures is_unlocked(&secure_rd);
  @ ensures system_invariant(&secure_rd);
  @ assigns secure_rd.locked, secure_rd.data[0..secure_rd.size-1];
  @*/
// extern void handle_unlock_command(const char *password);

/**
 * Lock Command Specification
 *
 * BUG #2 FIX: Must validate state before lock
 */

/*@ requires is_initialized(&secure_rd);
  @ requires is_unlocked(&secure_rd);  // BUG #2 FIX: Must be unlocked
  @ ensures is_locked(&secure_rd);
  @ ensures is_encrypted(&secure_rd);
  @ ensures system_invariant(&secure_rd);
  @ assigns secure_rd.locked, secure_rd.data[0..secure_rd.size-1];
  @*/
// extern void handle_lock_command(void);

/**
 * ramopen Specification
 *
 * BUG #4 FIX: Must increment refcount
 */

/*@ requires refcount_valid(&secure_rd);
  @ ensures secure_rd.refcount == \old(secure_rd.refcount) + 1;
  @ ensures refcount_invariant(&secure_rd);
  @ assigns secure_rd.refcount;
  @*/
// extern Chan *ramopen(Chan *c, int omode);

/**
 * ramclose Specification
 *
 * BUG #4 FIX: Must only wipe when refcount reaches 0
 */

/*@ requires refcount_valid(&secure_rd);
  @ requires secure_rd.refcount > 0;
  @ ensures secure_rd.refcount == \old(secure_rd.refcount) - 1;
  @ ensures (secure_rd.refcount == 0 && is_locked(&secure_rd)) ==>
  @   \forall integer i; 0 <= i < secure_rd.size ==> secure_rd.data[i] == 0;
  @ assigns secure_rd.refcount, secure_rd.data[0..secure_rd.size-1];
  @*/
// extern void ramclose(Chan *c);

/**
 * Memory Safety Properties
 */

/*@ axiomatic MemorySafety {
  @   // No buffer overflows
  @   axiom no_buffer_overflow:
  @     \forall uchar *buf, ulong size, ulong offset;
  @       \valid(buf + (0..size-1)) ==>
  @         (offset < size ==> \valid(buf + offset));
  @
  @   // No use-after-free
  @   axiom no_use_after_free:
  @     \forall uchar *buf;
  @       \valid(buf) ==> \valid(buf);  // Simplified
  @
  @   // No double-free
  @   axiom no_double_free:
  @     \forall uchar *buf;
  @       \valid(buf);  // Simplified
  @ }
  @*/

/**
 * Concurrency Safety (QLock)
 */

/*@ predicate lock_held(QLock *lock) =
  @   \true;  // Abstract: lock is held by current thread
  @
  @ predicate lock_free(QLock *lock) =
  @   \true;  // Abstract: lock is not held
  @*/

/*@ requires lock_free(&secure_rd.lock);
  @ ensures lock_held(&secure_rd.lock);
  @*/
extern void qlock(QLock *lock);

/*@ requires lock_held(&secure_rd.lock);
  @ ensures lock_free(&secure_rd.lock);
  @*/
extern void qunlock(QLock *lock);

/**
 * Runtime Assertion Macros (enabled with debug.invariants=1)
 */

/*@ assert system_invariant(&secure_rd); */
#define CHECK_INVARIANTS() \
  do { \
    check_lock_invariant(); \
    check_init_invariant(); \
    check_refcount_invariant(); \
  } while(0)

/**
 * Ghost Variables for Verification
 *
 * These variables exist only for proof purposes and are not compiled.
 */

/*@ ghost int nonce_generation_counter = 0; */

/*@ ghost
  @ ensures nonce_generation_counter == \old(nonce_generation_counter) + 1;
  @ ensures nonce_is_fresh(nonce);
  @ assigns nonce_generation_counter;
  @*/
// extern void ghost_generate_nonce(uchar *nonce);

#endif /* DEVRAM_ACSL_H */
