#pragma once

#include <psp2/io/dirent.h>
#include <string>
#include <vector>

#include "applist.h"

enum IconType {
    App,
    DB,
    Folder,
    Page,
    Trash
};

enum SortMode {
    SortDefault,
    SortAsc,
    SortDesc
};

enum State {
    StateNone,
    StateConfirmSort,
    StateConfirmSwap,
    StateRestore,
    StateLoadoutRestore,
    StateWarning,
    StateDone,
    StateDelete,
    StateError
};

namespace Tabs {
    void Sort(AppEntries &entries, State &state, bool &backupExists);
    void Pages(AppEntries &entries, State &state, bool &backupExists);
    void Loadouts(std::vector<SceIoDirent> &loadouts, State &state, int &date_format, std::string &loadout_name);
    void Settings(void);
}
