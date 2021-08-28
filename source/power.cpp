#include <psp2/kernel/processmgr.h>

#include "log.h"
#include "utils.h"

namespace Power {
    static bool lock_power = false;
    
    static int Thread(SceSize args, void *argp) {
        while (true) {
            if (lock_power)
                sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_AUTO_SUSPEND);
                
            sceKernelDelayThread(10 * 1000 * 1000);
        }
        
        return 0;
    }
    
    void InitThread(void) {
        SceUID thread = 0;

        if (R_SUCCEEDED(thread = sceKernelCreateThread("Power::Thread", Power::Thread, 0x10000100, 0x40000, 0, 0, nullptr)))
            sceKernelStartThread(thread, 0, nullptr);
    }
    
    void Lock(void) {
        lock_power = true;
    }
    
    void Unlock(void) {
        lock_power = false;
    }
}
