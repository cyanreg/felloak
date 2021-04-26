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

#include <stdio.h>
#include "logging.h"

typedef struct FOInputContext {
    FOClass *class;
    char *data;
    const char *src;
} FOInputContext;

typedef struct FOOutputContext {
    FOClass *class;
    FILE *stream;
    const char *dst;
} FOOutputContext;

int fo_open_input(void *src_ctx, FOInputContext *ctx, const char *filename);
void fo_close_input(FOInputContext *ctx);

int fo_open_output(void *src_ctx, FOOutputContext *ctx, const char *filename);
void fo_close_output(FOOutputContext *ctx);
