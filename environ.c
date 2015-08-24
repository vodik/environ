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
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <err.h>
#include <dirent.h>
#include <assert.h>
#include <unistd.h>
#include <pwd.h>
#include <limits.h>

#include "env.h"
#include "specifier.h"
#include "xdg.h"
#include "util.h"

static const char *default_env[] = {
    "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin",
    "LANG=C",
    NULL
};

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
    if (!fp)
        return -1;

    printf("> loading: %s\n", filename);

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

static int load_dir(char ***_ret, const Specifier *table, const char *root, ...)
{
    va_list ap;
    _cleanup_free_ char *path;

    va_start(ap, root);
    path = joinpath_ap(root, ap);
    va_end(ap);

    _cleanup_closedir_ DIR *dir = opendir(path);
    if (!dir) {
        if (errno == ENOENT)
            return 0;
        return -errno;
    }

    for (;;) {
        struct dirent *de, buf;
        int r;

        r = readdir_r(dir, &buf, &de);
        if (r != 0)
            return -r;

        if (!de)
            break;

        if (de->d_name[0] == '.')
            continue;

        load_config(_ret, table, path, de->d_name, NULL);
    }

    return 0;
}

static void env_set_xdg_base_spec(char **env) {
    _cleanup_free_ char *xdg_config_home, *xdg_data_home, *xdg_cache_home;

    asprintf(&xdg_config_home, "XDG_CONFIG_HOME=%s", get_user_config_dir());
    asprintf(&xdg_data_home,   "XDG_DATA_HOME=%s",   get_user_data_dir());
    asprintf(&xdg_cache_home,  "XDG_CACHE_HOME=%s",  get_user_cache_dir());

    env_append(env, (const char *[]){
        xdg_config_home,
        xdg_data_home,
        xdg_cache_home,
        NULL
    });
}

/* static int cstr_cmp(const void *a, const void *b) */
/* { */
/*     return strcmp(*(const char **)a, *(const char **)b); */
/* } */

int main(void)
{
    _cleanup_free_ char **env = calloc(sizeof(char *), 100);

    printf("home dir:        %s\n", get_home_dir());
    printf("user config dir: %s\n", get_user_config_dir());
    printf("user data dir:   %s\n", get_user_data_dir());
    printf("user cache dir:  %s\n", get_user_cache_dir());

    struct passwd *pwd = getpwuid(getuid());
    if (!pwd)
        err(EXIT_FAILURE, "failed to get passwd entry for user");

    const Specifier table[] = {
        { 'p', specifier_string,   getenv("PATH") },
        { 'u', specifier_user_pwd, pwd },
        { 'U', specifier_user_pwd, pwd },
        { 's', specifier_user_pwd, pwd },
        { 'h', specifier_user_pwd, pwd },
        { 0, NULL, NULL }
    };

    // Seed with default path and locale
    env_append(env, default_env);
    /* env_append(env, (const char **)environ); */

    if (load_config(&env, table, get_user_config_dir(), "path.conf", NULL) < 0)
        load_config(&env, table, "/etc/path.conf", NULL);

    if (load_config(&env, table, get_user_config_dir(), "locale.conf", NULL) < 0)
        load_config(&env, table, "/etc/locale.conf", NULL);

    load_config(&env, table, "/etc/environment", NULL);
    load_dir(&env, table, "/usr/lib/env.d", NULL);
    load_dir(&env, table, "/etc/env.d", NULL);

    env_set_xdg_base_spec(env);

    load_config(&env, table, get_user_config_dir(), "environment", NULL);
    load_dir(&env, table, get_user_config_dir(), "env.d", NULL);
    load_config(&env, table, get_home_dir(), ".pam_environment", NULL);

    char **e;
    /* size_t n = 0; */

    /* for (e = env; *e; ++e, ++n) */
    /* qsort(env, n, sizeof(char *), cstr_cmp); */
    for (e = env; *e; ++e) {
        printf("%s\n", *e);
        free(*e);
    }
}
