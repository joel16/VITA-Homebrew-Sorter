#include <algorithm>
#include <cstdio>
#include <string>

#include "applist.h"
#include "fs.h"
#include "log.h"
#include "sqlite3.h"
#include "utils.h"

namespace AppList {
    constexpr char path[] = "ur0:/shell/db/app.db";

    int Get(std::vector<AppInfoIcon> &entries, std::vector<AppInfoPage> &pages, std::vector<AppInfoFolder> &folders) {
        entries.clear();
        pages.clear();
        folders.clear();

        sqlite3 *db;
        int ret = sqlite3_open_v2(path, &db, SQLITE_OPEN_READWRITE, nullptr);

        if (ret)
            return -1;

        std::string query = 
            std::string("SELECT info_icon.pageId, info_page.pageNo, info_icon.pos, info_icon.title, info_icon.titleId, info_icon.icon0Type ")
            + "FROM tbl_appinfo_icon info_icon "
            + "INNER JOIN tbl_appinfo_page info_page "
            + "ON info_icon.pageId = info_page.pageId;";
        
        sqlite3_stmt *stmt;
        ret = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);

        while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
            AppInfoIcon entry;
            entry.pageId = std::stoi(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
            entry.pageNo = sqlite3_column_int(stmt, 1);
            entry.pos = sqlite3_column_int(stmt, 2);
            entry.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            std::snprintf(entry.titleId, 16, "%s", sqlite3_column_text(stmt, 4));
            entry.folder = (std::stoi(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)))) == 7? true : false;
            entries.push_back(entry);
        }
        
        if (ret != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return ret;
        }
        
        sqlite3_finalize(stmt);

        query = 
            std::string("SELECT DISTINCT info_page.pageId, info_page.pageNo ")
            + "FROM tbl_appinfo_page info_page "
            + "INNER JOIN tbl_appinfo_icon info_icon "
            + "ON info_page.pageId = info_icon.pageId "
            + "ORDER BY info_page.pageId;";

        ret = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);

        while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
            AppInfoPage entry;
            entry.pageId = sqlite3_column_int(stmt, 0);
            entry.pageNo = sqlite3_column_int(stmt, 1);
            pages.push_back(entry);

            AppInfoFolder folder;

            if (entry.pageNo < 0) {
                folder.pageId = sqlite3_column_int(stmt, 0);
                folders.push_back(folder);
            }
        }

        if (ret != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return ret;
        }
        
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 0;
    }

    int Save(std::vector<AppInfoIcon> &entries) {
        sqlite3 *db;
        int ret = sqlite3_open_v2(path, &db, SQLITE_OPEN_READWRITE, nullptr);

        if (ret)
            return ret;

        // Hacky workaround to avoid SQL's unique constraints. Please look away!
        for (int i = 0, counter = 10; i < entries.size(); i++, counter++) {
            std::string titleId = entries[i].titleId;
            std::string query = 
                std::string("UPDATE tbl_appinfo_icon ")
                + "SET pageId = " + std::to_string(entries[i].pageId) + ", pos = " + std::to_string(counter) + " "
                + "WHERE "
                + (titleId == "(null)"? "title = '" + entries[i].title + "';" : "titleId = '" + titleId + "';");
            
            char *error;
            ret = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &error);

            if (ret != SQLITE_OK) {
                Log::Error("sqlite3_exec error: %s\n", error);
                sqlite3_free(error);
                sqlite3_close(db);
                return ret;
            }

            sqlite3_free(error);
        }

        for (int i = 0; i < entries.size(); i++) {
            std::string titleId = entries[i].titleId;
            std::string query = 
                std::string("UPDATE tbl_appinfo_icon ")
                + "SET pageId = " + std::to_string(entries[i].pageId) + ", pos = " + std::to_string(entries[i].pos) + " "
                + "WHERE "
                + (titleId == "(null)"? "title = '" + entries[i].title + "';" : "titleId = '" + titleId + "';");
            
            char *error;
            ret = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &error);

            if (ret != SQLITE_OK) {
                Log::Error("sqlite3_exec error: %s\n", error);
                sqlite3_free(error);
                sqlite3_close(db);
                return ret;
            }
        }

        sqlite3_close(db);
        return 0;
    }

    bool SortAlphabeticalAsc(const AppInfoIcon &entryA, const AppInfoIcon &entryB) {
        std::string entryAname = entryA.title;
        std::string entryBname = entryB.title;

        std::transform(entryAname.begin(), entryAname.end(), entryAname.begin(), [](unsigned char c){ return std::tolower(c); });
        std::transform(entryBname.begin(), entryBname.end(), entryBname.begin(), [](unsigned char c){ return std::tolower(c); });

        if (entryAname.compare(entryBname) < 0)
            return true;

        return false;
    }

    bool SortAlphabeticalDesc(const AppInfoIcon &entryA, const AppInfoIcon &entryB) {
        std::string entryAname = entryA.title;
        std::string entryBname = entryB.title;

        std::transform(entryAname.begin(), entryAname.end(), entryAname.begin(), [](unsigned char c){ return std::tolower(c); });
        std::transform(entryBname.begin(), entryBname.end(), entryBname.begin(), [](unsigned char c){ return std::tolower(c); });

        if (entryBname.compare(entryAname) < 0)
            return true;

        return false;
    }

    void Sort(std::vector<AppInfoIcon> &entries, std::vector<AppInfoPage> &pages, std::vector<AppInfoFolder> &folders) {
        int pos = 0, pageCounter = 0;
        for (int i = 0; i < entries.size(); i ++) {
            // Reset position
            if (pos > 9) {
                pos = 0;
                pageCounter++;
            }
            
            // App/Game belongs to a folder
            if (entries[i].pageNo < 0) {
                for (int j = 0; j < folders.size(); j++) {
                    if (entries[i].pageId == folders[j].pageId) {
                        entries[i].pos = folders[j].index;
                        folders[j].index++;
                    }
                }
            }
            else {
                entries[i].pos = pos;
                entries[i].pageId = pages[pageCounter].pageId;
                pos++;
            }
        }
    }

    int Backup(void) {
        unsigned char *data = nullptr;
        SceOff size = 0;

        int ret = 0;
        if (R_FAILED(ret = FS::ReadFile(path, &data, &size)))
            return ret;

        if (R_FAILED(ret = FS::WriteFile("ux0:/data/VITAHomebrewSorter/app.db", data, size)))
            return ret; 
        
        return 0;
    }
}
