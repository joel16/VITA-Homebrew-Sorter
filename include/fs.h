#ifndef _VITA_HB_SORTER_FS_H_
#define _VITA_HB_SORTER_FS_H_

#include <psp2/io/dirent.h>
#include <psp2/types.h>
#include <string>
#include <vector>

namespace FS {
    bool FileExists(const std::string &path);
    bool DirExists(const std::string &path);
    int CreateFile(const std::string &path);
    int GetFileSize(const std::string &path, SceOff *size);
    int ReadFile(const std::string &path, void *data, SceSize size);
    int WriteFile(const std::string &path, const void *data, SceSize size);
    int RemoveFile(const std::string &path);
    int GetDirList(const std::string &path, std::vector<SceIoDirent> &entries);
}

#endif
