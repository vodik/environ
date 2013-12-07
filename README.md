WIP alternative to pam_env.so

Support for variable substitution and specifiers:

    PATH=%h/bin:%(PATH)

`%h` expands to the user's home directory, `%(PATH)` expands to the
existing value for `PATH`.
