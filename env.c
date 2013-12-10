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

#include "env.h"

#include <stdlib.h>
#include "util.h"

size_t env_key_length(const char *key)
{
    size_t n = strcspn(key, "=");
    if (key[n] == '=')
        n++;
    return n;
}

int env_append(char **env, const char **keys)
{
    const char **k;

    for (k = keys; *k; ++k) {
        size_t n = env_key_length(*k);

        char **e = env;
        for (; *e; ++e) {
            if (strneq(*e, *k, n)) {
                free(*e);
                *e = strdup(*k);
                break;
            }
        }

        if (!*e)
            *e = strdup(*k);
    }

    return 0;
}
