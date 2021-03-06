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

static inline size_t next_power(size_t x)
{
    return 1UL << (64 - __builtin_clzl(x - 1));
}

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

static char *env_lookup(char **env, const char *key, size_t len)
{
    char **e;

    for (e = env; *e; ++e) {
        if (strneq(*e, key, len)) {
            size_t key_len = env_key_length(*e);
            return strdup(&(*e)[key_len]);
        }
    }

    return NULL;
}

static char *specifier_lookup(const Specifier *table, void  *userdata, char specifier)
{
    const Specifier *i;

    for (i = table; i->specifier; i++) {
        if (i->specifier == specifier)
            break;
    }

    if (!i->lookup)
        return NULL;

    char *w = NULL;
    if (i->lookup(i->specifier, i->data, userdata, &w) < 0)
        return NULL;

    return w;
}

/*
 * Generic infrastructure for replacing %x style specifiers in
 * strings. Will call a callback for each replacement.
 *
 */
int specifier_printf(const char *text, const Specifier *table, void *userdata, char **env, char **_ret)
{
    assert(text);
    assert(table);

    size_t write_pos = strcspn(text, "=");
    if (text[write_pos] != '=')
        return -EINVAL;

    size_t text_len = strlen(text);
    size_t buf_len = (text_len + 1 < 64) ? 64 : next_power(text_len + 1);

    char *ret = malloc(buf_len);
    if (!ret)
        return -ENOMEM;

    ++write_pos;
    memcpy(ret, text, write_pos);

    size_t read_pos = write_pos;
    bool percent = false;
    for (; read_pos < text_len; read_pos++) {
        if (percent) {
            _cleanup_free_ char *payload = NULL;

            if (text[read_pos] == '%') {
                ret[write_pos++] = '%';
            } else if (text[read_pos] == '(') {
                size_t end = strcspn(&text[read_pos], ")");

                if (text[read_pos + end] == ')') {
                    payload = env_lookup(env, &text[read_pos + 1], end - 1);
                    read_pos += end;
                } else {
                    ret[write_pos++] = '%';
                    ret[write_pos++] = '(';
                }
            } else {
                payload = specifier_lookup(table, userdata, text[read_pos]);
                if (!payload) {
                    ret[write_pos++] = '%';
                    ret[write_pos++] = text[read_pos];
                }
            }

            if (payload) {
                size_t payload_len = strlen(payload);
                size_t newlen = write_pos + payload_len + text_len + 1;

                if (newlen > buf_len) {
                    buf_len = next_power(newlen);

                    char *temp = realloc(ret, buf_len);
                    if (!temp) {
                        free(ret);
                        return -ENOMEM;
                    }

                    ret = temp;
                }

                memcpy(&ret[write_pos], payload, payload_len);
                write_pos += payload_len;
            }

            percent = false;
        } else if (text[read_pos] == '%') {
            percent = true;
        } else {
            ret[write_pos++] = text[read_pos];
        }
    }

    ret[write_pos] = '\0';
    *_ret = ret;
    return 0;
}
