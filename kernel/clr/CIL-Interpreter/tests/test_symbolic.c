#include "../include/execution_engine.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Mock VM initialization
vm_init_t mock_init = {0};

int main() {
  printf("Running test_symbolic...\n");

  vm_execution_state_t *state = vm_create_execution_state(&mock_init);
  if (!state) {
    printf("Failed to create VM state\n");
    return 1;
  }

  // 1. Create Variable "x"
  // The interpreter expects a string reference on the stack for SYM_CREATE
  char *x_str = strdup("x");
  vm_value_t val_x_name = vm_make_ref(x_str);
  vm_value_t val_x;

  if (!vm_symbolic_create(&val_x_name, &val_x)) {
    printf("vm_symbolic_create failed\n");
    return 1;
  }

  sym_expr_t *expr_x = (sym_expr_t *)val_x.value.ref;
  if (expr_x->type != SYM_VAR || strcmp(expr_x->data.name, "x") != 0) {
    printf("FAILED: Expected SYM_VAR 'x'\n");
    return 1;
  }
  printf("PASS: Created variable 'x'\n");

  // 2. Create Constant 5
  // Manually construct since we don't have vm_symbolic_const yet
  sym_expr_t *const_5 = malloc(sizeof(sym_expr_t));
  const_5->type = SYM_CONST;
  const_5->data.value = 5;
  vm_value_t val_5 = vm_make_ref(const_5);

  // 3. Create Expression "x + 5" using vm_symbolic_expr (assuming ADD behavior)
  vm_value_t val_add;
  if (!vm_symbolic_expr(&val_x, &val_5, &val_add)) {
    printf("vm_symbolic_expr failed\n");
    return 1;
  }

  sym_expr_t *expr_add = (sym_expr_t *)val_add.value.ref;
  if (expr_add->type != SYM_ADD) {
    printf("FAILED: Expected SYM_ADD\n");
    return 1;
  }
  printf("PASS: Created expression 'x + 5'\n");

  // 4. Differentiate d/dx (x + 5) -> 1 + 0 -> 1 (simplified)
  // We differentiate 'val_add' with respect to 'val_x'
  vm_value_t val_diff;
  if (!vm_symbolic_differentiate(&val_add, &val_x, &val_diff)) {
    printf("vm_symbolic_differentiate failed\n");
    return 1;
  }

  sym_expr_t *expr_diff = (sym_expr_t *)val_diff.value.ref;
  // Expectation: result is a structure representing 1 + 0, or just 1 if
  // simplified. For this first pass, getting a tree of (1 + 0) is acceptable.
  // SYM_ADD(SYM_CONST(1), SYM_CONST(0)))

  if (expr_diff->type != SYM_ADD) {
    printf(
        "FAILED: Derivative should be an ADD expression (d/dx(x) + d/dx(5))\n");
    return 1;
  }

  sym_expr_t *left = expr_diff->data.binary.left;
  sym_expr_t *right = expr_diff->data.binary.right;

  if (left->type != SYM_CONST || left->data.value != 1) {
    printf("FAILED: d/dx(x) should be 1\n");
    return 1;
  }

  if (right->type != SYM_CONST || right->data.value != 0) {
    printf("FAILED: d/dx(5) should be 0\n");
    return 1;
  }

  printf("PASS: Differentiation correct (1 + 0)\n");

  vm_destroy_execution_state(state);
  free(x_str);
  // Cleanup of expression tree omitted for brevity in test

  return 0;
}
