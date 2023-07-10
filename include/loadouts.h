#pragma once

namespace Loadouts {
    int Backup(void);
    int Restore(const std::string &filename);
    int Delete(const std::string &filename);
}
