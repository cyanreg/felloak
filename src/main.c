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

#include <stdio.h>
#include <unistd.h>

#include "logging.h"
#include "utils.h"

#include "common.h"
#include "parse_arm.h"
#include "output_wasm.h"
#include "input_output.h"

#include "version.h"
#include "../config.h"

typedef struct FOMainContext {
    FOClass *class;
} FOMainContext;

int main(int argc, char *argv[])
{
    int ret = 0;
    FOMainContext ctx = { 0 };

    const char *input_file = NULL;
    const char *output_file = NULL;

    /* Options parsing */
    int opt;
    while ((opt = getopt(argc, argv, "Vvho:")) != -1) {
        switch (opt) {
        case 'o':
            output_file = optarg;
            break;
        case 'v':
            fprintf(stderr, "%s %s (%s)\n", PROJECT_NAME, PROJECT_VERSION_STRING, vcstag);
            return 0;
        case 'V':
            {
                char *save, *token = strtok_r(optarg, ",", &save);
                while (token) {
                    char *val = strstr(token, "=");
                    if (val) {
                        val[0] = '\0';
                        val++;
                    } else {
                        val = token;
                        token = "global";
                    }

                    ret = fo_log_set_ctx_lvl_str(token, val);
                    if (ret < 0) {
                        fo_log(&ctx, FO_LOG_ERROR, "Invalid verbose level \"%s\"!\n", val);
                        ret = FOERROR(EINVAL);
                        goto end;
                    }

                    token = strtok_r(NULL, ",", &save);
                }
            }
            break;
        default:
            fo_log(&ctx, FO_LOG_ERROR, "Unrecognized option \'%c\'!\n", optopt);
            ret = 1;
        case 'h':
            fprintf(stderr, "Usage info:\n"
                   "    -o <path>                     Output file\n"
                   "    -v                            Print program version\n"
                   "    -V (optional component)       Toggle debug info printing\n"
                   "    -h                            Usage help (this)\n"
                   "    <trailing argument>           Input file\n"
                   );
            return ret;
        }
    }

    fo_class_alloc(&ctx, "main", FO_CLASS_TYPE_NONE, NULL);

    if (optind >= argc) {
        fo_log(&ctx, FO_LOG_ERROR, "No input file specified!\n");
        return 1;
    } else if (argc - optind > 1) {
        fo_log(&ctx, FO_LOG_ERROR, "Multiple input files specified!\n");
        return 1;
    } else if (!output_file) {
        fo_log(&ctx, FO_LOG_ERROR, "No output file specified!\n");
        return 1;
    }

    input_file = argv[optind];

    FOInputContext input_ctx = { 0 };
    if ((ret = fo_open_input(&ctx, &input_ctx, input_file)))
        goto end;

    FOARMContext arm_ctx = { 0 };
    FOIntermediate inm = { 0 };
    if ((ret = fo_parse_arm_to_ir(&inm, &arm_ctx, &input_ctx)))
        goto end;

    FOOutputContext output_ctx = { 0 };
    if ((ret = fo_open_output(&ctx, &output_ctx, output_file)) < 0)
        goto end;

    FOWasmContext wasm_ctx = { 0 };
    if ((ret = fo_wasm_output_from_ir(&wasm_ctx, &inm, &output_ctx)) < 0)
        goto end;

    fo_close_input(&input_ctx);
    fo_close_output(&output_ctx);

end:
    return ret;
}

#ifdef HAVE_WMAIN
#include <windows.h>
#include <wchar.h>
int wmain(int argc, wchar_t *argv[])
{
    int i, ret, buffsize = 0, offset = 0;
    char *argstr_flat, **win32_argv_utf8 = NULL;

    /* determine the UTF-8 buffer size (including NULL-termination symbols) */
    for (i = 0; i < argc; i++)
        buffsize += WideCharToMultiByte(CP_UTF8, 0, argv[i], -1,
                                        NULL, 0, NULL, NULL);

    win32_argv_utf8 = calloc((argc + 1) + buffsize, sizeof(char *));
    argstr_flat = (char *)win32_argv_utf8 + sizeof(char *) * (argc + 1);

    for (i = 0; i < argc; i++) {
        win32_argv_utf8[i] = &argstr_flat[offset];
        offset += WideCharToMultiByte(CP_UTF8, 0, argv[i], -1,
                                      &argstr_flat[offset],
                                      buffsize - offset, NULL, NULL);
    }

    win32_argv_utf8[i] = NULL;

    ret = main(argc, win32_argv_utf8);

    free(win32_argv_utf8);
    return ret;
}
#endif
