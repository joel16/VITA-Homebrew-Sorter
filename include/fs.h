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
    int MakeDir(const std::string &path);
    int WriteFile(const std::string &path, const void *data, SceSize size);
    int RemoveFile(const std::string &path);
    int CopyFile(const std::string &src_path, const std::string &dest_path);
    std::string GetFileExt(const std::string &filename);
    int GetDirList(const std::string &path, std::vector<SceIoDirent> &entries);
}

#endif
