#ifndef _VITA_HB_SORTER_APP_LIST_H_
#define _VITA_HB_SORTER_APP_LIST_H_

#include <vector>
#include <string>

struct AppInfoIcon {
    int pageId = 0;
    int pageNo = 0;
    int pos = 0;
    char title[128];
    char titleId[16];
    int reserved01 = 0;
    bool folder = false;
};

struct AppInfoPage {
    int pageId = 0;
    int pageNo = 0;
};

struct AppInfoFolder {
    int pageId = 0;
    int index = 0;
};

struct AppEntries {
    std::vector<AppInfoIcon> icons;
    std::vector<AppInfoPage> pages;
    std::vector<AppInfoFolder> folders;
};

extern int sort_mode;

namespace AppList {
    int Get(AppEntries *entries);
    int Save(std::vector<AppInfoIcon> &entries);
    bool SortAlphabeticalAsc(const AppInfoIcon &entryA, const AppInfoIcon &entryB);
    bool SortAlphabeticalDesc(const AppInfoIcon &entryA, const AppInfoIcon &entryB);
    void Sort(AppEntries *entries);
    int Backup(void);
    int Restore(void);
    bool Compare(const char *db_name);
}

#endif
