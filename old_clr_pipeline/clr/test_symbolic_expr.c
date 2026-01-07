/*
 * test_symbolic_expr.c - Simple test for symbolic expression system
 */

#include "symbolic_expr.h"
#include "../../port/lib.h"
#include "../../9front-pc64/mem.h"

void test_symbolic_expr(void) {
    print("Testing symbolic expression system...\n");
    
    // Create variables
    sym_expr_t *x = sym_expr_create_var("x");
    sym_expr_t *y = sym_expr_create_var("y");
    
    if (!x || !y) {
        print("Failed to create variables\n");
        return;
    }
    
    print("Created variables: ");
    sym_expr_print(x);
    print(", ");
    sym_expr_print(y);
    print("\n");
    
    // Create expression: x + y * x^2
    sym_expr_t *x_squared = sym_expr_create_binary(SYM_POW, x, sym_expr_create_const(2));
    sym_expr_t *y_times_x_squared = sym_expr_create_binary(SYM_MUL, y, x_squared);
    sym_expr_t *expr = sym_expr_create_binary(SYM_ADD, x, y_times_x_squared);
    
    print("Created expression: ");
    sym_expr_print(expr);
    print("\n");
    
    // Test copying and equality
    sym_expr_t *expr_copy = sym_expr_copy(expr);
    print("Expression equals copy: %d\n", sym_expr_equals(expr, expr_copy));
    
    // Clean up
    sym_expr_free(expr);
    sym_expr_free(expr_copy);
    sym_expr_free(x);
    sym_expr_free(y);
    
    print("Symbolic expression test completed successfully!\n");
}
