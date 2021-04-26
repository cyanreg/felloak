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

#include <string.h>
#include "common.h"

static const char *instr_names[] = {
    [FO_INVALID] = "invalid",
    [FO_NOP] = "nop",
    [FO_MOV] = "mov",
    [FO_RET] = "ret",
    [FO_JMP_GT] = "jg",
    [FO_LOAD] = "load",
    [FO_LOAD_V] = "load_v",
    [FO_STORE] = "store",
    [FO_STORE_V] = "store_v",
    [FO_BROADCAST_V] = "broadcast",
    [FO_ADD] = "add",
    [FO_ADD_V] = "add_v",
    [FO_SUB] = "sub",
    [FO_SUB_V] = "sub_v",
    [FO_MUL] = "mul",
    [FO_MUL_V] = "mul_v",
    [FO_SHLL] = "shll",
    [FO_FMADD_V] = "fmadd",
};

const char *fo_instr_type_to_str(FOInstructionType type)
{
    return instr_names[type];
}

FOData *fo_alloc_type_data(void)
{
    return calloc(1, sizeof(FOData));
}

void fo_free_type_data(FOData **data)
{
    if (!data || !*data)
        return;

    FOData *_data = *data;
    free(_data->name);
    free(_data->data);
    *data = NULL;
}

FOFunction *fo_alloc_function(void)
{
    FOFunction *func = calloc(1, sizeof(FOFunction));
    func->active_block = &func->blocks[0];
    func->nb_blocks = 1;
    func->active_block->name = "start";
    return func;
}

FOOperand *fo_fn_find_operand(FOFunction *fn, FOOperand *operand)
{
    FORegisterType m = (FO_SCALAR | FO_VECTOR);

    for (int i = 0; i < fn->nb_operands; i++) {
        if (fn->operands[i]->src_id == operand->src_id &&
            (fn->operands[i]->reg_type & m) == (operand->reg_type & m))
            return fn->operands[i];
    }

    for (int i = 0; i < fn->nb_operands; i++) {
        if (!strcmp(fn->operands[i]->name, operand->name))
            return fn->operands[i];
    }

    return NULL;
}

int fo_fn_add_label(void *src_ctx, FOFunction *fn, char *label)
{
    FOFunctionBlock *next = &fn->blocks[fn->nb_blocks++];
    next->name = label;
    next->src[next->nb_src++] = fn->active_block;

    fn->active_block->dst[fn->active_block->nb_dst++] = next;

    fn->active_block = next;

    fo_log(src_ctx, FO_LOG_VERBOSE, "Switched to new block \"%s\"!\n", label);

    return 0;
}

int fo_fn_add_instr(void *src_ctx, FOIntermediate *inm,
                    FOFunction *fn, FOInstruction *instr)
{
    int j = 0;
    int had_mismatch = 0;
    for (int i = 0; i < instr->nb_operands; i++) {
        if (!instr->map.exp_type[j]) {
            if (had_mismatch)
                fo_log(src_ctx, FO_LOG_ERROR, "Invalid operand!\n");
            else
                fo_log(src_ctx, FO_LOG_ERROR, "Too many operands to instruction!\n");
            return FOERROR(EINVAL);
        }

        int mismatch = 0;
        int is_optional = instr->map.exp_type[i] & FO_OPTIONAL;
        if (!(instr->map.exp_type[j] & instr->operands[i].reg_type))
            mismatch = 1;
        had_mismatch |= 1;

        j++;

        if (mismatch && !is_optional) {
            fo_log(src_ctx, FO_LOG_ERROR, "Invalid operand!\n");
            return FOERROR(EINVAL);
        } else if (mismatch) {
            i--;
            continue;
        }
    }

    for (int i = 0; i < instr->nb_operands; i++) {
        if (!(instr->operands[i].reg_type & (FO_SCALAR | FO_VECTOR)))
            continue;

        FOOperand *operand = fo_fn_find_operand(fn, &instr->operands[i]);
        if (!operand) {
            fn->operands[fn->nb_operands++] = &instr->operands[i];
            instr->operands[i].id = fn->nb_operands - 1;
        } else {
            instr->operands[i].id = operand->id;
        }
    }

    for (int i = 0; i < instr->nb_operands; i++) {
        if (instr->operands[i].reg_type & FO_RODATA) {
            FOData *mapped_rodata = NULL;
            for (j = 0; j < inm->nb_rodata; j++) {
                if (!strcmp(inm->rodata[j]->name, instr->operands[i].name)) {
                    mapped_rodata = inm->rodata[j];
                    break;
                }
            }
            if (!mapped_rodata) {
                fo_log(src_ctx, FO_LOG_ERROR, "rodata entry \"%s\" not found "
                       "in instruction \"%s\"!\n",
                       instr->operands[i].name, instr->map.name);
                return FOERROR(EINVAL);
            }

            instr->operands[i].rodata = mapped_rodata;

            mapped_rodata->refs++;
        }
    }

    if (instr->map.inst == FO_JMP_GT) {
        const char *dst_name = instr->operands[0].name;
        FOFunctionBlock *fb = NULL;
        for (int i = 0; i < fn->nb_blocks; i++) {
            if (!strcmp(fn->blocks[i].name, dst_name)) {
                fb = &fn->blocks[i];
                break;
            }
        }
        if (fb) {
            fo_log(src_ctx, FO_LOG_VERBOSE, "Jump to block \"%s\"%s!\n",
                   dst_name, fb == fn->active_block ? " (loop)" : "");
            instr->operands[0].branch = fb;
            instr->operands[0].branch_loop = fb == fn->active_block;

            int found = 0;
            for (int i = 0; i < fn->active_block->nb_dst; i++) {
                if (fn->active_block->dst[i] == fb) {
                    found = 1;
                    break;
                }
            }
            if (!found)
                fn->active_block->dst[fn->active_block->nb_dst++] = fb;

            if (fb != fn->active_block) {
                found = 0;
                for (int i = 0; i < fb->nb_src; i++) {
                    if (fb->src[i] == fn->active_block) {
                        found = 1;
                        break;
                    }
                }
                if (!found)
                    fb->src[fb->nb_src++] = fn->active_block;
            }
        }
    }

    fn->active_block->instructions = realloc(fn->active_block->instructions,
                                             sizeof(*fn->active_block->instructions) *
                                             fn->active_block->nb_instructions + 1);

    fn->active_block->instructions[fn->active_block->nb_instructions] = instr;

    fn->active_block->nb_instructions++;

    fo_log(src_ctx, FO_LOG_DEBUG, "Adding instruction \"%s\" with %i operands to block \"%s\"!\n",
           fo_instr_type_to_str(instr->map.inst), instr->nb_operands,
           fn->active_block->name);

    return 0;
}

int fo_fn_nb_blocks(FOFunction *fn)
{
    if (!fn)
        return 0;
    return fn->nb_blocks;
}

int fo_fn_nb_instructions(FOFunction *fn)
{
    if (!fn)
        return 0;

    int nb_instr = 0;
    for (int i = 0; i < fn->nb_blocks; i++)
        nb_instr += fn->blocks[i].nb_instructions;
    return nb_instr;
}

void fo_fn_free(FOFunction **func)
{
    if (!func || !*func)
        return;

    FOFunction *_func = *func;
    free(_func->name);
    *func = NULL;
}
