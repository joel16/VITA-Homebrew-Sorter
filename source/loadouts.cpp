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
        unsigned char *data = nullptr;
        SceOff size = 0;

        if (R_FAILED(ret = FS::GetFileSize(path, &size)))
            return ret;

        data = new unsigned char[size];
        if (!data)
            return -1;
        
        if (R_FAILED(ret = FS::ReadFile(path, data, size))) {
            delete[] data;
            return ret;
        }

        if (FS::FileExists(loadout_path)) {
            if (R_FAILED(ret = FS::RemoveFile(loadout_path))) {
                delete[] data;
                return ret;
            }
        }

        if (R_FAILED(ret = FS::WriteFile(loadout_path, data, size))) {
            delete[] data;
            return ret;
        }

        delete[] data;
        return 0;
    }

    int Restore(const char *name) {
        int ret = 0;
        std::string loadout_path = "ux0:data/VITAHomebrewSorter/loadouts/" + std::string(name);
        unsigned char *data = nullptr;
        SceOff size = 0;

        if (R_FAILED(ret = FS::GetFileSize(loadout_path, &size)))
            return ret;

        data = new unsigned char[size];
        if (!data)
            return -1;

        if (R_FAILED(ret = FS::ReadFile(loadout_path, data, size))) {
            delete[] data;
            return ret;
        }

        if (FS::FileExists(path)) {
            if (R_FAILED(ret = FS::RemoveFile(path))) {
                delete[] data;
                return ret;
            }
        }

        if (R_FAILED(ret = FS::WriteFile(path, data, size))) {
            delete[] data;
            return ret;
        }

        delete[] data;
        return 0;
    }
}
