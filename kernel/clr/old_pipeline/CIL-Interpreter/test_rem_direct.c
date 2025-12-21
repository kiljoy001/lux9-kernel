#include <stdio.h>
#include <stdlib.h>
#include "include/il_decoder.h"

int main() {
    // Test REM opcode decoding
    uint8_t bytecode[] = {0x5D}; // REM opcode
    
    cil_decoder_t* decoder = create_cil_decoder(bytecode, sizeof(bytecode));
    cil_instruction_t* inst = decode_next_instruction(decoder);
    
    if (inst) {
        printf("REM opcode decoded successfully:\n");
        printf("  Opcode: 0x%02X\n", inst->opcode);
        printf("  Name: %s\n", get_opcode_name(inst->opcode));
        printf("  Operand type: %d\n", inst->operand_type);
        printf("  Size: %d\n", inst->size);
        free(inst);
    } else {
        printf("REM opcode decoding failed!\n");
    }
    
    destroy_cil_decoder(decoder);
    return 0;
}
