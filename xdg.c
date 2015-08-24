#include "xdg.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <err.h>

#include "util.h"

static char *home_dir_cache = NULL;
static char *user_config_dir_cache = NULL;
static char *user_data_dir_cache = NULL;
static char *user_cache_dir_cache = NULL;

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

    if (xdg_dir && xdg_dir[0]) {
        xdg_dir = strdup(xdg_dir);
    } else {
        xdg_dir = joinpath(get_home_dir(), default_path, NULL);
    }

    return xdg_dir;
}

const char *get_user_config_dir(void)
{
    if (!user_config_dir_cache) {
        user_config_dir_cache = get_xdg_dir("XDG_CONFIG_HOME", ".config");
    }

    return user_config_dir_cache;
}

const char *get_user_data_dir(void)
{
    if (!user_data_dir_cache) {
        user_data_dir_cache = get_xdg_dir("XDG_DATA_HOME", ".local/share");
    }

    return user_data_dir_cache;
}

const char *get_user_cache_dir(void)
{
    if (!user_cache_dir_cache)
        user_cache_dir_cache = get_xdg_dir("XDG_CACHE_HOME", ".cache");
    return user_cache_dir_cache;
}

#ifdef VALGRIND
static _destructor_ void free_dirs(void)
{
    free(home_dir_cache);
    free(user_config_dir_cache);
    free(user_data_dir_cache);
    free(user_cache_dir_cache);
}
#endif
