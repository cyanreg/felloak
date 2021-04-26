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

#include "parse_arm.h"
#include "utils.h"
#include "logging.h"

static const FOInstructionMap fo_instruction_map_arm64[] = {
    {
        .inst = FO_NOP,
        .name = "nop",
        .ele_type = FO_GENERIC,
        .exp_type = {
            0,
        },
    },

    {
        .inst = FO_MOV,
        .name = "mov",
        .ele_type = FO_GENERIC,
        .exp_type = {
            FO_SCALAR | FO_VECTOR,
            FO_SCALAR | FO_VECTOR,
        },
    },

    {
        .inst = FO_MOV,
        .name = "movrel",
        .ele_type = FO_PTR,
        .exp_type = {
            FO_SCALAR,
            FO_RODATA,
        },
    },

    {
        .inst = FO_JMP_GT,
        .name = "b.gt",
        .ele_type = FO_GENERIC,
        .exp_type = {
            FO_LABEL,
        },
    },

    {
        .inst = FO_RET,
        .name = "ret",
        .ele_type = FO_GENERIC,
        .exp_type = {
            0,
        },
    },

    {
        .inst = FO_ADD,
        .name = "add",
        .ele_type = FO_S32,
        .exp_type = {
            FO_SCALAR,
            FO_SCALAR,
            FO_SCALAR | FO_IMMEDIATE,
        },
    },

    {
        .inst = FO_SUB,
        .name = "sub",
        .ele_type = FO_S32,
        .exp_type = {
            FO_SCALAR,
            FO_SCALAR,
            FO_SCALAR | FO_IMMEDIATE,
        },
    },

    {
        .inst = FO_SUB,
        .name = "subs",
        .ele_type = FO_S32,
        .exp_type = {
            FO_SCALAR,
            FO_SCALAR,
            FO_SCALAR | FO_IMMEDIATE,
        },
    },

    {
        .inst = FO_SUB,
        .name = "subs",
        .ele_type = FO_U32,
        .exp_type = {
            FO_SCALAR,
            FO_SCALAR,
            FO_SCALAR | FO_IMMEDIATE,
        },
    },

    {
        .inst = FO_LOAD_V,
        .name = "ld1",
        .ele_type = FO_GENERIC,
        .exp_type = {
            FO_VECTOR | FO_LIST,
            FO_VECTOR | FO_LIST | FO_OPTIONAL,
            FO_VECTOR | FO_LIST | FO_OPTIONAL,
            FO_VECTOR | FO_LIST | FO_OPTIONAL,
            FO_SCALAR,
            FO_IMMEDIATE | FO_OPTIONAL,
        },
    },

    {
        .inst = FO_STORE_V,
        .name = "st1",
        .ele_type = FO_GENERIC,
        .exp_type = {
            FO_VECTOR | FO_LIST,
            FO_VECTOR | FO_LIST | FO_OPTIONAL,
            FO_VECTOR | FO_LIST | FO_OPTIONAL,
            FO_VECTOR | FO_LIST | FO_OPTIONAL,
            FO_SCALAR,
            FO_IMMEDIATE | FO_OPTIONAL,
        },
    },

    {
        .inst = FO_BROADCAST_V,
        .name = "dup",
        .ele_type = FO_GENERIC,
        .exp_type = {
            FO_VECTOR,
            FO_VECTOR,
        },
    },

    {
        .inst = FO_ADD_V,
        .name = "fadd",
        .ele_type = FO_FLT,
        .exp_type = {
            FO_VECTOR,
            FO_VECTOR,
            FO_VECTOR,
        },
    },

    {
        .inst = FO_FMADD_V,
        .name = "fmla",
        .ele_type = FO_FLT,
        .exp_type = {
            FO_VECTOR,
            FO_VECTOR,
            FO_VECTOR,
        },
    },

    {
        .inst = FO_SHLL,
        .name = "lsl",
        .ele_type = FO_S64,
        .exp_type = {
            FO_SCALAR,
            FO_SCALAR,
            FO_SCALAR | FO_IMMEDIATE,
        },
    },

    {
        .inst = FO_MUL_V,
        .name = "fmul",
        .ele_type = FO_FLT,
        .exp_type = {
            FO_VECTOR,
            FO_VECTOR,
            FO_VECTOR,
        },
    },
};

static int parse_line(FOARMContext *ctx, FOFunction *fn, char *line)
{
    int err;
    FOInstruction *instr = calloc(1, sizeof(*instr));
    FOInstruction *instr_fused = calloc(1, sizeof(*instr_fused));

    char *name_end = fo_skip_to_next_whitespace(line);
    if (name_end && name_end > line)
        name_end[0] = '\0';

    int cnt;
    for (cnt = 0; cnt < FO_ARRAY_ELEMS(fo_instruction_map_arm64); cnt++) {
        if (!strcmp(fo_instruction_map_arm64[cnt].name, line)) {
            instr->map = fo_instruction_map_arm64[cnt];
            break;
        }
    }
    if (cnt == FO_ARRAY_ELEMS(fo_instruction_map_arm64)) {
        fo_log(ctx, FO_LOG_ERROR, "Instruction %s not found!\n", line);
        err = FOERROR(EINVAL);
        goto err;
    }

    if (!name_end++) {
        if (!strcmp(line, "ret")) {
            for (cnt = 0; cnt < FO_ARRAY_ELEMS(fo_instruction_map_arm64); cnt++) {
                if (fo_instruction_map_arm64[cnt].inst == FO_RET) {
                    instr->map = fo_instruction_map_arm64[cnt];
                    break;
                }
            }
        }

        err = 0;
        goto send_instruction;
    }

    int is_list = 0;
    int end_list = 0;
    char *save, *token = fo_strtok(name_end, ",", &save);
    while (token) {
        int is_memory = 0;

        if (token[0] == ' ')
            token = fo_skip_whitespace(token);

        char *padding = fo_skip_to_next_whitespace(line);
        if (padding)
            padding[0] = '\0';

        if (token[0] == '[') {
            is_memory = 1;
            token++;
            char *end_token = strstr(token, "]");
            if (!end_token) {
                fo_log(ctx, FO_LOG_ERROR, "Missing closing bracked ']' in instruction \"%s\"!\n",
                       instr->map.name);
                err = FOERROR(EINVAL);
                goto err;
            } else {
                end_token[0] = '\0';
            }
        }

        if (token[0] == '{') {
            is_list = 1;
            token++;
            token = fo_skip_whitespace(token);
        }
        if (is_list) {
            char *end_token = strstr(token, "}");
            if (end_token) {
                end_list = 1;
                end_token[0] = '\0';
            }
        }

        FOType ele_type = 0x0;
        int nb_elements = 0;
        int op_id = 0;

        FORegisterType reg_type = 0x0;
        reg_type |= (is_list ? FO_LIST : 0x0);
        reg_type |= (is_memory ? FO_MEMORY : 0x0);

        int vector_extract_index = 0;
        int64_t immediate_val = 0x0;
        if (token[0] == 'v') {
            reg_type |= FO_VECTOR;
            ele_type = instr->map.ele_type;
            if (token[strlen(token) - 1] == ']')
                nb_elements = 1;
            else
                nb_elements = 16/fo_type_to_bytes(ele_type);

            char *end_token = strstr(token, "[");
            if (end_token)
                vector_extract_index = 1 << strtol(end_token, NULL, 10);

            end_token = strstr(token, ".");
            end_token[0] = '\0';

            op_id = strtol(token + 1, NULL, 0);
        } else if (token[0] == 'x') {
            reg_type |= FO_SCALAR;
            ele_type = FO_S64;
            nb_elements = 1;
            op_id = strtol(token + 1, NULL, 0);
        } else if (token[0] == 'w') {
            reg_type |= FO_SCALAR;
            ele_type = FO_S32;
            nb_elements = 1;
            op_id = strtol(token + 1, NULL, 0);
        } else if (token[0] == 's') {
            reg_type |= FO_SCALAR;
            ele_type = FO_FLT;
            nb_elements = 1;
            op_id = strtol(token + 1, NULL, 0);
        } else if ((instr->map.inst == FO_LOAD_V || instr->map.inst == FO_STORE_V) &&
                   token[0] == '#') {
            instr->map.fused_next = 1;
            for (cnt = 0; cnt < FO_ARRAY_ELEMS(fo_instruction_map_arm64); cnt++) {
                if (fo_instruction_map_arm64[cnt].inst == FO_ADD) {
                    instr_fused->map = fo_instruction_map_arm64[cnt];
                    break;
                }
            }

            instr_fused->operands[0] = instr->operands[instr->nb_operands - 1];
            instr_fused->operands[1] = instr->operands[instr->nb_operands - 1];
            instr_fused->operands[2].reg_type = FO_IMMEDIATE;
            instr_fused->operands[2].elements = 1;
            instr_fused->operands[2].immediate_val = strtol(token + 1, NULL, 10);
            instr_fused->nb_operands = 3;
            goto skip;
        } else if (token[0] == '#') {
            reg_type |= FO_IMMEDIATE;
            immediate_val = strtol(token, NULL, 0);
            nb_elements = 1;
        } else if (fo_char_is_number(token[0]) && token[strlen(token) - 1] == 'b') {
            reg_type |= FO_LABEL;
            token[strlen(token) - 1] = '\0';
        } else if (!strcmp(instr->map.name, "movrel")) {
            reg_type |= FO_RODATA;
        } else if (!strncmp(token, "lsl", 3)) { /* Fused instruction */
            instr->map.fused_next = 1;
            for (cnt = 0; cnt < FO_ARRAY_ELEMS(fo_instruction_map_arm64); cnt++) {
                if (fo_instruction_map_arm64[cnt].inst == FO_SHLL) {
                    instr_fused->map = fo_instruction_map_arm64[cnt];
                    break;
                }
            }

            instr_fused->operands[0] = instr->operands[0];
            instr_fused->operands[1] = instr->operands[0];
            instr_fused->operands[2].reg_type = FO_IMMEDIATE;
            instr_fused->operands[2].elements = 1;
            instr_fused->operands[2].immediate_val = strtol(token + 3, NULL, 10);
            instr_fused->nb_operands = 3;
            goto skip;
        } else {
            fo_log(ctx, FO_LOG_ERROR, "Unrecognized operand \"%s\" in instruction \"%s\"!\n",
                   token, instr->map.name);
            err = FOERROR(EINVAL);
            goto err;
        }

        if (op_id < 0 || op_id > 32) {
            fo_log(ctx, FO_LOG_ERROR, "Unrecognized operand \"%s\" in instruction \"%s\"!\n",
                   token, instr->map.name);
            err = FOERROR(EINVAL);
            goto err;
        }

        instr->operands[instr->nb_operands].ele_type = ele_type;
        instr->operands[instr->nb_operands].reg_type = reg_type;
        instr->operands[instr->nb_operands].vextr_indices = vector_extract_index;
        instr->operands[instr->nb_operands].elements = nb_elements;
        instr->operands[instr->nb_operands].immediate_val = immediate_val;
        instr->operands[instr->nb_operands].name = token;
        instr->operands[instr->nb_operands].src_id = op_id;
        instr->nb_operands++;

skip:
        if (end_list)
            is_list = 0;
        token = fo_strtok(NULL, ",", &save);
    }

send_instruction:
    if ((err = fo_fn_add_instr(ctx, ctx->inm, fn, instr)) < 0)
        goto err;
    if (instr->map.fused_next && (err = fo_fn_add_instr(ctx, ctx->inm, fn, instr)) < 0)
        goto err;

    return 0;
err:
    free(instr);
    free(instr_fused);
    return err;
}

static int parse_prop(char *str, const char *prop, int *prop_val)
{
    char *prop_loc = strstr(str + 1, prop);
    if (!prop_loc)
        return 0;

    prop_loc = strstr(prop_loc, "=");
    if (!prop_loc)
        return FOERROR(EINVAL);

    prop_loc += 1;

    prop_loc = fo_skip_whitespace(prop_loc);

    char *prop_end = prop_loc;
    while (*prop_end != '\0' && *prop_end != ' ')
        prop_end++;
    char prop_end_char = *prop_end;
    *prop_end = '\0';

    char *end;
    long val = strtol(prop_loc, &end, 0);
    if (!val && prop_loc == end) {
        *prop_end = prop_end_char;
        return FOERROR(EINVAL);
    }

    *prop_end = prop_end_char;
    *prop_val = val;

    return 0;
}

int fo_parse_arm_to_ir(FOIntermediate *inm, FOARMContext *ctx, FOInputContext *in)
{
    fo_class_alloc(ctx, "arm_parser", FO_CLASS_TYPE_PARSER, in);

    ctx->inm = inm;

    int err = 0;

    FOFunction *p_function = NULL;
    int p_function_line = 0;

    FOData *p_const = NULL;
    int p_const_line = 0;
    int p_const_type_bits = 0;

    char *p_macro_name = NULL;
    int p_macro = 0;
    int p_macro_line = 0;

    char *line = in->data;
    int line_nb = 1;

    do {
        char *next_line = strstr(line, "\n");
        if (next_line)
            next_line[0] = '\0';

start:
        /* Skip whitespace */
        line = fo_skip_whitespace(line);

        /* Skip to next line */
        if (line[0] == '\n' || line[0] == '#') {
            if (line[0] == '#')
                fo_log(ctx, FO_LOG_TRACE, "Skipping comment on line %i: \"%s\"\n", line_nb, line);
            goto next;
        }

        fo_log(ctx, FO_LOG_TRACE, "Line %i: %s\n", line_nb, line);

        /* Skip whitespace lines */
        if (line == next_line)
            goto next;

        /* Check for a label */
        char *label = strstr(line, ":");
        if (label) {
            label[0] = '\0';
            if (!p_function) {
                fo_log(ctx, FO_LOG_ERROR, "Label \"%s\" defined on line %i outside of a function!\n",
                       line, line_nb);
                err = FOERROR(EINVAL);
                goto end;
            }
            fo_log(ctx, FO_LOG_VERBOSE, "Registering new label \"%s\" on line %i in function \"%s\"\n",
                   line, line_nb, p_function->name);

            if (!fo_is_number(line)) {
                fo_log(ctx, FO_LOG_ERROR, "Label name \"%s\" is not a number!\n", line);
                err = FOERROR(EINVAL);
                goto end;
            }

            char backup_label[64];
            memcpy(backup_label, line, 63);
            err = fo_fn_add_label(ctx, p_function, line);
            if (err < 0) {
                fo_log(ctx, FO_LOG_ERROR, "Error parsing label \"%s\" defined on line %i!\n",
                       backup_label, line_nb);
                err = FOERROR(EINVAL);
                goto end;
            }

            line = label + 1;
            goto start;
        }

        if (!p_function && !p_const) {
            if (!strncmp(line, "const ", strlen("const "))) {
                line += strlen("const ");

                p_const = fo_alloc_type_data();

                char *attribs = strstr(line, ",");
                if (attribs)
                    attribs[0] = '\0';

                p_const->name = strdup(line);
                p_const_line = line_nb;
                p_const->is_const = 1;
                p_const_type_bits = 0;

                if (attribs) {
                    err = parse_prop(attribs + 1, "align", &p_const->alignment);
                    if (err < 0) {
                        fo_log(ctx, FO_LOG_ERROR, "Error parsing property \"align\" in "
                               "function \"%s\" on line %i!\n",
                               p_const->name, p_const_line);
                        goto end;
                    }
                }

                fo_log(ctx, FO_LOG_VERBOSE, "Registering new constant \"%s\", alignment = %i\n",
                       p_const->name, p_const->alignment);
                goto next; /* TODO: make this more liberal maybe? */
            } else if (!strncmp(line, "function ", strlen("function "))) {
                line += strlen("function ");

                p_function = fo_alloc_function();

                char *attribs = strstr(line, ",");
                if (attribs)
                    attribs[0] = '\0';

                p_function->name = strdup(line);
                p_function_line = line_nb;

                if (attribs) {
                    err = parse_prop(attribs + 1, "export", &p_function->is_global);
                    if (err < 0) {
                        fo_log(ctx, FO_LOG_ERROR, "Error parsing property \"export\" in "
                               "function \"%s\" on line %i!\n",
                               p_function->name, p_function_line);
                        goto end;
                    }
                }

                fo_log(ctx, FO_LOG_VERBOSE, "Registering new %s function \"%s\"\n",
                       p_function->is_global ? "global" : "local", p_function->name);
                goto next;
            } else {
                fo_log(ctx, FO_LOG_ERROR, "Unrecognized identifier \"%s\" on line %i!\n",
                       line, line_nb);
                err = FOERROR(EINVAL);
                goto end;
            }
        } else if (p_const) {
            if (!strcmp(line, "endconst")) {
                fo_log(ctx, FO_LOG_VERBOSE, "Constant \"%s\" parsed, %li bytes\n",
                       p_const->name, p_const->size);
                inm->rodata[inm->nb_rodata++] = p_const;
                p_const = NULL;
                goto next;
            } else if (p_const_type_bits) {
                char *more = strstr(line, ",");
                if (more)
                    more[0] = '\0';

                char *end;
                unsigned long long bin = strtoull(line, &end, 0);
                if (!bin && line == end) {
                    fo_log(ctx, FO_LOG_ERROR, "Invalid value \"%s\" in const \"%s\" on line %i!\n",
                           line, p_const->name, p_const_line);
                    err = FOERROR(EINVAL);
                    goto end;
                }

                int bin_bytes = p_const_type_bits >> 3;
                bin &= 0xffffffffffffffff >> (64 - p_const_type_bits);

                p_const->data = realloc(p_const->data, p_const->size + bin_bytes);
                memcpy(p_const->data + p_const->size, &bin, bin_bytes);
                p_const->size += bin_bytes;

                fo_log(ctx, FO_LOG_DEBUG, "Adding %i bit value of %llu (0x%llx) to "
                       "constant \"%s\" on line %i, now %li bytes\n",
                       p_const_type_bits, bin, bin, p_const->name, p_const_line, p_const->size);
                if (more) {
                    line = more + 1;
                    goto start;
                } else {
                    goto next;
                }

#define PSIZE(str, bits)                                                   \
    } else if (!strncmp(line, str, strlen(str))) {                         \
        p_const_type_bits = bits;                                          \
        line += strlen(str);                                               \
        fo_log(ctx, FO_LOG_DEBUG, "Switching size fot constant \"%s\" on " \
               "line %i to %i bits!\n",                                    \
               p_const->name, p_const_line, p_const_type_bits);            \
        goto start;
            PSIZE(".byte",   8)
            PSIZE(".hword", 16)
            PSIZE(".word",  32)
            PSIZE(".quad",  64)
#undef PSIZE

            } else {
                fo_log(ctx, FO_LOG_ERROR, "No size specified for constant \"%s\" on line %i!\n",
                       p_const->name, p_const_line);
                err = FOERROR(EINVAL);
                goto end;
            }
        } else if (p_function) {
            if (!strcmp(line, "endfunc")) {
                 fo_log(ctx, FO_LOG_VERBOSE, "%s function \"%s\" parsed, %i instructions, %i block(s)\n",
                        p_function->is_global ? "Global" : "Local",
                        p_function->name, fo_fn_nb_instructions(p_function), fo_fn_nb_blocks(p_function));
                inm->functions[inm->nb_functions++] = p_function;
                p_function = NULL;
                p_function_line = 0;
                goto next;
            } else {
                char line_backup[64];
                memcpy(line_backup, line, 63);
                err = parse_line(ctx, p_function, line);
                if (err < 0) {
                    fo_log(ctx, FO_LOG_ERROR, "Error parsing line \"%s\" on line %i!\n",
                           line_backup, line_nb);
                    goto end;
                }
            }
        }

next:
        line = next_line ? (next_line + 1) : NULL;
        line_nb++;
    } while (line && (line[0] != '\0'));

end:
    if (p_function) {
        if (!err)
            fo_log(ctx, FO_LOG_ERROR, "Unterminated function \"%s\" at line %i!\n",
                   p_function->name, p_function_line);
        fo_fn_free(&p_function);
        err = err ? err : FOERROR(EINVAL);
    }
    if (p_const) {
        if (!err)
            fo_log(ctx, FO_LOG_ERROR, "Unterminated constant \"%s\" at line %i!\n",
                   p_const->name, p_const_line);
        fo_free_type_data(&p_const);
        err = err ? err : FOERROR(EINVAL);
    }
    if (p_macro) {
        if (!err)
            fo_log(ctx, FO_LOG_ERROR, "Unterminated macro \"%s\" at line %i!\n",
                   p_macro_name, p_macro_line);
        err = err ? err : FOERROR(EINVAL);
    }

    if (err >= 0) {
        fo_log(ctx, FO_LOG_INFO, "File \"%s\" parsed, %i rodata entries, "
               "%i functions\n", in->src, inm->nb_rodata, inm->nb_functions);
    }

    return err;
}
