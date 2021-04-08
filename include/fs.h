#ifndef _VITA_HB_SORTER_FS_H_
#define _VITA_HB_SORTER_FS_H_

#include <psp2/types.h>
#include <string>

namespace FS {
    bool FileExists(const std::string &path);
    bool DirExists(const std::string &path);
    int CreateFile(const std::string &path);
    int ReadFile(const std::string &path, unsigned char **buffer, SceOff *size);
    int WriteFile(const std::string &path, const void *data, SceSize size);
}

#endif
