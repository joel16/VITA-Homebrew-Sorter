#ifndef _VITA_HB_SORTER_LOADOUTS_H_
#define _VITA_HB_SORTER_LOADOUTS_H_

namespace Loadouts {
    int Backup(void);
    int Restore(const std::string &filename);
    int Delete(const std::string &filename);
}

#endif
