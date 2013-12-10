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

static char *truncate_comment(char *s)
{
    s[strcspn(s, "#")] = 0;
    return s;
}

static char **parse_line(const char *line, const Specifier *table, char **env)
{
    _cleanup_free_ char *value = NULL;
    specifier_printf(line, table, NULL, environ, &value);

    env_append(env, (const char *[]){ value, NULL });
    return env;
}

static int config_parse(const char *filename, const Specifier *table, char ***_ret)
{
    _cleanup_fclose_ FILE *fp = fopen(filename, "re");
    if (fp == NULL)
        err(EXIT_FAILURE, "failed to open %s", filename);

    unsigned line = 0;

    while (!feof(fp)) {
        char p[LINE_MAX];

        if (!fgets(p, sizeof(p), fp)) {
            if (feof(fp))
                break;

            warn("Failed to read configuration file '%s'", filename);
            return -errno;
        }

        truncate_nl(p);
        printf("LINE %02d: %s\n", ++line, p);

        truncate_comment(p);
        if (!*p)
            continue;

        *_ret = parse_line(p, table, *_ret);
    }

    return 0;
}

int main(void)
{
    _cleanup_free_ char **env = calloc(sizeof(char *), 100);
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

    _cleanup_free_ char *pam_env;
    asprintf(&pam_env, "%s/.pam_environment", pwd->pw_dir);

    config_parse(pam_env, table, &env);

    char **e = env;
    for (; *e; ++e) {
        printf("%s\n", *e);
        free(*e);
    }
}
