#include "util.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <pwd.h>

static char *home_dir_cache = NULL;
static char *user_config_dir_cache = NULL;

static bool home_dir_cached = false;
static bool user_config_dir_cached = false;

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
static char *joinpath_ap(const char *root, va_list ap)
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

    ret = malloc(len);
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
    if (!home_dir_cached) {
        struct passwd *pwd = getpwuid(getuid());
        if (!pwd)
            err(EXIT_FAILURE, "failed to get pwd entry for user");

        home_dir_cache = strdup(pwd->pw_dir);
        home_dir_cached = true;
    }

    return home_dir_cache;

}

const char *get_user_config_dir(void)
{
    if (!user_config_dir_cached) {
        char *config_dir = getenv("XDG_CONFIG_HOME");

        if (config_dir && config_dir[0]) {
            config_dir = strdup(config_dir);
        } else {
            const char *home_dir = get_home_dir();

            if (home_dir)
                config_dir = joinpath(home_dir, ".config", NULL);
        }

        user_config_dir_cache = config_dir;
        user_config_dir_cached = true;
    }

    return user_config_dir_cache;
}
