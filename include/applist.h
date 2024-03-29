#pragma once

#include <vector>
#include <string>

struct AppInfoIcon {
    int pageId = 0;
    int pageNo = 0;
    int pos = 0;
    char title[128] = {0};
    char titleId[16] = {0};
    char reserved01[16] = {0};
    int icon0Type = 0; 
};

struct AppInfoPage {
    int pageId = 0;
    int pageNo = 0;
};

struct AppInfoFolder {
    int pageId = 0;
    int index = 0;
};

// Info for an app that belongs to a parent folder.
struct AppInfoChild {
    int pageId = 0;
    int pageNo = 0;
    int pos = 0;
    char title[128] = {0};
    char titleId[16] = {0};
};

struct AppEntries {
    std::vector<AppInfoIcon> icons;
    std::vector<AppInfoPage> pages;
    std::vector<AppInfoFolder> folders;
    std::vector<AppInfoChild> child_apps;
};

namespace AppList {
    int Get(AppEntries &entries);
    int Save(std::vector<AppInfoIcon> &entries);
    int SavePages(std::vector<AppInfoPage> &entries);
    bool SortAppAsc(const AppInfoIcon &entryA, const AppInfoIcon &entryB);
    bool SortAppDesc(const AppInfoIcon &entryA, const AppInfoIcon &entryB);
    bool SortChildAppAsc(const AppInfoChild &entryA, const AppInfoChild &entryB);
    bool SortChildAppDesc(const AppInfoChild &entryA, const AppInfoChild &entryB);
    void Sort(AppEntries &entries);
    int Backup(void);
    int Restore(void);
    bool Compare(const std::string &db_name);
}
