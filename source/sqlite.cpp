#include "sqlite3.h"

#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/rtc.h>

#if VHBS_DEBUG
#define DEBUG sceClibPrintf
#else
#define DEBUG(...)
#endif

typedef struct VITAFile {
    sqlite3_file base = {0};
    SceUID file = 0;
} VITAFile;

/*  Incomplete impl of sqlite3_vfs heavily based on https://github.com/alpakeno/vita-savemgr/blob/master/src/vita_sqlite.c
    with minor changes, (C++ style casting, usage of sceClib etc)
    Based on sqlite version-3.37.1 with -DSQLITE_OS_OTHER=1 -DSQLITE_TEMP_STORE=3 -DSQLITE_THREADSAFE=0 -DDSQLITE_OMIT_WAL
    TODO: Use https://github.com/sqlite/sqlite/blob/master/src/test_vfs.c as a template
*/
namespace VITAVFS {
    static int xClose(sqlite3_file *file) {
        VITAFile *ptr = reinterpret_cast<VITAFile*>(file);
        
        sceIoClose(ptr->file);
        DEBUG("close %x\n", ptr->file);
        
        return SQLITE_OK;
    }
    
    static int xRead(sqlite3_file *file, void *zBuf, int iAmt, sqlite_int64 iOfst) {
        VITAFile *ptr = reinterpret_cast<VITAFile*>(file);
        
        sceClibMemset(zBuf, 0, iAmt);
        sceIoLseek(ptr->file, iOfst, SCE_SEEK_SET);
        int read = sceIoRead(ptr->file, zBuf, iAmt);
        DEBUG("read %x %x %x => %x\n", ptr->file, zBuf, iAmt, read);
        
        if (read == iAmt)
            return SQLITE_OK;
        else if (read >= 0)
            return SQLITE_IOERR_SHORT_READ;
            
        return SQLITE_IOERR_READ;
    }
    
    static int xWrite(sqlite3_file *file, const void *zBuf, int iAmt, sqlite_int64 iOfst) {
        VITAFile *ptr = reinterpret_cast<VITAFile*>(file);
        
        int ofst = sceIoLseek(ptr->file, iOfst, SCE_SEEK_SET);
        DEBUG("seek %x %x => %x\n", ptr->file, iOfst, ofst);
        
        if (ofst != iOfst)
            return SQLITE_IOERR_WRITE;
            
        int write = sceIoWrite(ptr->file, zBuf, iAmt);
        DEBUG("write %x %x %x => %x\n", ptr->file, zBuf, iAmt);
        
        if (write != iAmt)
            return SQLITE_IOERR_WRITE;
            
        return SQLITE_OK;
    }
    
    static int xTruncate(sqlite3_file *file, sqlite_int64 size) {
        DEBUG("truncate\n");
        return SQLITE_OK;
    }
    
    static int xSync(sqlite3_file *file, int flags) {
        return SQLITE_OK;
    }
    
    static int xFileSize(sqlite3_file *file, sqlite_int64 *pSize) {
        VITAFile *ptr = reinterpret_cast<VITAFile*>(file);
        SceIoStat stat = {0};
        sceIoGetstatByFd(ptr->file, &stat);
        DEBUG("filesize %x => %x\n", ptr->file, stat.st_size);
        *pSize = stat.st_size;
        return SQLITE_OK;
    }
    
    static int xLock(sqlite3_file *file, int eLock) {
        return SQLITE_OK;
    }
    
    static int xUnlock(sqlite3_file *file, int eLock) {
        return SQLITE_OK;
    }
    
    static int xCheckReservedLock(sqlite3_file *file, int *pResOut) {
        *pResOut = 0;
        return SQLITE_OK;
    }
    
    static int xFileControl(sqlite3_file *file, int op, void *pArg) {
        return SQLITE_OK;
    }
    
    static int xSectorSize(sqlite3_file *file) {
        return 0;
    }
    
    static int xDeviceCharacteristics(sqlite3_file *file) {
        return 0;
    }
    
    static int xOpen(sqlite3_vfs *vfs, const char *name, sqlite3_file *file, int flags, int *outFlags) {
        static const sqlite3_io_methods vitaio = {
            1,
            VITAVFS::xClose,
            VITAVFS::xRead,
            VITAVFS::xWrite,
            VITAVFS::xTruncate,
            VITAVFS::xSync,
            VITAVFS::xFileSize,
            VITAVFS::xLock,
            VITAVFS::xUnlock,
            VITAVFS::xCheckReservedLock,
            VITAVFS::xFileControl,
            VITAVFS::xSectorSize,
            VITAVFS::xDeviceCharacteristics,
        };
        
        VITAFile *ptr = reinterpret_cast<VITAFile*>(file);
        unsigned int oflags = 0;
        
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
            
        sceClibMemset(ptr, 0, sizeof(*ptr));
        ptr->file = sceIoOpen(name, oflags, 7);
        DEBUG("open %s %x orig flags %x => %x\n", name, oflags, flags, ptr->file);
        
        if (ptr->file < 0)
            return SQLITE_CANTOPEN;
        if (outFlags)
            *outFlags = flags;
            
        ptr->base.pMethods = &vitaio;
        return SQLITE_OK;
    }
    
    int xDelete(sqlite3_vfs *vfs, const char *name, int syncDir) {
        int ret = sceIoRemove(name);
        if (ret < 0)
            return SQLITE_IOERR_DELETE;
        return SQLITE_OK;
    }
    
    int xAccess(sqlite3_vfs *vfs, const char *name, int flags, int *pResOut) {
        *pResOut = 1;
        return SQLITE_OK;
    }
    
    int xFullPathname(sqlite3_vfs *vfs, const char *zName, int nOut, char *zOut) {
        sceClibSnprintf(zOut, nOut, "%s", zName);
        return 0;
    }
    
    void* xDlOpen(sqlite3_vfs *vfs, const char *zFilename) {
        return nullptr;
    }
    
    void xDlError(sqlite3_vfs *vfs, int nByte, char *zErrMsg) {
        
    }
    
    void (*xDlSym(sqlite3_vfs *vfs,void*p, const char *zSymbol))(void) {
        return nullptr;
    }
    
    void xDlClose(sqlite3_vfs *vfs, void*p) {

    }
    
    int xRandomness(sqlite3_vfs *vfs, int nByte, char *zOut) {
        return SQLITE_OK;
    }
    
    int xSleep(sqlite3_vfs *vfs, int microseconds) {
        sceKernelDelayThread(microseconds);
        return SQLITE_OK;
    }
    
    int xCurrentTime(sqlite3_vfs *vfs, double *pTime) {
        time_t t = 0;
        SceDateTime time = {0};
        sceRtcGetCurrentClock(&time, 0);
        sceRtcGetTime_t(&time, &t);
        *pTime = t/86400.0 + 2440587.5;
        return SQLITE_OK;
    }
    
    int xGetLastError(sqlite3_vfs *vfs, int e, char *err) {
        return 0;
    }
}

sqlite3_vfs vita_vfs = {
    .iVersion = 1,
    .szOsFile = sizeof(VITAFile),
    .mxPathname = 0x100,
    .pNext = nullptr,
    .zName = "psp2",
    .pAppData = nullptr,
    .xOpen = VITAVFS::xOpen,
    .xDelete = VITAVFS::xDelete,
    .xAccess = VITAVFS::xAccess,
    .xFullPathname = VITAVFS::xFullPathname,
    .xDlOpen = VITAVFS::xDlOpen,
    .xDlError = VITAVFS::xDlError,
    .xDlSym = VITAVFS::xDlSym,
    .xDlClose = VITAVFS::xDlClose,
    .xRandomness = VITAVFS::xRandomness,
    .xSleep = VITAVFS::xSleep,
    .xCurrentTime = VITAVFS::xCurrentTime,
    .xGetLastError = VITAVFS::xGetLastError,
};

int sqlite3_os_init(void) {
    sqlite3_vfs_register(&vita_vfs, 1);
    return 0;
}

int sqlite3_os_end(void) {
    return 0;
}
