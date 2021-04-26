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

#include "output_wasm.h"
#include "../config.h"
#include "version.h"

static const char *get_instruction(FOInstruction *instr)
{
    switch (instr->map.inst) {
    case FO_STORE_V:
        return "v128.store";
    case FO_LOAD_V:
        return "v128.load";
    case FO_MUL_V:
        switch (instr->map.ele_type) {
        case FO_FLT: return "f32x4.mul";
        case FO_S32: return "i32x4.mul";
        default: return NULL;
        };
    case FO_SUB_V:
        switch (instr->map.ele_type) {
        case FO_FLT: return "f32x4.sub";
        case FO_S32: return "i32x4.sub";
        default: return NULL;
        };
    case FO_ADD_V:
        switch (instr->map.ele_type) {
        case FO_FLT: return "f32x4.add";
        case FO_S32: return "i32x4.add";
        default: return NULL;
        };
    case FO_BROADCAST_V:
        switch (instr->map.ele_type) {
        case FO_FLT: return "f32x4.splat";
        case FO_S32: return "i32x4.splat";
        default: return NULL;
        };
    default:
        return NULL;
    };
}

static const char *ele_size_to_str(FOType type)
{
    switch (type) {
    case FO_U8:
    case FO_S8:
        return "i8";
    default:
        return "i32";
    }
}

static int write_block(FOWasmContext *ctx, FOIntermediate *inm,
                       FOOutputContext *out, FOFunction *fn,
                       FOFunctionBlock *block)
{
    for (int i = 0; i < block->nb_instructions; i++) {
        FOInstruction *instr = block->instructions[i];

        const char *fn_name = get_instruction(instr);
        if (!fn_name)
            continue;

        fprintf(out->stream, "    %s%s", fn_name, instr->nb_operands > 0 ? " " : "");
        for (int j = 0; j < instr->nb_operands; j++) {
            fprintf(out->stream, "(local.get %i)%s", instr->operands[j].id,
                    j != (instr->nb_operands - 1) ? " " : "");
        }
        fprintf(out->stream, "\n");
    }

    return 0;
}

int fo_wasm_output_from_ir(FOWasmContext *ctx, FOIntermediate *inm, FOOutputContext *out)
{
    int err;

    fo_class_alloc(ctx, "wasm_output", FO_CLASS_TYPE_OUTPUT, out);

    fprintf(out->stream, "    .text\n");
    fprintf(out->stream, "    .file \"%s\"\n", out->dst);

    for (int i = 0; i < inm->nb_functions; i++) {
        FOFunction *fn = inm->functions[i];
        fo_log(ctx, FO_LOG_VERBOSE, "Writing function \"%s\"\n", fn->name);

        /* Header */
        fprintf(out->stream, "    .section .text.%s,"",@\n", fn->name);
        fprintf(out->stream, "    .hidden %s\n", fn->name);
        fprintf(out->stream, "    .globl %s\n", fn->name);
        fprintf(out->stream, "    .type %s,@function\n", fn->name);
        fprintf(out->stream, "%s:\n", fn->name);

        /* Arguments and return results */
        fprintf(out->stream, "    .functype (");
        for (int j = 0; j < fn->nb_in_args; j++)
            fprintf(out->stream, "%s%s", ele_size_to_str(fn->in_args[i]),
                    j != (fn->nb_in_args - 1) ? ", " : "");
        fprintf(out->stream, ") -> (");
        for (int j = 0; j < fn->nb_out_args; j++)
            fprintf(out->stream, "%s%s", ele_size_to_str(fn->out_args[i]),
                    j != (fn->nb_out_args - 1) ? ", " : "");
        fprintf(out->stream, ")\n");

        /* Stack? Or registers? Not sure. */
        fprintf(out->stream, "    .local");
        for (int j = 0; j < fn->nb_operands; j++)
            fprintf(out->stream, "%s%s", "i32",
                    i != (fn->nb_operands - 1) ? ", " : "");
        fprintf(out->stream, "\n");

        for (int j = 0; j < fn->nb_blocks; j++) {
            FOFunctionBlock *block = &fn->blocks[i];
            if ((err = write_block(ctx, inm, out, fn, block)) < 0)
                return err;
        }
    }

    for (int i = 0; i < inm->nb_rodata; i++) {
        FOData *rodata = inm->rodata[i];
        fo_log(ctx, FO_LOG_VERBOSE, "Writing rodata entry \"%s\" (%li bytes)\n",
               rodata->name, rodata->size);

        fprintf(out->stream, "    .type %s,@object\n", rodata->name);
        fprintf(out->stream, "    .section .rodata.%s,"",@\n", rodata->name);
        fprintf(out->stream, "    .p2align 4\n");

        fprintf(out->stream, "%s:\n", rodata->name);
        for (int j = 0; j < rodata->size; j++) {
            fprintf(out->stream, "    .int8 %i\n", *((int8_t *)(rodata->data + j)));
        }
        fprintf(out->stream, ".size %s, %li\n", rodata->name, rodata->size);
        fprintf(out->stream, "\n");
    }

    fprintf(out->stream, "    .ident \"%s %s %s\"\n",
            PROJECT_NAME, PROJECT_VERSION_STRING, vcstag);
    fprintf(out->stream, "    .globaltype     __stack_pointer, i32\n"); /* Not sure */

    return 0;
}
