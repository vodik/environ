## pam_environ

WIP alternative to pam_env.so. Hopefully can replace the reliance on
`/etc/profile` for environment that isn't really shell specific.

Reads the following files and merges them down into (hopefully) a full
environment:

1. `$XDG_CONFIG_DIR/path.conf` or `/etc/path.conf` to load the
   preferred path.
2. `$XDG_CONFIG_DIR/locale.conf` or `/etc/locale.conf` to load the
   preferred locale.
3. load every file inside `/usr/lib/env.d` ending with `.env`.
4. Load every file inside `/etc/env.d` ending with `.env`.
5. ???
6. `$HOME/.pam_environment` (for legacy support).

### Substition

Support for variable substitution and specifiers:

    PATH=%h/bin:%(PATH)

`%h` expands to the user's home directory, `%(PATH)` expands to the
existing value for `PATH`.

### Open Questions

- How to handle perl?
