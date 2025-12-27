#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../include/execution_engine.h"

vm_init_t mock_init = {0};

int main() {
    printf("Testing Symbolic Differentiation...\n");
    vm_execution_state_t* state = vm_create_execution_state(&mock_init);
    
    // 1. Create Variable "x"
    char* name_x = strdup("x");
    vm_value_t val_x_name = vm_make_ref(name_x);
    vm_value_t val_x;
    if (!vm_symbolic_create(&val_x_name, &val_x)) {
        printf("Failed to create symbolic variable\n");
        return 1;
    }
    
    // 2. Create Constant 5
    sym_expr_t* const_5 = malloc(sizeof(sym_expr_t));
    memset(const_5, 0, sizeof(sym_expr_t));
    const_5->type = SYM_CONST;
    const_5->data.value = 5;
    vm_value_t val_5 = vm_make_ref(const_5);
    
    // 3. Create "x + 5"
    vm_value_t val_expr;
    if (!vm_symbolic_add(&val_x, &val_5, &val_expr)) {
        printf("Failed to create symbolic expression\n");
        return 1;
    }
    
    // 4. Differentiate d/dx (x+5) -> 1
    vm_value_t val_diff;
    if (!vm_symbolic_differentiate(&val_expr, &val_x, &val_diff)) {
        printf("Failed to differentiate\n");
        return 1;
    }
    
    sym_expr_t* res = (sym_expr_t*)val_diff.value.ref;
    if (res->type != SYM_ADD) {
        printf("Result root type mismatch: expected ADD\n");
        return 1;
    }
    
    sym_expr_t* left = res->data.binary.left;
    sym_expr_t* right = res->data.binary.right;
    
    // d(x)/dx = 1
    if (left->type != SYM_CONST || left->data.value != 1) {
        printf("Left term error: expected 1\n");
        return 1;
    }
    
    // d(5)/dx = 0
    if (right->type != SYM_CONST || right->data.value != 0) {
        printf("Right term error: expected 0\n");
        return 1;
    }
    
    printf("PASS: d/dx(x+5) = 1 + 0\n");
    return 0;
}
