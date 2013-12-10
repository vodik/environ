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

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <err.h>
#include <assert.h>
#include <unistd.h>
#include <pwd.h>
#include <limits.h>

#include "env.h"
#include "specifier.h"
#include "util.h"

static const char *default_path = "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin";
static const char *default_locale = "LANG=C";

static char **parse_line(const char *line, const Specifier *table, char **env)
{
    _cleanup_free_ char *value = NULL;
    specifier_printf(line, table, NULL, env, &value);

    env_append(env, (const char *[]){ value, NULL });
    return env;
}

static int parse_config(const char *filename, const Specifier *table, char ***_ret)
{
    _cleanup_fclose_ FILE *fp = fopen(filename, "re");
    if (fp == NULL)
        return -1;

    while (!feof(fp)) {
        char p[LINE_MAX];

        if (!fgets(p, sizeof(p), fp)) {
            if (feof(fp))
                break;

            warn("failed to read configuration file '%s'", filename);
            return -errno;
        }

        truncate_to(p, "#\r\n");
        if (!*p)
            continue;

        *_ret = parse_line(p, table, *_ret);
    }

    return 0;
}

static int load_config(char ***_ret, const Specifier *table, const char *root, ...)
{
    va_list ap;
    _cleanup_free_ char *path;

    va_start(ap, root);
    path = joinpath_ap(root, ap);
    va_end(ap);

    return parse_config(path, table, _ret);
}

int main(void)
{
    _cleanup_free_ char **env = calloc(sizeof(char *), 100);

    printf("home dir:        %s\n", get_home_dir());
    printf("user config dir: %s\n\n", get_user_config_dir());

    struct passwd *pwd = getpwuid(getuid());
    if (!pwd)
        err(EXIT_FAILURE, "failed to get passwd entry for user");

    const Specifier table[] = {
        { 'p', specifier_string,   getenv("PATH") },
        { 'u', specifier_user_pwd, pwd },
        { 's', specifier_user_pwd, pwd },
        { 'h', specifier_user_pwd, pwd },
        { 0, NULL, NULL }
    };

    // Seed with default path and locale
    env_append(env, (const char *[]){ default_path, default_locale, NULL });

    // Merge the existing environment in with the working set
    /* env_append(env, (const char **)environ); */

    if (load_config(&env, table, get_user_config_dir(), "locale.conf", NULL) < 0)
        load_config(&env, table, "/etc/locale.conf", NULL);

    load_config(&env, table, get_home_dir(), ".pam_environment", NULL);

    char **e = env;
    for (; *e; ++e) {
        printf("%s\n", *e);
        free(*e);
    }
}
