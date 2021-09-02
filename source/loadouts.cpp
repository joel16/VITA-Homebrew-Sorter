#include "fs.h"
#include "keyboard.h"
#include "utils.h"

namespace Loadouts {
    constexpr char db_path[] = "ur0:shell/db/app.db";
    constexpr char ini_path[] = "ux0:iconlayout.ini";
    
    static std::string StripExt(const std::string &filename) {
        size_t last_index = filename.find_last_of("."); 
        std::string raw_filename = filename.substr(0, last_index); 
        return raw_filename;
    }

    int Backup(void) {
        int ret = 0;
        std::string filename = Keyboard::GetText("Enter loadout name");
        
        if (filename.empty())
            return -1;

        // In the case user adds an extension, remove it.
        filename = Loadouts::StripExt(filename);
        const std::string loadout_path = "ux0:data/VITAHomebrewSorter/loadouts/" + filename + ".db";
        const std::string layout_path = "ux0:data/VITAHomebrewSorter/loadouts/" + filename + ".ini";

        if (R_FAILED(ret = FS::CopyFile(db_path, loadout_path)))
            return ret;

        if (R_FAILED(ret = FS::CopyFile(ini_path, layout_path)))
            return ret;
        
        return 0;
    }

    int Restore(const std::string &filename) {
        int ret = 0;

        // Remove extension and append it manually.
        const std::string raw_filename = Loadouts::StripExt(filename);
        const std::string loadout_path = "ux0:data/VITAHomebrewSorter/loadouts/" + raw_filename + ".db";
        const std::string layout_path = "ux0:data/VITAHomebrewSorter/loadouts/" + raw_filename + ".ini";

        if (R_FAILED(ret = FS::CopyFile(loadout_path, db_path)))
            return ret;

        if (R_FAILED(ret = FS::CopyFile(layout_path, ini_path)))
            return ret;
        
        return 0;
    }

    int Delete(const std::string &filename) {
        int ret = 0;

        // Remove extension and append it manually.
        const std::string raw_filename = Loadouts::StripExt(filename);
        const std::string loadout_path = "ux0:data/VITAHomebrewSorter/loadouts/" + raw_filename + ".db";
        const std::string layout_path = "ux0:data/VITAHomebrewSorter/loadouts/" + raw_filename + ".ini";

        if (R_FAILED(ret = FS::RemoveFile(loadout_path)))
            return ret;

        if (R_FAILED(ret = FS::RemoveFile(layout_path)))
            return ret;
        
        return 0;
    }
}
