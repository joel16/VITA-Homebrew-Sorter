/**
* \usergroup{SceSqlite}
* \usage{psp2/sqlite.h,SceSqlite_stub}
*/


#ifndef _PSP2_SQLITE_H_
#define _PSP2_SQLITE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	void *(*xMalloc)(int);
	void *(*xRealloc)(void*, int);
	void(*xFree)(void*);
} SceSqliteMallocMethods;

/**
 * Wrapper for sqlite3_config(SQLITE_CONFIG_MALLOC)
 *
 * @param[in] methods - A proper set of memory allocation methods
 *
 * @return 0 on success, < 0 on error.
 */
int sceSqliteConfigMallocMethods(SceSqliteMallocMethods* methods);

/**
 * Initialize the Vita SQLite library for RW access.
 * This is achieved by overriding the default SQlite VFS ("psp2") to
 * add RW calls, and registering it as a new default VFS ("psp2_rw").
 *
 * Note that calling this function also automatically calls on
 * sceSqliteConfigMallocMethods() to initialize the malloc methods
 * and that you still need to provide your own `sqlite3.h` header, in
 * your application, before you can access the usual sqlite3 functions.
 *
 * @return SQLITE_OK (0) on success or an SQLITE_### code on error
 */
int sqlite3_rw_init();

/**
 * Release any resources allocated in sqlite3_rw_init().
 *
 * @return SQLITE_OK (0) on success or an SQLITE_### code on error
 */
int sqlite3_rw_exit();

#ifdef __cplusplus
}
#endif

#endif /* _PSP2_SQLITE_H_ */
