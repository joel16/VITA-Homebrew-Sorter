#pragma once

typedef struct {
    bool beta_features = false;
    int sort_by = 0;
    int sort_folders = 0;
    int sort_mode = 0;
} config_t;

extern config_t cfg;

enum SortBy {
    SortTitle,
    SortTitleID
};

enum SortFolders {
    SortBoth,
    SortAppsOnly,
    SortFoldersOnly
};

namespace Config {
    int Save(config_t &config);
    int Load(void);
}
