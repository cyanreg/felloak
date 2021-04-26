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

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "logging.h"

struct FOClass {
    char *name;
    enum FOClassType type;
    void *parent;
};

static FILE *log_file = NULL;

//static AVDictionary *log_levels = NULL;

static inline FOClass *get_class(void *ctx)
{
    if (!ctx)
        return NULL;

    struct {
        FOClass *class;
    } *s = ctx;

    return s->class;
}

static inline const char *get_class_color(FOClass *class)
{
    if (!class)
        return "";
    else if (class->type == FO_CLASS_TYPE_NONE)
        return "\033[38;5;243m";
    else if (class->type & FO_CLASS_TYPE_READER)
        return "\033[38;5;199m";
    else if (class->type & FO_CLASS_TYPE_WRITER)
        return "\033[38;5;129m";
    else if (class->type & FO_CLASS_TYPE_PARSER)
        return "\033[38;5;99m";
    else if (class->type & FO_CLASS_TYPE_OUTPUT)
        return "\033[38;5;178m";
#if 0
    else if (class->type & FO_TYPE_CONTEXT)
        return "\033[036m";
    else if (class->type & FO_TYPE_INTERFACE)
        return "\033[38;5;129m";
    else if (class->type & (FO_TYPE_AUDIO_BIDIR | FO_TYPE_VIDEO_BIDIR))
        return "\033[035m";
    else if (class->type & (FO_TYPE_FILTER))
        return "\033[38;5;99m";
    else if (class->type & (FO_TYPE_CODEC))
        return "\033[38;5;199m";
    else if (class->type & (FO_TYPE_MUXING))
        return "\033[38;5;178m";
    else if (class->type & (FO_TYPE_EXTERNAL))
        return "\033[38;5;60m";
#endif
    else
        return "";
}

static char *build_line(FOClass *class, enum FOLogLevel lvl, int with_color,
                        const char *format, va_list args)
{
    char *buf = NULL;
    int bytes = 0;

#define PTOBUF(format, ...)                                          \
    do {                                                             \
        int needed = snprintf(NULL, 0, format, __VA_ARGS__) + 1;     \
        buf = realloc(buf, bytes + needed);                          \
        needed = snprintf(buf + bytes, needed, format, __VA_ARGS__); \
        buf[bytes + needed] = '\0';                                  \
        bytes += needed;                                             \
    } while (0)

#define VPTOBUF(format, args)                                  \
    do {                                                       \
        va_list tmp;                                           \
        va_copy(tmp, args);                                    \
        int needed = vsnprintf(NULL, 0, format, tmp) + 1;      \
        va_end(tmp);                                           \
        buf = realloc(buf, bytes + needed);                    \
        needed = vsnprintf(buf + bytes, needed, format, args); \
        buf[bytes + needed] = '\0';                            \
        bytes += needed;                                       \
    } while (0)

    if (!with_color) {
        if (lvl == FO_LOG_FATAL)
            PTOBUF("%s", "(fatal)");
        else if (lvl == FO_LOG_ERROR)
            PTOBUF("%s", "(error)");
        else if (lvl == FO_LOG_WARN)
            PTOBUF("%s", "(warn)");
        else if (lvl == FO_LOG_VERBOSE)
            PTOBUF("%s", "(info)");
        else if (lvl == FO_LOG_DEBUG)
            PTOBUF("%s", "(debug)");
        else if (lvl == FO_LOG_TRACE)
            PTOBUF("%s", "(trace)");
    }

    if (class) {
        FOClass *parent = get_class(class->parent);
        if (parent)
            PTOBUF("[%s%s%s->%s%s%s]",
                   with_color ? get_class_color(parent) : "",
                   parent->name,
                   with_color ? "\033[0m" : "",
                   with_color ? get_class_color(class) : "",
                   class->name,
                   with_color ? "\033[0m" : "");
        else
            PTOBUF("[%s%s%s]",
                   with_color ? get_class_color(class) : "",
                   class->name,
                   with_color ? "\033[0m" : "");
        PTOBUF("%c", ' ');
    }

    int colord = ~with_color;
    if (lvl == FO_LOG_FATAL && ++colord)
        PTOBUF("%s", "\033[1;031m");
    else if (lvl == FO_LOG_ERROR && ++colord)
        PTOBUF("%s", "\033[1;031m");
    else if (lvl == FO_LOG_WARN && ++colord)
        PTOBUF("%s", "\033[1;033m");
    else if (lvl == FO_LOG_VERBOSE && ++colord)
        PTOBUF("%s", "\033[38;5;46m");
    else if (lvl == FO_LOG_DEBUG && ++colord)
        PTOBUF("%s", "\033[38;5;34m");
    else if (lvl == FO_LOG_TRACE && ++colord)
        PTOBUF("%s", "\033[38;5;28m");

    int format_ends_with_newline = 0;
    if (with_color && colord) {
        format_ends_with_newline = format[strlen(format) - 1] == '\n';
        if (format_ends_with_newline) {
            char *fmt_copy = strdup(format);
            fmt_copy[strlen(fmt_copy) - 1] = '\0';
            format = fmt_copy;
        }
    }

    VPTOBUF(format, args);

    if (with_color && colord) {
        PTOBUF("%s", "\033[0m");
        if (format_ends_with_newline) {
            PTOBUF("%c", '\n');
            free((void *)format);
        }
    }

    return buf;
}

static int decide_print_line(FOClass *class, enum FOLogLevel lvl)
{
#if 0
    AVDictionaryEntry *global_lvl_entry = av_dict_get(log_levels, "global", NULL, 0);
    AVDictionaryEntry *local_lvl_entry = NULL;
    AVDictionaryEntry *parent_lvl_entry = NULL;

    if (class) {
    //    local_lvl_entry = av_dict_get(log_levels, class->name, NULL, 0);
  //      if (class->parent)
//            parent_lvl_entry = av_dict_get(log_levels, fo_class_get_name(class->parent), NULL, 0);
    }

    if (local_lvl_entry) {
        int local_lvl = strtol(local_lvl_entry->value, NULL, 10);
        return local_lvl >= lvl;
    } else if (parent_lvl_entry) {
        int parent_lvl = strtol(parent_lvl_entry->value, NULL, 10);
        return parent_lvl >= lvl;
    } else {
        int global_lvl = global_lvl_entry ? strtol(global_lvl_entry->value, NULL, 10) : FO_LOG_INFO;
        return global_lvl >= lvl;
    }
#endif

    return 1;
}

static void main_log(FOClass *class, enum FOLogLevel lvl, const char *format, va_list args)
{
    int with_color = 1;
    int print_line = decide_print_line(class, lvl);
    int log_line = !!log_file;

    char *pline = NULL;
    char *lline = NULL;
    if (log_line || print_line) {
        if (print_line)
            pline = build_line(class, lvl, with_color, format, args);
        if (log_line)
            lline = build_line(class, lvl, 0, format, args);
    }

    if (pline)
        fprintf(lvl <= FO_LOG_ERROR ? stderr : stdout, "%s", pline);

    if (lline)
        fprintf(log_file, "%s", lline);

    free(pline);
    free(lline);
}

void fo_log(void *classed_ctx, enum FOLogLevel lvl, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    FOClass *class = get_class(classed_ctx);
    main_log(class, lvl, format, args);
    va_end(args);
}

int fo_class_alloc(void *ctx, const char *name, enum FOClassType type, void *parent)
{
    struct {
        FOClass *class;
    } *s = ctx;

    s->class = calloc(1, sizeof(FOClass));
    if (!s->class)
        return FOERROR(ENOMEM);

    s->class->name = strdup(name);
    s->class->type = type;
    s->class->parent = parent;


    return 0;
}

void fo_class_free(void *ctx)
{
    FOClass *class = get_class(ctx);
    if (!class)
        return;

    free(class->name);
    free(class);
}

int fo_class_set_name(void *ctx, const char *name)
{
    FOClass *class = get_class(ctx);
    if (!class)
        return FOERROR(EINVAL);

    char *dup = strdup(name);
    if (!dup)
        return FOERROR(ENOMEM);

    free(class->name);
    class->name = dup;

    return 0;
}

const char *fo_class_get_name(void *ctx)
{
    FOClass *class = get_class(ctx);
    if (!class)
        return NULL;
    return class->name;
}

const char *fo_class_get_parent_name(void *ctx)
{
    FOClass *class = get_class(ctx);
    if (class) {
        FOClass *parent = get_class(class->parent);
        if (parent)
            return parent->name;
    }
    return NULL;
}

enum FOClassType fo_class_get_type(void *ctx)
{
    return get_class(ctx)->type;
}

const char *fo_class_type_string(void *ctx)
{
    FOClass *class = get_class(ctx);
    if (!class)
        return NULL;

    switch (class->type) {
    case FO_CLASS_TYPE_NONE:         return "none";

    /* zalgofied because this should never happen */
    default:                         return "û̴̼n̷̡̎̄k̸͍̓͒ṅ̵̨̅ò̷̢̏w̷̙͍͌n̸̩̦̅";
    }
}

int fo_log_set_ctx_lvl_str(const char *component, const char *lvl)
{
#if 0
    enum FOLogLevel res;
    if (!strcmp(lvl, "quiet"))
        res = FO_LOG_QUIET;
    else if (!strcmp(lvl, "fatal"))
        res = FO_LOG_FATAL;
    else if (!strcmp(lvl, "error"))
        res = FO_LOG_ERROR;
    else if (!strcmp(lvl, "warn"))
        res = FO_LOG_WARN;
    else if (!strcmp(lvl, "info"))
        res = FO_LOG_INFO;
    else if (!strcmp(lvl, "verbose"))
        res = FO_LOG_VERBOSE;
    else if (!strcmp(lvl, "debug"))
        res = FO_LOG_DEBUG;
    else if (!strcmp(lvl, "trace"))
        res = FO_LOG_TRACE;
    else
        return FOERROR(EINVAL);

    av_dict_set_int(&log_levels, component, res, 0);
#endif

    return 0;
}

void fo_log_set_ctx_lvl(const char *component, enum FOLogLevel lvl)
{
//    av_dict_set_int(&log_levels, component, lvl, 0);
}

int fo_log_set_file(const char *path)
{
    int ret = 0;
    if (!log_file) {
        log_file = fopen(path, "w");
        if (!log_file)
            ret = FOERROR(EINVAL);
    }
    return ret;
}

void fo_log_end(void)
{
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}
