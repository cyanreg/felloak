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
#include <errno.h>

#define FOERROR(num) (-(num))

enum FOClassType {
    FO_CLASS_TYPE_NONE = 0,
    FO_CLASS_TYPE_READER = (1 << 0),
    FO_CLASS_TYPE_WRITER = (1 << 1),

    FO_CLASS_TYPE_PARSER = (1 << 2),
    FO_CLASS_TYPE_OUTPUT = (1 << 0),
};

enum FOLogLevel {
    FO_LOG_QUIET   = -(1 << 0),
    FO_LOG_FATAL   =  (0 << 0),
    FO_LOG_ERROR   = +(1 << 0),
    FO_LOG_WARN    = +(1 << 1),
    FO_LOG_INFO    = +(1 << 2),
    FO_LOG_VERBOSE = +(1 << 3),
    FO_LOG_DEBUG   = +(1 << 4),
    FO_LOG_TRACE   = +(1 << 5),
};

typedef struct FOClass FOClass;

#if defined(__GNUC__) || defined(__clang__)
#define fo_printf_format(fmtpos, attrpos) __attribute__((__format__(__printf__, fmtpos, attrpos)))
#else
#define fo_printf_format(fmtpos, attrpos)
#endif

/* ffmpeg log callback */
void fo_log_set_ff_cb(void);

/* Context level */
void fo_log_set_ctx_lvl(const char *component, enum FOLogLevel lvl);
int fo_log_set_ctx_lvl_str(const char *component, const char *lvl);

/* Main logging */
void fo_log(void *ctx, int level, const char *fmt, ...) fo_printf_format(3, 4);

/* Set log file */
int fo_log_set_file(const char *path);

/* Stop logging and free all */
void fo_log_end(void);

/* Class allocation */
int fo_class_alloc(void *ctx, const char *name, enum FOClassType type, void *parent);
void fo_class_free(void *ctx);

/* Getters */
const char *fo_class_get_name(void *ctx);
const char *fo_class_get_parent_name(void *ctx);
int fo_class_set_name(void *ctx, const char *name);
enum FOClassType fo_class_get_type(void *ctx);
const char *fo_class_type_string(void *ctx);
