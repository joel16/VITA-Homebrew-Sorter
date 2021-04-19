#include <algorithm>
#include <filesystem>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <string>
#include <vector>

#include "log.h"
#include "utils.h"

namespace FS {
    bool FileExists(const std::string &path) {
        SceUID file = 0;
        
        if (R_SUCCEEDED(file = sceIoOpen(path.c_str(), SCE_O_RDONLY, 0777))) {
            sceIoClose(file);
            return true;
        }
        
        return false;
    }

    bool DirExists(const std::string &path) {
        SceUID dir = 0;
        
        if (R_SUCCEEDED(dir = sceIoDopen(path.c_str()))) {
            sceIoDclose(dir);
            return true;
        }
        
        return false;
    }

    int CreateFile(const std::string &path) {
        int ret = 0;
        SceUID file = 0;
        
        if (R_FAILED(ret = file = sceIoOpen(path.c_str(), SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777))) {
            Log::Error("sceIoOpen(%s) failed: 0x%lx\n", path.c_str(), ret);
            return ret;
        }
            
        if (R_FAILED(ret = sceIoClose(file))) {
            Log::Error("sceIoClose(%s) failed: 0x%lx\n", path.c_str(), ret);
            return ret;
        }
            
        return 0;
    }

    int GetFileSize(const std::string &path, SceOff *size) {
        SceIoStat stat;
        int ret = 0;
        
        if (R_FAILED(ret = sceIoGetstat(path.c_str(), &stat))) {
            Log::Error("sceIoOpen(%s) failed: 0x%lx\n", path.c_str(), ret);
            return ret;
        }

        *size = stat.st_size;
        return 0;
    }

    int ReadFile(const std::string &path, void *data, SceSize size) {
        int ret = 0, bytes_read = 0;
        SceUID file = 0;

        if (R_FAILED(ret = file = sceIoOpen(path.c_str(), SCE_O_RDONLY, 0))) {
            Log::Error("sceIoOpen(%s) failed: 0x%lx\n", path.c_str(), ret);
            return ret;
        }

        if (R_FAILED(ret = bytes_read = sceIoRead(file, data, size))) {
            Log::Error("sceIoRead(%s) failed: 0x%lx\n", path.c_str(), ret);
            return ret;
        }

        if (R_FAILED(ret = sceIoClose(file))) {
            Log::Error("sceIoClose(%s) failed: 0x%lx\n", path.c_str(), ret);
            return ret;
        }

        return bytes_read;
    }

    int WriteFile(const std::string &path, const void *data, SceSize size) {
        int ret = 0, bytes_written = 0;
        SceUID file = 0;

        if (R_FAILED(ret = file = sceIoOpen(path.c_str(), SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777))) {
            Log::Error("sceIoOpen(%s) failed: 0x%lx\n", path.c_str(), ret);
            return ret;
        }

        if (R_FAILED(ret = bytes_written = sceIoWrite(file, data, size))) {
            Log::Error("sceIoWrite(%s) failed: 0x%lx\n", path.c_str(), ret);
            sceIoClose(file);
            return ret;
        }
        
        if (R_FAILED(ret = sceIoClose(file))) {
            Log::Error("sceIoClose(%s) failed: 0x%lx\n", path.c_str(), ret);
            return ret;
        }

        return bytes_written;
    }

    int RemoveFile(const std::string &path) {
        int ret = 0;

        if (R_FAILED(ret = sceIoRemove(path.c_str()))) {
            Log::Error("sceIoRemove(%s) failed: 0x%lx\n", path.c_str(), ret);
            return ret;
        }
            
        return 0;
    }

    static std::string GetFileExt(const std::string &filename) {
        std::string ext = std::filesystem::path(filename).extension();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);
        return ext;
    }

    static bool IsDBFile(const std::string &filename) {
        std::string ext = FS::GetFileExt(filename);
        
        if (!ext.compare(".DB"))
            return true;
            
        return false;
    }

    static bool Sort(const SceIoDirent &entryA, const SceIoDirent &entryB) {
        if ((SCE_S_ISDIR(entryA.d_stat.st_mode)) && !(SCE_S_ISDIR(entryB.d_stat.st_mode)))
            return true;
        else if (!(SCE_S_ISDIR(entryA.d_stat.st_mode)) && (SCE_S_ISDIR(entryB.d_stat.st_mode)))
            return false;
        else {
            std::u16string entryA_name = reinterpret_cast<const char16_t *>(entryA.d_name);
            std::u16string entryB_name = reinterpret_cast<const char16_t *>(entryB.d_name);
            std::transform(entryA_name.begin(), entryA_name.end(), entryA_name.begin(), [](unsigned char c){ return std::tolower(c); });
            std::transform(entryB_name.begin(), entryB_name.end(), entryB_name.begin(), [](unsigned char c){ return std::tolower(c); });

            if (entryA_name.compare(entryB_name) < 0)
                return true;
        }
        
        return false;
    }

    int GetDirList(const std::string &path, std::vector<SceIoDirent> &entries) {
        int ret = 0, i = 0;
        SceUID dir = 0;
        entries.clear();

        if (R_FAILED(dir = sceIoDopen(path.c_str()))) {
            Log::Error("sceIoDopen(%s) failed: 0x%lx\n", path.c_str(), ret);
            return dir;
        }
        
        do {
            SceIoDirent dirent;
            if (R_FAILED(ret = sceIoDread(dir, &dirent)))
                Log::Error("sceIoDread(%s) failed: 0x%lx\n", path.c_str(), ret);
            
            if (!FS::IsDBFile(dirent.d_name))
                continue;

            if (SCE_S_ISDIR(dirent.d_stat.st_mode))
                continue;
            
            entries.push_back(dirent);
            i++;
        } while (ret > 0);

        std::sort(entries.begin(), entries.end(), FS::Sort);
        
        if (R_FAILED(ret = sceIoDclose(dir))) {
            Log::Error("sceIoDclose(%s) failed: 0x%lx\n", path.c_str(), ret);
            return ret;
        }

        return 0;
    }
}
