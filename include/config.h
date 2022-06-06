#ifndef _VITA_HB_SORTER_CONFIG_H_
#define _VITA_HB_SORTER_CONFIG_H_

typedef struct {
    bool beta_features = false;
    int sort_by = 0;
    int sort_folders = 0;
    int sort_mode = 0;
} config_t;

extern config_t cfg;

namespace Config {
    int Save(config_t &config);
    int Load(void);
}

#endif
