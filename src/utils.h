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

#include <string.h>

#define FO_ARRAY_ELEMS(a) (sizeof(a) / sizeof((a)[0]))

static inline int fo_char_is_number(const char ch)
{
    if (!(ch >= '0' && ch <= '9') && ch != '.')
        return 0;
    return 1;
}

static inline int fo_is_number(const char *src)
{
    for (int i = 0; i < strlen(src); i++)
        if (!fo_char_is_number(src[i]))
            return 0;
    return 1;
}

static inline char *fo_skip_whitespace(char *str)
{
    while (str[0] == ' ' || str[0] == '\t')
        str++;
    return str;
}

static inline char *fo_skip_to_next_whitespace(char *str)
{
    str = fo_skip_whitespace(str);
    while (str[0] != ' ' && str[0] != '\t' && str[0] != '\n' && str[0] != '\0')
        str++;
    if (str[0] == '\n' || str[0] == '\0')
        return NULL;
    return str;
}

static inline char *fo_skip_to_next_token(char *str)
{
    str = fo_skip_whitespace(str);
    while (str[0] != ' ' && str[0] != '\t' && str[0] != '\n' && str[0] != '\0')
        str++;
    str = fo_skip_whitespace(str);
    if (str[0] == '\n' || str[0] == '\0')
        return NULL;
    return str;
}

static inline char *fo_strtok(char *s, const char *delim, char **saveptr)
{
    char *tok;

    if (!s && !(s = *saveptr))
        return NULL;

    /* skip leading delimiters */
    s += strspn(s, delim);

    /* s now points to the first non delimiter char, or to the end of the string */
    if (!*s) {
        *saveptr = NULL;
        return NULL;
    }
    tok = s++;

    /* skip non delimiters */
    s += strcspn(s, delim);
    if (*s) {
        *s = 0;
        *saveptr = s+1;
    } else {
        *saveptr = NULL;
    }

    return tok;
}
