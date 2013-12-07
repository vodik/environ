#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <pwd.h>

#define _unused_          __attribute__((unused))
#define _cleanup_(x)      __attribute__((cleanup(x)))

#define _cleanup_free_    _cleanup_(freep)
#define _cleanup_fclose_  _cleanup_(fclosep)

static inline void freep(void *p) { free(*(void **)p); }
static inline void fclosep(FILE **fp) { fclose(*fp); }

static inline bool streq(const char *s1, const char *s2) { return strcmp(s1, s2) == 0; }
static inline bool strneq(const char *s1, const char *s2, size_t n) { return strncmp(s1, s2, n) == 0; }
static inline const char* strempty(const char *s) { return s ? s : ""; }

typedef int (*SpecifierCallback)(char specifier, void *data, void *userdata, char **ret);

typedef struct Specifier {
    const char specifier;
    const SpecifierCallback lookup;
    void *data;
} Specifier;

//{{{ SPECIFIERS
int specifier_user_pwd(char specifier, void _unused_ *data, void *userdata, char **ret)
{
    uid_t *uid = userdata;
    struct passwd *pwd = getpwuid(*uid);
    char *n = NULL;

    switch (specifier) {
    case 'u':
        n = strdup(pwd->pw_name);
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
//}}}

static size_t env_key_length(const char *key)
{
    size_t n = strcspn(key, "=");
    if (key[n] == '=')
        n++;
    return n;
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

static int env_append(char **env, const char **keys)
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

int main(void)
{
    const Specifier table[] = {
        { 'p', specifier_string,   getenv("PATH") },
        { 'u', specifier_user_pwd, NULL },
        { 's', specifier_user_pwd, NULL },
        { 'h', specifier_user_pwd, NULL },
        { 0, NULL, NULL }
    };

    char **env = calloc(sizeof(char *), 100);

    size_t len = 0;
    ssize_t nbytes_r;
    _cleanup_fclose_ FILE *fp = fopen("/home/simon/.pam_environment", "r");
    _cleanup_free_ char *line = NULL;

    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((nbytes_r = getline(&line, &len, fp)) != -1) {
        if (!line || *line == '#' || *line == '\n')
            continue;

        line[nbytes_r - 1] = 0;

        _cleanup_free_ char *value = NULL;
        specifier_printf(line, table, &(uid_t){ 1000 }, env, &value);

        env_append(env, (const char *[]){ value, NULL });
    }

    char **e = env;
    for (; *e; ++e) {
        printf("%s\n", *e);
        free(*e);
    }

    free(env);
}
