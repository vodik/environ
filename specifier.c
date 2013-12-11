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

#include "specifier.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <pwd.h>
#include <sys/types.h>

#include "env.h"
#include "util.h"

int specifier_user_pwd(char specifier, void *data, void _unused_ *userdata, char **ret)
{
    struct passwd *pwd = data;
    char *n = NULL;

    switch (specifier) {
    case 'u':
        n = strdup(pwd->pw_name);
        break;
    case 'U':
        asprintf(&n, "%d", pwd->pw_uid);
        break;
    case 's':
        n = strdup(pwd->pw_shell);
        break;
    case 'h':
        n = strdup(pwd->pw_dir);
        break;
    default:
        return -EINVAL;
    }

    if (!n)
        return -ENOMEM;

    *ret = n;
    return 0;
}

int specifier_string(char _unused_ specifier, void *data, void _unused_ *userdata, char **ret)
{
    char *n;

    n = strdup(strempty(data));
    if (!n)
        return -ENOMEM;

    *ret = n;
    return 0;
}

/*
 * Generic infrastructure for replacing %x style specifiers in
 * strings. Will call a callback for each replacement.
 *
 */
int specifier_printf(const char *text, const Specifier table[], void *userdata, char **env, char **_ret)
{
    char *ret, *t;
    const char *f;
    bool percent = false;
    size_t l;
    int r;

    assert(text);
    assert(table);

    l = strlen(text);
    ret = malloc(l+1);
    if (!ret)
        return -ENOMEM;

    t = ret;

    for (f = text; *f && *f != '='; f++, l--)
        *(t++) = *f;

    for (; *f; f++, l--) {

        if (percent) {
            _cleanup_free_ char *w = NULL;

            if (*f == '%')
                *(t++) = '%';
            else if (*f == '(') {
                char *end = strchr(f, ')');

                if (end) {
                    char **e;
                    size_t len = end - f - 1;
                    const char *key = f + 1;

                    for (e = env; *e; ++e) {
                        if (strneq(*e, key, len)) {
                            size_t key_len = env_key_length(*e);
                            w = strdup(&(*e)[key_len]);
                        }
                    }

                    f += len + 1;
                } else {
                    *(t++) = '%';
                    *(t++) = '(';
                }
            } else {
                const Specifier *i;

                for (i = table; i->specifier; i++)
                    if (i->specifier == *f)
                        break;

                if (i->lookup) {
                    r = i->lookup(i->specifier, i->data, userdata, &w);
                    if (r < 0) {
                        free(ret);
                        return r;
                    }
                } else {
                    *(t++) = '%';
                    *(t++) = *f;
                }
            }

            if (w) {
                char *n;
                size_t k, j;

                j = t - ret;
                k = strlen(w);

                n = malloc(j + k + l + 1);
                if (!n) {
                    free(ret);
                    return -ENOMEM;
                }

                memcpy(n, ret, j);
                memcpy(n + j, w, k);

                free(ret);

                ret = n;
                t = n + j + k;
            }

            percent = false;
        } else if (*f == '%')
            percent = true;
        else
            *(t++) = *f;
    }

    *t = 0;
    *_ret = ret;
    return 0;
}
