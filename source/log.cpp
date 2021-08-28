#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/clib.h>
#include <cstdarg>
#include <cstdio>

#include "fs.h"
#include "utils.h"

namespace Log {
    static SceUID log_file = 0;

    void Init(void) {
        if (!FS::FileExists("ux0:data/VITAHomebrewSorter/debug.log"))
            FS::CreateFile("ux0:data/VITAHomebrewSorter/debug.log");
            
        if (R_FAILED(log_file = sceIoOpen("ux0:data/VITAHomebrewSorter/debug.log", SCE_O_WRONLY | SCE_O_APPEND, 0)))
            return;
    }

    void Exit(void) {
        if (R_FAILED(sceIoClose(log_file)))
            return;
    }
    
    void Error(const char *data, ...) {
        char buf[512];
        va_list args;
        va_start(args, data);
        sceClibVsnprintf(buf, sizeof(buf), data, args);
        va_end(args);
        
        std::string error_string = "[ERROR] ";
        error_string.append(buf);
        
        sceClibPrintf("%s", error_string.c_str());
        printf("%s", error_string.c_str());

        if (R_FAILED(sceIoWrite(log_file, error_string.data(), error_string.length())))
            return;
    }
}
