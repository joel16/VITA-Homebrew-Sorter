# libsqlite

## libsqlite
An SQlite 3 library, with unlocked R/W access, for PS Vita.

## Compiling
Prerequisites:
* [VitaSDK](https://github.com/vitasdk/buildscripts)

Once all the prerequisites have been met, you can compile this library by typing:
```
cd libsqlite
make
```

Note that if you choose to run `make install`, the SDK's `include/psp2/sqlite.h`
will be overwritten with this repository's version (which adds `sqlite3_rw_init()`
and `sqlite3_rw_exit()` to it).

A demo SQLite application is also provided in the `sample/` directory.
This demo, which doesn't require any special permissions, will create or update
a sample database residing at `ux0:data/sqlite_demo.db`.

## Credits
* Thanks to __xyzz__ for the original VFS override code.
* Thanks to __xerpi__ and his [libftpvita](https://github.com/xerpi/libftpvita),
  which I used as repo and Makefile template.
* Thanks to everybody who contributed to vita-toolchain.
