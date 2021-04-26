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

#include <stdlib.h>
#include <string.h>
#include "input_output.h"

int fo_open_input(void *src_ctx, FOInputContext *ctx, const char *filename)
{
    fo_class_alloc(ctx, "input", FO_CLASS_TYPE_READER, NULL);

    FILE *in = fopen(filename, "r");
    if (!in) {
        fo_log(ctx, FO_LOG_ERROR, "Failed to open file: %s!\n", strerror(errno));
        return FOERROR(errno);
    }

    fseek(in, 0, SEEK_END);
    size_t bytes = ftell(in);
    rewind(in);

    ctx->data = malloc(bytes + 1);
    if (!fread(ctx->data, 1, bytes, in)) {
        fo_log(ctx, FO_LOG_ERROR, "File read failed!\n");
        free(ctx->data);
        fclose(in);
        return FOERROR(EINVAL);
    }

    fo_log(ctx, FO_LOG_VERBOSE, "Read %li bytes from %s\n", bytes, filename);
    ctx->src = filename;

    ctx->data[bytes] = '\0';

    fclose(in);
    return 0;
}

void fo_close_input(FOInputContext *ctx)
{
    free(ctx->data);
}

int fo_open_output(void *src_ctx, FOOutputContext *ctx, const char *filename)
{
    fo_class_alloc(ctx, "output", FO_CLASS_TYPE_WRITER, NULL);

    FILE *out = fopen(filename, "w+");
    if (!out) {
        fo_log(ctx, FO_LOG_ERROR, "Failed to open file: %s!\n", strerror(errno));
        return FOERROR(errno);
    }

    fo_log(ctx, FO_LOG_VERBOSE, "File %s opened for writing\n", filename);

    ctx->stream = out;
    ctx->dst = filename;

    return 0;
}

void fo_close_output(FOOutputContext *ctx)
{
    long bytes = ftell(ctx->stream);

    fclose(ctx->stream);

    fo_log(ctx, FO_LOG_VERBOSE, "File %s closed, wrote %li bytes\n", ctx->dst, bytes);
}
