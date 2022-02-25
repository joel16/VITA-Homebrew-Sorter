#include "sqlite3.h"

#include <assert.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/rtc.h>
//#include <unistd.h>

#define MAXPATHNAME 512

/*
** Size of the write buffer used by journal files in bytes.
*/
#ifndef SQLITE_PSP2VFS_BUFFERSZ
# define SQLITE_PSP2VFS_BUFFERSZ 8192
#endif

/*
** The maximum pathname length supported by this VFS.
*/
#define MAXPATHNAME 512

/*
** When using this VFS, the sqlite3_file* handles that SQLite uses are
** actually pointers to instances of type PSP2File.
*/
typedef struct PSP2File PSP2File;

struct PSP2File {
    sqlite3_file base = {0};            /* Base class. Must be first. */
    SceUID fd = 0;                      /* File descriptor */
    char *aBuffer = nullptr;            /* Pointer to malloc'd buffer */
    int nBuffer = 0;                    /* Valid bytes of data in zBuffer */
    sqlite3_int64 iBufferOfst = 0;      /* Offset in file of zBuffer[0] */
};

/*
** Write directly to the file passed as the first argument. Even if the
** file has a write-buffer (PSP2File.aBuffer), ignore it.
*/
static int psp2DirectWrite(PSP2File *p, const void *zBuf, int iAmt, sqlite_int64 iOfst) {
    off_t ofst = 0;                    /* Return value from sceIoLseek() */
    int nWrite = 0;                    /* Return value from sceIoWrite() */
    
    ofst = sceIoLseek(p->fd, iOfst, SCE_SEEK_SET);
    if (ofst != iOfst)
        return SQLITE_IOERR_WRITE;
        
    nWrite = sceIoWrite(p->fd, zBuf, iAmt);
    if (nWrite != iAmt)
        return SQLITE_IOERR_WRITE;
        
    return SQLITE_OK;
}

/*
** Flush the contents of the PSP2File.aBuffer buffer to disk. This is a
** no-op if this particular file does not have a buffer (i.e. it is not
** a journal file) or if the buffer is currently empty.
*/
static int psp2FlushBuffer(PSP2File *p) {
    int rc = SQLITE_OK;
    
    if (p->nBuffer) {
        rc = psp2DirectWrite(p, p->aBuffer, p->nBuffer, p->iBufferOfst);
        p->nBuffer = 0;
    }
    
    return rc;
}

/*
** Close a file.
*/
static int psp2Close(sqlite3_file *pFile) {
    int rc = 0;
    PSP2File *p = reinterpret_cast<PSP2File*>(pFile);
    rc = psp2FlushBuffer(p);
    sqlite3_free(p->aBuffer);
    sceIoClose(p->fd);
    return rc;
}

/*
** Read data from a file.
*/
static int psp2Read(sqlite3_file *pFile, void *zBuf, int iAmt, sqlite_int64 iOfst) {
    PSP2File *p = reinterpret_cast<PSP2File*>(pFile);
    off_t ofst = 0;                     /* Return value from sceIoLseek() */
    int nRead = 0;                      /* Return value from sceIoRead() */
    int rc = 0;                         /* Return code from psp2FlushBuffer() */
    
    /* Flush any data in the write buffer to disk in case this operation
    ** is trying to read data the file-region currently cached in the buffer.
    ** It would be possible to detect this case and possibly save an 
    ** unnecessary write here, but in practice SQLite will rarely read from
    ** a journal file when there is data cached in the write-buffer.
    */
    rc = psp2FlushBuffer(p);
    if (rc != SQLITE_OK)
        return rc;
    
    ofst = sceIoLseek(p->fd, iOfst, SCE_SEEK_SET);
    if (ofst != iOfst)
        return SQLITE_IOERR_READ;
        
    nRead = sceIoRead(p->fd, zBuf, iAmt);
    
    if (nRead == iAmt)
        return SQLITE_OK;
    else if (nRead >= 0) {
        if (nRead < iAmt)
            sceClibMemset(&(static_cast<char*>(zBuf))[nRead], 0, iAmt-nRead);
        
        return SQLITE_IOERR_SHORT_READ;
    }
    
    return SQLITE_IOERR_READ;
}

/*
** Write data to a crash-file.
*/
static int psp2Write(sqlite3_file *pFile, const void *zBuf, int iAmt, sqlite_int64 iOfst) {
    PSP2File *p = reinterpret_cast<PSP2File*>(pFile);
    
    if (p->aBuffer) {
        char *z = const_cast<char*>(static_cast<const char *>(zBuf));  /* Pointer to remaining data to write */
        int n = iAmt;                                                  /* Number of bytes at z */
        sqlite3_int64 i = iOfst;                                       /* File offset to write to */
        
        while(n > 0) {
            int nCopy;                                                 /* Number of bytes to copy into buffer */
            
            /* If the buffer is full, or if this data is not being written directly
            ** following the data already buffered, flush the buffer. Flushing
            ** the buffer is a no-op if it is empty.  
            */
            if (p->nBuffer == SQLITE_PSP2VFS_BUFFERSZ || p->iBufferOfst+p->nBuffer != i) {
                int rc = psp2FlushBuffer(p);
                if (rc != SQLITE_OK)
                    return rc;
            }
            
            assert(p->nBuffer == 0 || p->iBufferOfst+p->nBuffer == i);
            p->iBufferOfst = i - p->nBuffer;
            
            /* Copy as much data as possible into the buffer. */
            nCopy = SQLITE_PSP2VFS_BUFFERSZ - p->nBuffer;
            if (nCopy > n)
                nCopy = n;
                
            sceClibMemcpy(&p->aBuffer[p->nBuffer], z, nCopy);
            p->nBuffer += nCopy;
            
            n -= nCopy;
            i += nCopy;
            z += nCopy;
        }
    }
    else
        return psp2DirectWrite(p, zBuf, iAmt, iOfst);
        
    return SQLITE_OK;
}

/*
** Truncate a file. This is a no-op for this VFS (see header comments at
** the top of the file).
*/
static int psp2Truncate(sqlite3_file *pFile, sqlite_int64 size) {
    return SQLITE_OK;
}

/*
** Sync the contents of the file to the persistent media.
*/
static int psp2Sync(sqlite3_file *pFile, int flags) {
    PSP2File *p = reinterpret_cast<PSP2File*>(pFile);
    int rc = 0;
    
    rc = psp2FlushBuffer(p);
    if (rc != SQLITE_OK)
        return rc;
    
    rc = sceIoSyncByFd(p->fd, 0);
    return (rc == 0? SQLITE_OK : SQLITE_IOERR_FSYNC);
}

/*
** Write the size of the file in bytes to *pSize.
*/
static int psp2FileSize(sqlite3_file *pFile, sqlite_int64 *pSize) {
    PSP2File *p = reinterpret_cast<PSP2File*>(pFile);
    int rc = 0;                     /* Return code from sceIoGetstatByFd() call */
    SceIoStat sStat = {0};          /* Output of sceIoGetstatByFd() call */
    
    /* Flush the contents of the buffer to disk. As with the flush in the
    ** psp2Read() method, it would be possible to avoid this and save a write
    ** here and there. But in practice this comes up so infrequently it is
    ** not worth the trouble.
    */
    rc = psp2FlushBuffer(p);
    if (rc != SQLITE_OK)
        return rc;
        
    rc = sceIoGetstatByFd(p->fd, &sStat);
    
    if (rc != 0)
        return SQLITE_IOERR_FSTAT;
        
    *pSize = sStat.st_size;
    return SQLITE_OK;
}

/*
** Locking functions. The xLock() and xUnlock() methods are both no-ops.
** The xCheckReservedLock() always indicates that no other process holds
** a reserved lock on the database file. This ensures that if a hot-journal
** file is found in the file-system it is rolled back.
*/
static int psp2Lock(sqlite3_file *pFile, int eLock) {
    return SQLITE_OK;
}

static int psp2Unlock(sqlite3_file *pFile, int eLock) {
    return SQLITE_OK;
}

static int psp2CheckReservedLock(sqlite3_file *pFile, int *pResOut) {
    *pResOut = 0;
    return SQLITE_OK;
}

/*
** No xFileControl() verbs are implemented by this VFS.
*/
static int psp2FileControl(sqlite3_file *pFile, int op, void *pArg) {
    return SQLITE_NOTFOUND;
}

/*
** The xSectorSize() and xDeviceCharacteristics() methods. These two
** may return special values allowing SQLite to optimize file-system 
** access to some extent. But it is also safe to simply return 0.
*/
static int psp2SectorSize(sqlite3_file *pFile) {
    return 0;
}

static int psp2DeviceCharacteristics(sqlite3_file *pFile) {
    return 0;
}

/*
** Open a file handle.
*/
static int psp2Open(sqlite3_vfs *pVfs, const char *zName, sqlite3_file *pFile, int flags, int *pOutFlags) {
    static const sqlite3_io_methods psp2io = {
        1,                            /* iVersion */
        psp2Close,                    /* xClose */
        psp2Read,                     /* xRead */
        psp2Write,                    /* xWrite */
        psp2Truncate,                 /* xTruncate */
        psp2Sync,                     /* xSync */
        psp2FileSize,                 /* xFileSize */
        psp2Lock,                     /* xLock */
        psp2Unlock,                   /* xUnlock */
        psp2CheckReservedLock,        /* xCheckReservedLock */
        psp2FileControl,              /* xFileControl */
        psp2SectorSize,               /* xSectorSize */
        psp2DeviceCharacteristics     /* xDeviceCharacteristics */
    };

    PSP2File *p = reinterpret_cast<PSP2File*>(pFile); /* Populate this structure */
    int oflags = 0;                                   /* flags to pass to open() call */
    char *aBuf = 0;

    if (zName == 0)
        return SQLITE_IOERR;
        
    if (flags & SQLITE_OPEN_MAIN_JOURNAL) {
        aBuf = reinterpret_cast<char*>(sqlite3_malloc(SQLITE_PSP2VFS_BUFFERSZ));

        if (!aBuf)
            return SQLITE_NOMEM;
    }
    
    if (flags & SQLITE_OPEN_EXCLUSIVE)
        oflags |= SCE_O_EXCL;
    if (flags & SQLITE_OPEN_CREATE)
        oflags |= SCE_O_CREAT;
    if (flags & SQLITE_OPEN_READONLY)
        oflags |= SCE_O_RDONLY;
    if (flags & SQLITE_OPEN_READWRITE)
        oflags |= SCE_O_RDWR;
        
    // TODO(xyz): sqlite tries to open inexistant journal and then tries to read from it, wtf?
    // so force O_CREAT here
    if (flags & SQLITE_OPEN_MAIN_JOURNAL && !(flags & SQLITE_OPEN_EXCLUSIVE))
        oflags |= SCE_O_CREAT;
    
    sceClibMemset(p, 0, sizeof(PSP2File));
    p->fd = sceIoOpen(zName, oflags, 7);
    
    if (p->fd < 0) {
        sqlite3_free(aBuf);
        return SQLITE_CANTOPEN;
    }
    
    p->aBuffer = aBuf;
    
    if (pOutFlags)
        *pOutFlags = flags;
    
    p->base.pMethods = &psp2io;
    return SQLITE_OK;
}

/*
** Delete the file identified by argument zPath. If the dirSync parameter
** is non-zero, then ensure the file-system modification to delete the
** file has been synced to disk before returning.
*/
static int psp2Delete(sqlite3_vfs *pVfs, const char *zPath, int dirSync) {
    int rc = 0;                         /* Return code */
    // rc = unlink(zPath);
    
    // if (rc != 0 && errno == ENOENT)
    //     return SQLITE_OK;
        
    // if (rc == 0 && dirSync) {
    //     int dfd;                      /* File descriptor open on directory */
    //     int i;                        /* Iterator variable */
    //     char zDir[MAXPATHNAME + 1];   /* Name of directory containing file zPath */
        
    //     /* Figure out the directory name from the path of the file deleted. */
    //     sqlite3_snprintf(MAXPATHNAME, zDir, "%s", zPath);
    //     zDir[MAXPATHNAME] = '\0';
        
    //     for (i = strlen(zDir); i > 1 && zDir[i] != '/'; i++);
    //     zDir[i] = '\0';
            
    //     /* Open a file-descriptor on the directory. Sync. Close. */
    //     dfd = open(zDir, O_RDONLY, 0);
    //     if (dfd < 0) 
    //         rc = -1;
    //     else {
    //         rc = fsync(dfd);
    //         close(dfd);
    //     }
    // }
    rc = sceIoRemove(zPath);
    return (rc < 0? SQLITE_IOERR_DELETE : SQLITE_OK);
}

// #ifndef F_OK
// # define F_OK 0
// #endif
// #ifndef R_OK
// # define R_OK 4
// #endif
// #ifndef W_OK
// # define W_OK 2
// #endif

/*
** Query the file-system to see if the named file exists, is readable or
** is both readable and writable.
*/
static int psp2Access(sqlite3_vfs *pVfs, const char *zPath, int flags, int *pResOut) {
    int rc = 0;                     /* access() return code */
    // int eAccess = F_OK;             /* Second argument to access() */
    
    // assert(flags == SQLITE_ACCESS_EXISTS       /* access(zPath, F_OK) */
    //     || flags == SQLITE_ACCESS_READ         /* access(zPath, R_OK) */
    //     || flags == SQLITE_ACCESS_READWRITE    /* access(zPath, R_OK|W_OK) */
    // );
    
    // if (flags == SQLITE_ACCESS_READWRITE)
    //     eAccess = R_OK|W_OK;
    // if (flags == SQLITE_ACCESS_READ)
    //     eAccess = R_OK;
        
    // rc = access(zPath, eAccess);
    *pResOut = (rc == 0);
    return SQLITE_OK;
}

/*
** Argument zPath points to a nul-terminated string containing a file path.
** If zPath is an absolute path, then it is copied as is into the output 
** buffer. Otherwise, if it is a relative path, then the equivalent full
** path is written to the output buffer.
**
** This function assumes that paths are UNIX style. Specifically, that:
**
**   1. Path components are separated by a '/'. and 
**   2. Full paths begin with a '/' character.
*/
static int psp2FullPathname(sqlite3_vfs *pVfs, const char *zPath, int nPathOut, char *zPathOut) {
    // char zDir[MAXPATHNAME + 1];
    // if (zPath[0] == '/') {
    //     zDir[0] = '\0';
    // }
    // else {
    //     if (getcwd(zDir, sizeof(zDir)) == 0)
    //         return SQLITE_IOERR;
    // }
    
    // zDir[MAXPATHNAME] = '\0';
    // sqlite3_snprintf(nPathOut, zPathOut, "%s/%s", zDir, zPath);
    // zPathOut[nPathOut - 1] = '\0';
    
    sqlite3_snprintf(nPathOut, zPathOut, zPath);
    return SQLITE_OK;
}

/*
** The following four VFS methods:
**
**   xDlOpen
**   xDlError
**   xDlSym
**   xDlClose
**
** are supposed to implement the functionality needed by SQLite to load
** extensions compiled as shared objects. This simple VFS does not support
** this functionality, so the following functions are no-ops.
*/
static void *psp2DlOpen(sqlite3_vfs *pVfs, const char *zPath) {
    return 0;
}
static void psp2DlError(sqlite3_vfs *pVfs, int nByte, char *zErrMsg) {
    sqlite3_snprintf(nByte, zErrMsg, "Loadable extensions are not supported");
    zErrMsg[nByte-1] = '\0';
}
static void (*psp2DlSym(sqlite3_vfs *pVfs, void *pH, const char *z))(void) {
    return 0;
}
static void psp2DlClose(sqlite3_vfs *pVfs, void *pHandle) {
    return;
}

/*
** Parameter zByte points to a buffer nByte bytes in size. Populate this
** buffer with pseudo-random data.
*/
static int psp2Randomness(sqlite3_vfs *pVfs, int nByte, char *zByte) {
    return SQLITE_OK;
}

/*
** Sleep for at least nMicro microseconds. Return the (approximate) number 
** of microseconds slept for.
*/
static int psp2Sleep(sqlite3_vfs *pVfs, int nMicro) {
    sceKernelDelayThread(nMicro);
    return nMicro;
}

/*
** Set *pTime to the current UTC time expressed as a Julian day. Return
** SQLITE_OK if successful, or an error code otherwise.
**
**   http://en.wikipedia.org/wiki/Julian_day
**
** This implementation is not very good. The current time is rounded to
** an integer number of seconds. Also, assuming time_t is a signed 32-bit 
** value, it will stop working some time in the year 2038 AD (the so-called
** "year 2038" problem that afflicts systems that store time this way). 
*/
static int psp2CurrentTime(sqlite3_vfs *pVfs, double *pTime) {
    time_t t = 0;
    SceDateTime time = {0};
    sceRtcGetCurrentClock(&time, 0);
    sceRtcGetTime_t(&time, &t);
    *pTime = t/86400.0 + 2440587.5;
    return SQLITE_OK;
}

/*
** This function returns a pointer to the VFS implemented in this file.
** To make the VFS available to SQLite:
**
**   sqlite3_vfs_register(sqlite3_psp2vfs(), 0);
*/
sqlite3_vfs *sqlite3_psp2vfs(void) {
    static sqlite3_vfs psp2vfs = {
        1,                            /* iVersion */
        sizeof(PSP2File),             /* szOsFile */
        MAXPATHNAME,                  /* mxPathname */
        0,                            /* pNext */
        "psp2",                       /* zName */
        0,                            /* pAppData */
        psp2Open,                     /* xOpen */
        psp2Delete,                   /* xDelete */
        psp2Access,                   /* xAccess */
        psp2FullPathname,             /* xFullPathname */
        psp2DlOpen,                   /* xDlOpen */
        psp2DlError,                  /* xDlError */
        psp2DlSym,                    /* xDlSym */
        psp2DlClose,                  /* xDlClose */
        psp2Randomness,               /* xRandomness */
        psp2Sleep,                    /* xSleep */
        psp2CurrentTime,              /* xCurrentTime */
    };
    
    return &psp2vfs;
}

int sqlite3_os_init(void) {
    if (sqlite3_vfs_register(sqlite3_psp2vfs(), 1) != SQLITE_OK)
        return -1;
        
    return 0;
}

int sqlite3_os_end(void) {
    return sqlite3_vfs_unregister(sqlite3_psp2vfs());
}
