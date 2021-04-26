/*
 * This file is part of felloak.
 *
 * felloak is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * felloak is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with felloak; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#pragma once
#include <stdint.h>
#include <stdlib.h>
#include "logging.h"
#include "input_output.h"

typedef enum FOType {
    FO_NONE = 0,

    FO_U8,
    FO_S8,
    FO_U16,
    FO_S16,
    FO_U32,
    FO_S32,
    FO_U64,
    FO_S64,

    FO_FLT,
    FO_DBL,

    FO_PTR,
    FO_GENERIC,

    FO_CONST = (1 << 31), /* Flag */
} FOType;

typedef enum FORegisterType {
    FO_SCALAR    = (1 <<  0),
    FO_VECTOR    = (1 <<  1),
    FO_MEMORY    = (1 <<  2),
    FO_IMMEDIATE = (1 <<  3),
    FO_LABEL     = (1 <<  4),
    FO_LIST      = (1 <<  5),
    FO_RODATA    = (1 <<  6),

    FO_OPTIONAL  = (1 << 31),
} FORegisterType;

typedef enum FOInstructionType {
    FO_INVALID = 0,

    FO_NOP,

    FO_MOV,

    FO_RET,

    FO_JMP_GT,

    FO_LOAD,
    FO_LOAD_V,

    FO_STORE,
    FO_STORE_V,

    FO_BROADCAST_V,

    FO_ADD,
    FO_ADD_V,

    FO_SUB,
    FO_SUB_V,

    FO_MUL,
    FO_MUL_V,

    FO_SHLL,

    FO_FMADD_V,
} FOInstructionType;

typedef struct FOInstructionMap {
    FOInstructionType inst;
    FOType ele_type;
    const char *name;
    FORegisterType exp_type[6];
    int fused_next; /* Hint, indicates next instruction was fused with this one */
} FOInstructionMap;

typedef struct FOData {
    char *name; /* Name */
    uint8_t *data; /* Data */
    size_t size; /* Size in bytes */
    int global; /* If global or not */
    int is_const; /* If constant or not */
    int alignment; /* Alignment, 0 for undefined */
    int refs; /* How many times its referenced */
} FOData;

typedef struct FOOperand {
    FOType ele_type;
    FORegisterType reg_type;
    int elements;
    int id;
    int src_id;
    uint32_t vextr_indices; /* Bitmask of indices to extract */
    const char *name;

    FOData *rodata;
    int64_t immediate_val;
    struct FOFunctionBlock *branch;
    int branch_loop;
} FOOperand;

typedef struct FOInstruction {
    FOInstructionMap map;
    FOOperand operands[16];
    int nb_operands;
} FOInstruction;

typedef struct FOFunctionBlock {
    char *name;

    /* Source points - cannot be itself! Can be null once at the start! */
    struct FOFunctionBlock *src[64];
    int nb_src;

    /* Destination points - can be itself if this is a loop */
    struct FOFunctionBlock *dst[64];
    int nb_dst;

    FOInstruction **instructions;
    int nb_instructions;
} FOFunctionBlock;

typedef struct FOFunction {
    char *name;
    int is_global;

    FOType in_args[1024];
    int nb_in_args;

    FOType out_args[1024];
    int nb_out_args;

    FOFunctionBlock *active_block;

    FOOperand *operands[1024];
    int nb_operands;

    FOFunctionBlock blocks[1024];
    int nb_blocks;
} FOFunction;

typedef struct FOIntermediate {
    char *name; /* Usually the file name */

    FOData *rodata[128];
    int nb_rodata;

    FOFunction *functions[128];
    int nb_functions;
} FOIntermediate;

/* Data */
FOData *fo_alloc_type_data(void);
void fo_free_type_data(FOData **data);

/* Functions */
FOFunction *fo_alloc_function(void);

int fo_fn_add_label(void *src_ctx, FOFunction *fn, char *label);
int fo_fn_add_instr(void *src_ctx, FOIntermediate *inm,
                    FOFunction *fn, FOInstruction *instr);

FOOperand *fo_fn_find_operand(FOFunction *fn, FOOperand *operand);
int fo_fn_nb_blocks(FOFunction *fn);
int fo_fn_nb_instructions(FOFunction *fn);

void fo_fn_free(FOFunction **func);

const char *fo_instr_type_to_str(FOInstructionType type);

static inline int fo_type_to_bytes(FOType type)
{
    switch (type) {
    case FO_U8:
    case FO_S8:
        return 1;
    case FO_U16:
    case FO_S16:
        return 2;
    case FO_U32:
    case FO_S32:
    case FO_FLT:
        return 4;
    case FO_U64:
    case FO_S64:
    case FO_DBL:
        return 8;
    default:
        return INT32_MAX;
    }
}
