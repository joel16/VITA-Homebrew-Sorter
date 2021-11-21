#include <psp2/io/fcntl.h>
#include <psp2/kernel/clib.h>
#include <psp2/sqlite.h>
#include <cstdlib>

#include "sqlite.h"

// Heavily based on https://github.com/VitaSmith/libsqlite/blob/master/libsqlite/sqlite.c with c++ style casting and sceClib usage.

/*
 * From sqlite3.h
 */
#ifndef SQLITE_EXTERN
# define SQLITE_EXTERN extern
#endif

#ifndef SQLITE_API
# define SQLITE_API
#endif

#define SQLITE_OK                      0
#define SQLITE_IOERR                   10

#define SQLITE_IOERR_READ              (SQLITE_IOERR | (1<<8))
#define SQLITE_IOERR_SHORT_READ        (SQLITE_IOERR | (2<<8))
#define SQLITE_IOERR_WRITE             (SQLITE_IOERR | (3<<8))
#define SQLITE_IOERR_FSYNC             (SQLITE_IOERR | (4<<8))
#define SQLITE_IOERR_DIR_FSYNC         (SQLITE_IOERR | (5<<8))
#define SQLITE_IOERR_TRUNCATE          (SQLITE_IOERR | (6<<8))
#define SQLITE_IOERR_FSTAT             (SQLITE_IOERR | (7<<8))
#define SQLITE_IOERR_UNLOCK            (SQLITE_IOERR | (8<<8))
#define SQLITE_IOERR_RDLOCK            (SQLITE_IOERR | (9<<8))
#define SQLITE_IOERR_DELETE            (SQLITE_IOERR | (10<<8))
#define SQLITE_IOERR_BLOCKED           (SQLITE_IOERR | (11<<8))
#define SQLITE_IOERR_NOMEM             (SQLITE_IOERR | (12<<8))
#define SQLITE_IOERR_ACCESS            (SQLITE_IOERR | (13<<8))
#define SQLITE_IOERR_CHECKRESERVEDLOCK (SQLITE_IOERR | (14<<8))
#define SQLITE_IOERR_LOCK              (SQLITE_IOERR | (15<<8))
#define SQLITE_IOERR_CLOSE             (SQLITE_IOERR | (16<<8))
#define SQLITE_IOERR_DIR_CLOSE         (SQLITE_IOERR | (17<<8))
#define SQLITE_IOERR_SHMOPEN           (SQLITE_IOERR | (18<<8))
#define SQLITE_IOERR_SHMSIZE           (SQLITE_IOERR | (19<<8))
#define SQLITE_IOERR_SHMLOCK           (SQLITE_IOERR | (20<<8))
#define SQLITE_IOERR_SHMMAP            (SQLITE_IOERR | (21<<8))
#define SQLITE_IOERR_SEEK              (SQLITE_IOERR | (22<<8))
#define SQLITE_LOCKED_SHAREDCACHE      (SQLITE_LOCKED |  (1<<8))
#define SQLITE_BUSY_RECOVERY           (SQLITE_BUSY   |  (1<<8))
#define SQLITE_CANTOPEN_NOTEMPDIR      (SQLITE_CANTOPEN | (1<<8))
#define SQLITE_CORRUPT_VTAB            (SQLITE_CORRUPT | (1<<8))
#define SQLITE_READONLY_RECOVERY       (SQLITE_READONLY | (1<<8))
#define SQLITE_READONLY_CANTLOCK       (SQLITE_READONLY | (2<<8))

#define SQLITE_OPEN_READONLY           0x00000001
#define SQLITE_OPEN_READWRITE          0x00000002
#define SQLITE_OPEN_CREATE             0x00000004
#define SQLITE_OPEN_URI                0x00000040
#define SQLITE_OPEN_NOMUTEX            0x00008000
#define SQLITE_OPEN_FULLMUTEX          0x00010000
#define SQLITE_OPEN_SHAREDCACHE        0x00020000
#define SQLITE_OPEN_PRIVATECACHE       0x00040000

#ifdef SQLITE_INT64_TYPE
typedef SQLITE_INT64_TYPE sqlite_int64;
typedef unsigned SQLITE_INT64_TYPE sqlite_uint64;
#else
typedef long long int sqlite_int64;
typedef unsigned long long int sqlite_uint64;
#endif
typedef sqlite_int64 sqlite3_int64;
typedef sqlite_uint64 sqlite3_uint64;

typedef struct sqlite3_file sqlite3_file;
struct sqlite3_file {
    const struct sqlite3_io_methods *pMethods;
};

typedef struct sqlite3_io_methods sqlite3_io_methods;
struct sqlite3_io_methods {
    int iVersion;
    int(*xClose)(sqlite3_file*);
    int(*xRead)(sqlite3_file *, void *, int, sqlite3_int64);
    int(*xWrite)(sqlite3_file *, const void *, int, sqlite3_int64);
    int(*xTruncate)(sqlite3_file *, sqlite3_int64);
    int(*xSync)(sqlite3_file *, int);
    int(*xFileSize)(sqlite3_file *, sqlite3_int64 *);
    int(*xLock)(sqlite3_file *, int);
    int(*xUnlock)(sqlite3_file *, int);
    int(*xCheckReservedLock)(sqlite3_file *, int *);
    int(*xFileControl)(sqlite3_file *, int, void *);
    int(*xSectorSize)(sqlite3_file *);
    int(*xDeviceCharacteristics)(sqlite3_file *);
    int(*xShmMap)(sqlite3_file *, int, int, int, void volatile **);
    int(*xShmLock)(sqlite3_file *, int, int, int);
    void(*xShmBarrier)(sqlite3_file *);
    int(*xShmUnmap)(sqlite3_file *, int);
};

typedef struct sqlite3_vfs sqlite3_vfs;
typedef void(*sqlite3_syscall_ptr)(void);
struct sqlite3_vfs {
    int iVersion;
    int szOsFile;
    int mxPathname;
    sqlite3_vfs *pNext;
    const char *zName;
    void *pAppData;
    int(*xOpen)(sqlite3_vfs *, const char *zName, sqlite3_file *, int flags, int *pOutFlags);
    int(*xDelete)(sqlite3_vfs *, const char *zName, int syncDir);
    int(*xAccess)(sqlite3_vfs *, const char *zName, int flags, int *pResOut);
    int(*xFullPathname)(sqlite3_vfs *, const char *zName, int nOut, char *zOut);
    void *(*xDlOpen)(sqlite3_vfs *, const char *zFilename);
    void(*xDlError)(sqlite3_vfs *, int nByte, char *zErrMsg);
    void(*(*xDlSym)(sqlite3_vfs *, void *, const char *zSymbol))(void);
    void(*xDlClose)(sqlite3_vfs *, void *);
    int(*xRandomness)(sqlite3_vfs *, int nByte, char *zOut);
    int(*xSleep)(sqlite3_vfs *, int microseconds);
    int(*xCurrentTime)(sqlite3_vfs *, double *);
    int(*xGetLastError)(sqlite3_vfs *, int, char *);
    int(*xCurrentTimeInt64)(sqlite3_vfs *, sqlite3_int64 *);
    int(*xSetSystemCall)(sqlite3_vfs *, const char *zName, sqlite3_syscall_ptr);
    sqlite3_syscall_ptr(*xGetSystemCall)(sqlite3_vfs *, const char *zName);
    const char *(*xNextSystemCall)(sqlite3_vfs *, const char *zName);
};

extern "C" {
    SQLITE_API sqlite3_vfs *sqlite3_vfs_find(const char *);
    SQLITE_API int sqlite3_vfs_register(sqlite3_vfs *, int);
    SQLITE_API int sqlite3_vfs_unregister(sqlite3_vfs *);
}

/*
 * End of data from sqlite3.h
 */

//#define VERBOSE 1
#if VERBOSE
extern int sceClibPrintf(const char *format, ...);
#define LOG sceClibPrintf
#else
#define LOG(...)
#endif

#define IS_ERROR(x) ((unsigned)x & 0x80000000)

namespace SQLite {
    static sqlite3_io_methods *rw_methods = nullptr;
    static sqlite3_vfs *rw_vfs = nullptr;
    
    // Internal file structure used by Sony's VFS
    typedef struct {
        sqlite3_file file;
        SceUID *fd;
    } vfs_file;
    
    static int Write(sqlite3_file *file, const void *buf, int count, sqlite_int64 offset) {
        vfs_file *p = reinterpret_cast<vfs_file *>(file);
        int seek = sceIoLseek(*p->fd, offset, SCE_SEEK_SET);
        
        LOG("seek %08x %x => %x\n", *p->fd, offset, seek);
        
        if (seek != offset)
            return SQLITE_IOERR_WRITE;
            
        int write = sceIoWrite(*p->fd, buf, count);
        
        LOG("write %08x %08x %x => %x\n", *p->fd, buf, count, write);
        
        if (write != count) {
            LOG("write error %08x\n", write);
            return SQLITE_IOERR_WRITE;
        }
        
        return SQLITE_OK;
    }
    
    static int Sync(sqlite3_file *file, int flags) {
        vfs_file *p = reinterpret_cast<vfs_file *>(file);
        int ret = sceIoSyncByFd(*p->fd, 0);
        
        LOG("sync %x, %x => %x\n", *p->fd, flags, r);
        
        if (IS_ERROR(ret))
            return SQLITE_IOERR_FSYNC;
            
        return SQLITE_OK;
    }
    
    static int Open(sqlite3_vfs *vfs, const char *name, sqlite3_file *file, int flags, int *out_flags) {
        // sqlite can call open with a NULL name, we need to create a temporary file then
        char tempbuf[] = "ux0:data/sqlite-tmp-XXXXXX";
        if (name == nullptr)
            mkstemp(tempbuf);
            
        sqlite3_vfs *org_vfs = reinterpret_cast<sqlite3_vfs *>(vfs->pAppData);
        
        LOG("open %s: flags = %08x, ", (name == nullptr)? tempbuf : name, flags);
        
        // Default xOpen() does not create files => do that ourselves
        // TODO: handle SQLITE_OPEN_EXCLUSIVE
        if (flags & SQLITE_OPEN_CREATE) {
            SceUID fd = sceIoOpen((name == nullptr)? tempbuf : name, SCE_O_RDONLY, 0777);
            if (IS_ERROR(fd))
                fd = sceIoOpen((name == nullptr)? tempbuf : name, SCE_O_WRONLY | SCE_O_CREAT, 0777);
            
            if (!IS_ERROR(fd))
                sceIoClose(fd);
            else
                LOG("create error: %08x\n", fd);
        }
        
        // Call the original xOpen()
        int ret = org_vfs->xOpen(org_vfs, (name == nullptr)? tempbuf : name, file, flags, out_flags);
        vfs_file *p = reinterpret_cast<vfs_file *>(file);
        
        LOG("fd = %08x, r = %d\n", (p == nullptr) ? 0 : *p->fd, r);
        
        // Default xOpen() also forces read-only on SQLITE_OPEN_READWRITE files
        if ((file->pMethods != nullptr) && (flags & SQLITE_OPEN_READWRITE)) {
            if (!IS_ERROR(*p->fd)) {
                // Reopen the file with write access
                sceIoClose(*p->fd);
                *p->fd = sceIoOpen((name == nullptr)? tempbuf : name, SCE_O_RDWR, 0777);
                
                LOG("override fd = %08x\n", *p->fd);
                
                if (IS_ERROR(*p->fd))
                    return SQLITE_IOERR_WRITE;
            }
            
            // Need to override xWrite() and xSync() as well
            if (rw_methods == nullptr) {
                rw_methods = static_cast<sqlite3_io_methods *>(std::malloc(sizeof(sqlite3_io_methods)));

                if (rw_methods != nullptr) {
                    sceClibMemcpy(rw_methods, file->pMethods, sizeof(sqlite3_io_methods));
                    rw_methods->xWrite = SQLite::Write;
                    rw_methods->xSync = SQLite::Sync;
                }
            }
            
            if (rw_methods != nullptr)
                file->pMethods = rw_methods;
        }
        
        return ret;
    }
    
    static int Delete(sqlite3_vfs *vfs, const char *name, int syncDir) {
        int ret = sceIoRemove(name);
        
        LOG("delete %s: 0x%08x\n", name, ret);
        
        if (IS_ERROR(ret))
            return SQLITE_IOERR_DELETE;
            
        return SQLITE_OK;
    }

    int Init(void) {
        int ret = SQLITE_OK;
        
        if (rw_vfs != nullptr)
            return SQLITE_OK;
            
        SceSqliteMallocMethods mf = {
            (void* (*) (int)) std::malloc,
            (void* (*) (void*, int)) std::realloc,
            std::free
        };
        
        sceSqliteConfigMallocMethods(&mf);
        rw_vfs = static_cast<sqlite3_vfs *>(std::malloc(sizeof(sqlite3_vfs)));
        sqlite3_vfs *vfs = sqlite3_vfs_find(nullptr);
        
        if ((vfs != nullptr) && (rw_vfs != nullptr)) {
            // Override xOpen() and xDelete()
            sceClibMemcpy(rw_vfs, vfs, sizeof(sqlite3_vfs));
            rw_vfs->zName = "psp2_rw";
            rw_vfs->xOpen = SQLite::Open;
            rw_vfs->xDelete = SQLite::Delete;

            // Keep a copy of the original VFS pointer
            rw_vfs->pAppData = vfs;
            ret = sqlite3_vfs_register(rw_vfs, 1);
            
            if (ret != SQLITE_OK) {
                LOG("sqlite_rw_init: could not register vfs: %d\n", ret);
                return ret;
            }
        }
        
        return SQLITE_OK;
    }

    int Exit(void) {
        int ret = SQLITE_OK;
        std::free(rw_methods);
        rw_methods = nullptr;
        
        if (rw_vfs != nullptr) {
            ret = sqlite3_vfs_unregister(rw_vfs);
            if (ret != SQLITE_OK)
                LOG("sqlite_rw_exit: error unregistering vfs:  %d\n", ret);
                
            std::free(rw_vfs);
            rw_vfs = nullptr;
        }
        
        return ret;
    }
}
