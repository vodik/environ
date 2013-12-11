#include "util.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <pwd.h>

static char *home_dir_cache = NULL;
static char *user_config_dir_cache = NULL;
static char *user_data_dir_cache = NULL;
static char *user_cache_dir_cache = NULL;

char *strnappend(const char *s, const char *suffix, size_t b)
{
    size_t a;
    char *r;

    if (!s && !suffix)
        return strdup("");

    if (!s)
        return strndup(suffix, b);

    if (!suffix)
        return strdup(s);

    assert(s);
    assert(suffix);

    a = strlen(s);
    if (b > ((size_t) -1) - a)
        return NULL;

    r = malloc(a + b + 1);
    if (!r)
        return NULL;

    memcpy(r, s, a);
    memcpy(r + a, suffix, b);
    r[a + b] = 0;

    return r;
}

char *strappend(const char *s, const char *suffix)
{
    return strnappend(s, suffix, suffix ? strlen(suffix) : 0);
}

char *truncate_to(char *s, const char *term)
{
    assert(s);
    assert(term);

    s[strcspn(s, term)] = 0;
    return s;
}

/* loosely adopted from systemd shared/util.c */
char *joinpath_ap(const char *root, va_list ap)
{
    size_t len;
    char *ret, *p;
    const char *temp;

    va_list aq;
    va_copy(aq, ap);

    if (!root)
        return NULL;

    len = strlen(root);
    while ((temp = va_arg(ap, const char *))) {
        size_t temp_len = strlen(temp) + 1;
        if (temp_len > ((size_t) -1) - len) {
            return NULL;
        }

        len += temp_len;
    }

    ret = malloc(len + 1);
    if (ret) {
        p = stpcpy(ret, root);
        while ((temp = va_arg(aq, const char *))) {
            p++[0] = '/';
            p = stpcpy(p, temp);
        }
    }

    return ret;
}

char *joinpath(const char *root, ...)
{
    va_list ap;
    char *ret;

    va_start(ap, root);
    ret = joinpath_ap(root, ap);
    va_end(ap);

    return ret;
}

const char *get_home_dir(void)
{
    if (!home_dir_cache) {
        struct passwd *pwd = getpwuid(getuid());
        if (!pwd)
            err(EXIT_FAILURE, "failed to get pwd entry for user");

        home_dir_cache = strdup(pwd->pw_dir);
    }

    return home_dir_cache;

}

static char *get_xdg_dir(const char *env, char *default_path)
{
    char *xdg_dir = getenv(env);

    if (xdg_dir && xdg_dir[0])
        xdg_dir = strdup(xdg_dir);
    else
        xdg_dir = joinpath(get_home_dir(), default_path, NULL);

    return xdg_dir;
}

const char *get_user_config_dir(void)
{
    if (!user_config_dir_cache)
        user_config_dir_cache = get_xdg_dir("XDG_CONFIG_HOME", ".config");
    return user_config_dir_cache;
}

const char *get_user_data_dir(void)
{
    if (!user_data_dir_cache)
        user_data_dir_cache = get_xdg_dir("XDG_DATA_HOME", ".local/share");
    return user_data_dir_cache;
}

const char *get_user_cache_dir(void)
{
    if (!user_cache_dir_cache)
        user_cache_dir_cache = get_xdg_dir("XDG_CACHE_HOME", ".cache");
    return user_cache_dir_cache;
}

#ifdef VALGRIND
_destructor_ static void free_dirs(void)
{
    free(home_dir_cache);
    free(user_config_dir_cache);
    free(user_data_dir_cache);
    free(user_cache_dir_cache);
}
#endif
