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
#include <assert.h>
#include <unistd.h>
#include <pwd.h>

#include "env.h"
#include "specifier.h"
#include "util.h"

int main(void)
{
    _cleanup_free_ char **env = calloc(sizeof(char *), 100);
    struct passwd *pwd = getpwuid(getuid());

    const Specifier table[] = {
        { 'p', specifier_string,   getenv("PATH") },
        { 'u', specifier_user_pwd, pwd },
        { 's', specifier_user_pwd, pwd },
        { 'h', specifier_user_pwd, pwd },
        { 0, NULL, NULL }
    };

    _cleanup_free_ char *pam_env;
    asprintf(&pam_env, "%s/.pam_environment", pwd->pw_dir);

    _cleanup_fclose_ FILE *fp = fopen(pam_env, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    ssize_t nbytes_r;
    size_t len = 0;
    _cleanup_free_ char *line = NULL;

    while ((nbytes_r = getline(&line, &len, fp)) != -1) {
        if (!line || *line == '#' || *line == '\n')
            continue;

        line[nbytes_r - 1] = 0;

        _cleanup_free_ char *value = NULL;
        specifier_printf(line, table, NULL, environ, &value);

        env_append(env, (const char *[]){ value, NULL });
    }

    char **e = env;
    for (; *e; ++e) {
        printf("%s\n", *e);
        free(*e);
    }
}
