/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
* Copyright (C) Simon Gomizelj, 2013
*/

#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>

#define _unused_          __attribute__((unused))
#define _noreturn_        __attribute__((noreturn))
#define _printf_(a,b)     __attribute__((format (printf, a, b)))
#define _cleanup_(x)      __attribute__((cleanup(x)))
#define _destructor_      __attribute__((destructor))

#define _cleanup_free_      _cleanup_(freep)
#define _cleanup_fclose_    _cleanup_(fclosep)
#define _cleanup_closedir_  _cleanup_(closedirp)

static inline void freep(void *p) { free(*(void **)p); }
static inline void fclosep(FILE **fp) { if (*fp) fclose(*fp); }
static inline void closedirp(DIR **dp) { if (*dp) closedir(*dp); }

static inline bool streq(const char *s1, const char *s2) { return strcmp(s1, s2) == 0; }
static inline bool strneq(const char *s1, const char *s2, size_t n) { return strncmp(s1, s2, n) == 0; }
static inline const char* strempty(const char *s) { return s ? s : ""; }

char *truncate_to(char *s, const char *term);
char *strappend(const char *s, const char *suffix);
char *strnappend(const char *s, const char *suffix, size_t b);

char *joinpath(const char *root, ...);
char *joinpath_ap(const char *root, va_list ap);
