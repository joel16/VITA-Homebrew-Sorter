#include "fs.h"
#include "keyboard.h"
#include "utils.h"

namespace Loadouts {
    constexpr char path[] = "ur0:shell/db/app.db";

    int Backup(void) {
        int ret = 0;
        std::string filename = Keyboard::GetText("Enter loadout name");
        
        if (filename.empty())
            return -1;
        
        std::string loadout_path = "ux0:data/VITAHomebrewSorter/loadouts/" + filename + ".db";

        if (R_FAILED(ret = FS::CopyFile(path, loadout_path)))
            return ret;
        
        return 0;
    }

    int Restore(const char *name) {
        int ret = 0;
        std::string loadout_path = "ux0:data/VITAHomebrewSorter/loadouts/" + std::string(name);

        if (R_FAILED(ret = FS::CopyFile(loadout_path, path)))
            return ret;
        
        return 0;
    }
}
