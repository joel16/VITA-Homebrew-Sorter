#ifndef _VITA_HB_SORTER_APP_LIST
#define _VITA_HB_SORTER_APP_LIST

#include <vector>
#include <string>

typedef struct AppInfoIcon {
    int pageId = 0;
    int pageNo = 0;
    int pos = 0;
    std::string title;
    char titleId[16];
    bool folder = false;
} AppInfoIcon;

typedef struct AppInfoPage {
    int pageId = 0;
    int pageNo = 0;
} AppInfoPage;

typedef struct AppInfoFolder {
    int pageId = 0;
    int index = 0;
} AppInfoFolder;

namespace AppList {
    int Get(std::vector<AppInfoIcon> &entries, std::vector<AppInfoPage> &pages, std::vector<AppInfoFolder> &folders);
    int Save(std::vector<AppInfoIcon> &entries);
    bool SortAlphabeticalAsc(const AppInfoIcon &entryA, const AppInfoIcon &entryB);
    bool SortAlphabeticalDesc(const AppInfoIcon &entryA, const AppInfoIcon &entryB);
    void Sort(std::vector<AppInfoIcon> &entries, std::vector<AppInfoPage> &pages, std::vector<AppInfoFolder> &folders);
    int Backup(void);
    int Restore(void);
}

#endif
