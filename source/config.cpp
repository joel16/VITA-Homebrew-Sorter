#include <memory>
#include <psp2/json.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/clib.h>

#include "config.h"
#include "fs.h"
#include "log.h"
#include "utils.h"

#define CONFIG_VERSION 1

config_t cfg;

namespace Config {
    static constexpr char config_path[] = "ux0:data/VITAHomebrewSorter/config.json";
    static const char *config_file = "{\n\t\"beta_features\": %s,\n\t\"sort_by\": %d,\n\t\"sort_folders\": %d,\n\t\"sort_mode\": %d,\n\t\"version\": %d\n}";
    static int config_version_holder = 0;
    
    class Allocator : public sce::Json::MemAllocator {
        public:
            Allocator() {
                
            }
            
            virtual void *allocateMemory(size_t size, void *unk) override {
                return std::malloc(size);
            }
            
            virtual void freeMemory(void *ptr, void *unk) override {
                std::free(ptr);
            }
    };
    
    int Save(config_t &config) {
        int ret = 0;
        std::unique_ptr<char[]> buffer(new char[128]);
        SceSize len = sceClibSnprintf(buffer.get(), 128, config_file, config.beta_features? "true" : "false",
            config.sort_by, config.sort_folders, config.sort_mode, CONFIG_VERSION);
        
        if (R_FAILED(ret = FS::WriteFile(config_path, buffer.get(), len)))
            return ret;
        
        return 0;
    }

    int Load(void) {
        int ret = 0;
            
        if (!FS::FileExists(config_path)) {
            cfg = {0};
            return Config::Save(cfg);
        }
            
        SceUID file = 0;
        if (R_FAILED(ret = file = sceIoOpen(config_path, SCE_O_RDONLY, 0))) {
            Log::Error("sceIoOpen(%s) failed: 0x%lx\n", config_path, ret);
            return ret;
        }
            
        SceSize size = sceIoLseek(file, 0, SCE_SEEK_END);
        std::unique_ptr<char[]> buffer(new char[size]);
        
        if (R_FAILED(ret = sceIoPread(file, buffer.get(), size, SCE_SEEK_SET))) {
            Log::Error("sceIoRead(%s) failed: 0x%lx\n", config_path, ret);
            sceIoClose(file);
            return ret;
        }
        
        if (R_FAILED(ret = sceIoClose(file))) {
            Log::Error("sceIoClose(%s) failed: 0x%lx\n", config_path, ret);
            return ret;
        }

        Allocator *alloc = new Allocator();

        sce::Json::InitParameter params;
        params.allocator = alloc;
        params.bufSize = size;

        sce::Json::Initializer init = sce::Json::Initializer();
        if (R_FAILED(ret = init.initialize(&params))) {
            Log::Error("sce::Json::Initializer::initialize failed  0x%lx\n", ret);
            init.terminate();
            delete alloc;
            return ret;
        }

        sce::Json::Value value = sce::Json::Value();
        if (R_FAILED(ret = sce::Json::Parser::parse(value, buffer.get(), params.bufSize))) {
            Log::Error("sce::Json::Parser::parse failed  0x%lx\n", ret);
            init.terminate();
            delete alloc;
            return ret;
        }

        // We know sceJson API loops through the child values in root alphabetically.
        cfg.beta_features = value.getValue(0).getBoolean();
        cfg.sort_by = value.getValue(1).getInteger();
        cfg.sort_folders = value.getValue(2).getInteger();
        cfg.sort_mode = value.getValue(3).getInteger();
        config_version_holder = value.getValue(4).getInteger();

        init.terminate();
        delete alloc;
            
        // Delete config file if config file is updated. This will rarely happen.
        if (config_version_holder < CONFIG_VERSION) {
            sceIoRemove(config_path);
            cfg = {0};
            return Config::Save(cfg);
        }
        
        return 0;
    }
}
