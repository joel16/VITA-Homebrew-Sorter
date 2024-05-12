#include <algorithm>
#include <cstdio>
#include <psp2/kernel/clib.h>
#include <string>

#include "applist.h"
#include "config.h"
#include "fs.h"
#include "log.h"
#include "sqlite3.h"
#include "power.h"
#include "utils.h"

namespace AppList {
    int Get(AppEntries &entries) {
        entries.icons.clear();
        entries.pages.clear();
        entries.folders.clear();
        entries.child_apps.clear();

        sqlite3 *db = nullptr;
        int ret = sqlite3_open_v2(db_path, &db, SQLITE_OPEN_READWRITE, nullptr);
        if (ret != SQLITE_OK) {
            Log::Error("sqlite3_open_v2 failed to open %s\n", db_path);
            return ret;
        }

        std::string query = std::string("SELECT info_icon.pageId, info_page.pageNo, info_icon.pos, info_icon.title, info_icon.titleId, info_icon.reserved01, ")
            + "info_icon.icon0Type "
            + "FROM tbl_appinfo_icon info_icon "
            + "INNER JOIN tbl_appinfo_page info_page "
            + "ON info_icon.pageId = info_page.pageId;";
        
        sqlite3_stmt *stmt = nullptr;
        ret = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);

        while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
            AppInfoIcon icon;
            AppInfoChild child;

            icon.pageId = std::stoi(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
            icon.pageNo = sqlite3_column_int(stmt, 1);
            icon.pos = sqlite3_column_int(stmt, 2);
            sceClibSnprintf(icon.title, 128, "%s", sqlite3_column_text(stmt, 3));
            sceClibSnprintf(icon.titleId, 16, "%s", sqlite3_column_text(stmt, 4));
            sceClibSnprintf(icon.reserved01, 16, "%s", sqlite3_column_text(stmt, 5));
            icon.icon0Type = std::stoi(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6))); // 7 = folder
            
            if (icon.pageNo < 0) {
                child.pageId = icon.pageId;
                child.pageNo = icon.pageNo;
                child.pos = icon.pos;
                sceClibSnprintf(child.title, 128, icon.title);
                sceClibSnprintf(child.titleId, 16, icon.titleId);
            }

            entries.icons.push_back(icon);
            entries.child_apps.push_back(child);
        }
        
        if (ret != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return ret;
        }
        
        sqlite3_finalize(stmt);

        query = std::string("SELECT DISTINCT info_page.pageId, info_page.pageNo ")
            + "FROM tbl_appinfo_page info_page "
            + "INNER JOIN tbl_appinfo_icon info_icon "
            + "ON info_page.pageId = info_icon.pageId "
            + "ORDER BY info_page.pageId;";

        ret = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);

        while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
            AppInfoPage page;
            AppInfoFolder folder;
            
            int pageNo = sqlite3_column_int(stmt, 1);

            if (pageNo >= 0) {
                page.pageId = sqlite3_column_int(stmt, 0);
                page.pageNo = pageNo;
                entries.pages.push_back(page);
            }
            else if (pageNo < 0) {
                folder.pageId = sqlite3_column_int(stmt, 0);
                entries.folders.push_back(folder);
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

    static void Error(const std::string &query, char *error, sqlite3 *db, const std::string &path) {
        Log::Error("%s error %s\n", query.c_str(), error);
        sqlite3_free(error);
        sqlite3_close(db);
        FS::RemoveFile(path);
        Power::Unlock();
    }

    int Save(std::vector<AppInfoIcon> &entries) {
        int ret = 0;
        sqlite3 *db = nullptr;
        char *error = nullptr;
        char db_path_backup[] = "ur0:shell/db/app.db.sort.bkp";
        
        if (R_FAILED(ret = FS::CopyFile(db_path, db_path_backup))) {
            return ret;
        }

        ret = sqlite3_open_v2(db_path, &db, SQLITE_OPEN_READWRITE, nullptr);
        if (ret != SQLITE_OK) {
            Log::Error("sqlite3_open_v2 failed to open %s\n", db_path);
            FS::RemoveFile(db_path);
            return ret;
        }

        // Lock power and prevent auto suspend.
        Power::Lock();

        const char *prepare_query[] = {
            "BEGIN TRANSACTION",
            "PRAGMA foreign_keys = off",
            "DROP TABLE IF EXISTS tbl_appinfo_icon_sort",
            "CREATE TABLE tbl_appinfo_icon_sort AS SELECT * FROM tbl_appinfo_icon"
        };
        
        for (int i = 0; i < 4; ++i) {
            ret = sqlite3_exec(db, prepare_query[i], nullptr, nullptr, &error);
            if (ret != SQLITE_OK) {
                AppList::Error(prepare_query[i], error, db, db_path);
                return ret;
            }
        }

        // Update tbl_appinfo_icon_sort with sorted icons
        for (unsigned int i = 0; i < entries.size(); i++) {
            std::string title = entries[i].title;
            std::string titleId = entries[i].titleId;
            std::string reserved01 = entries[i].reserved01;
            std::string query = std::string("UPDATE tbl_appinfo_icon_sort ")
                + "SET pageId = " + std::to_string(entries[i].pageId) + ", pos = " + std::to_string(entries[i].pos) + " "
                + "WHERE ";

            if ((title == "(null)") && (titleId == "(null)")) {
                // Check if power icon on PSTV, otherwise use reserved01.
                if (entries[i].icon0Type == 8) {
                    query.append("icon0Type = " + std::to_string(entries[i].icon0Type) + ";");
                }
                else if (reserved01 != "(null)") {
                    query.append("reserved01 = " + reserved01 + ";");
                }
            }
            else {
                query.append((titleId == "(null)"? "title = \"" + title + "\"" : "titleId = \"" + titleId + "\"")
                    + (entries[i].icon0Type == 7? " AND reserved01 = " + reserved01 + ";" : ";"));
            }

            ret = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &error);
            if (ret != SQLITE_OK) {
                AppList::Error(query, error, db, db_path);

                // If sorting fails, drop tbl_appinfo_icon_sort.
                query = std::string("DROP TABLE IF EXISTS tbl_appinfo_icon_sort");
                int ret_next = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &error);
                if (ret_next != SQLITE_OK) {
                    AppList::Error(query, error, db, db_path);
                    return ret_next;
                }

                return ret;
            }
        }

        const char *finish_query[] = {
            "DROP TABLE tbl_appinfo_icon",
            "CREATE TABLE tbl_appinfo_icon(pageId REFERENCES tbl_appinfo_page(pageId) ON DELETE RESTRICT NOT NULL, pos INT NOT NULL, iconPath TEXT, title TEXT COLLATE NOCASE, type NOT NULL, command TEXT, titleId TEXT, icon0Type NOT NULL, parentalLockLv INT, status INT, reserved01, reserved02, reserved03, reserved04, reserved05, PRIMARY KEY(pageId, pos))",
            "CREATE INDEX idx_icon_pos ON tbl_appinfo_icon ( pos, pageId )",
            "CREATE INDEX idx_icon_title ON tbl_appinfo_icon (title, titleId, type)",
            "INSERT INTO tbl_appinfo_icon SELECT * FROM tbl_appinfo_icon_sort",
            "DROP TABLE tbl_appinfo_icon_sort",
            "PRAGMA foreign_keys = on",
            "COMMIT"
        };

        for (int i = 0; i < 8; ++i) {
            ret = sqlite3_exec(db, finish_query[i], nullptr, nullptr, &error);
            if (ret != SQLITE_OK) {
                AppList::Error(finish_query[i], error, db, db_path);
                return ret;
            }
        }

        Power::Unlock();
        sqlite3_close(db);
        return 0;
    }

    int SavePages(std::vector<AppInfoPage> &entries) {
        int ret = 0;
        sqlite3 *db = nullptr;
        char *error = nullptr;
        char db_path_backup[] = "ur0:shell/db/app.db.sort.bkp";
        
        if (R_FAILED(ret = FS::CopyFile(db_path, db_path_backup))) {
            return ret;
        }

        ret = sqlite3_open_v2(db_path, &db, SQLITE_OPEN_READWRITE, nullptr);
        if (ret != SQLITE_OK) {
            Log::Error("sqlite3_open_v2 failed to open %s\n", db_path);
            FS::RemoveFile(db_path);
            return ret;
        }

        // Lock power and prevent auto suspend.
        Power::Lock();

        const char *prepare_query[] = {
            "BEGIN TRANSACTION",
            "PRAGMA foreign_keys = off",
            "DROP TABLE IF EXISTS tbl_appinfo_page_sort",
            "CREATE TABLE tbl_appinfo_page_sort AS SELECT * FROM tbl_appinfo_page"
        };
        
        for (int i = 0; i < 4; ++i) {
            ret = sqlite3_exec(db, prepare_query[i], nullptr, nullptr, &error);
            if (ret != SQLITE_OK) {
                AppList::Error(prepare_query[i], error, db, db_path);
                return ret;
            }
        }

        // Update tbl_appinfo_page_sort with swapped pages
        for (unsigned int i = 0; i < entries.size(); i++) {
            std::string query = std::string("UPDATE tbl_appinfo_page_sort ")
                + "SET pageNo = " + std::to_string(entries[i].pageNo) + " WHERE pageId = "
                + std::to_string(entries[i].pageId) + ";";

            ret = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &error);
            if (ret != SQLITE_OK) {
                AppList::Error(query, error, db, db_path);

                // If sorting fails, drop tbl_appinfo_page_sort.
                query = std::string("DROP TABLE IF EXISTS tbl_appinfo_page_sort");
                int ret_next = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &error);
                if (ret_next != SQLITE_OK) {
                    AppList::Error(query, error, db, db_path);
                    return ret_next;
                }

                return ret;
            }
        }

        const char *finish_query[] = {
            "DROP TABLE tbl_appinfo_page",
            "CREATE TABLE tbl_appinfo_page(pageId INTEGER PRIMARY KEY NOT NULL, pageNo INT NOT NULL, themeFile TEXT, bgColor INT, texWidth INT, texHeight INT, imageWidth INT, imageHeight INT, reserved01, reserved02, reserved03, reserved04, reserved05)",
            "CREATE INDEX idx_page_no ON tbl_appinfo_page ( pageNo )",
            "INSERT INTO tbl_appinfo_page SELECT * FROM tbl_appinfo_page_sort",
            "DROP TABLE tbl_appinfo_page_sort",
            "CREATE TRIGGER tgr_deletePage2 AFTER DELETE ON tbl_appinfo_page WHEN OLD.pageNo >= 0 BEGIN UPDATE tbl_appinfo_page SET pageNo = pageNo - 1 WHERE tbl_appinfo_page.pageNo > OLD.pageNo; END",
            "CREATE TRIGGER tgr_insertPage2 BEFORE INSERT ON tbl_appinfo_page WHEN NEW.pageNo >= 0 BEGIN UPDATE tbl_appinfo_page SET pageNo = pageNo + 1 WHERE tbl_appinfo_page.pageNo >= NEW.pageNo; END",
            "PRAGMA foreign_keys = on",
            "COMMIT"
        };

        for (int i = 0; i < 9; ++i) {
            ret = sqlite3_exec(db, finish_query[i], nullptr, nullptr, &error);
            if (ret != SQLITE_OK) {
                AppList::Error(finish_query[i], error, db, db_path);
                return ret;
            }
        }

        Power::Unlock();
        sqlite3_close(db);
        return 0;
    }

    static bool Compare(const std::string &nameA, const std::string &nameB, SortOrder sortOrder) {
        if (sortOrder == SortTitle) {
            return std::lexicographical_compare(nameA.begin(), nameA.end(), nameB.begin(), nameB.end(), [](char a, char b) {
                return std::tolower(a) < std::tolower(b);
            });
        }
        else {
            return nameA < nameB;
        }
    }

    bool SortAppAsc(const AppInfoIcon &entryA, const AppInfoIcon &entryB) {
        const std::string &entryAName = (cfg.sort_by == SortTitle) ? entryA.title : entryA.titleId;
        const std::string &entryBName = (cfg.sort_by == SortTitle) ? entryB.title : entryB.titleId;
        return AppList::Compare(entryAName, entryBName, cfg.sort_by);
    }

    bool SortAppDesc(const AppInfoIcon &entryA, const AppInfoIcon &entryB) {
        const std::string &entryAName = (cfg.sort_by == SortTitle) ? entryA.title : entryA.titleId;
        const std::string &entryBName = (cfg.sort_by == SortTitle) ? entryB.title : entryB.titleId;
        return AppList::Compare(entryBName, entryAName, cfg.sort_by);
    }

    bool SortChildAppAsc(const AppInfoChild &entryA, const AppInfoChild &entryB) {
        const std::string &entryAName = (cfg.sort_by == SortTitle) ? entryA.title : entryA.titleId;
        const std::string &entryBName = (cfg.sort_by == SortTitle) ? entryB.title : entryB.titleId;
        return AppList::Compare(entryAName, entryBName, cfg.sort_by);
    }

    bool SortChildAppDesc(const AppInfoChild &entryA, const AppInfoChild &entryB) {
        const std::string &entryAName = (cfg.sort_by == SortTitle) ? entryA.title : entryA.titleId;
        const std::string &entryBName = (cfg.sort_by == SortTitle) ? entryB.title : entryB.titleId;
        return AppList::Compare(entryBName, entryAName, cfg.sort_by);
    }
    
    void Sort(AppEntries &entries) {
        const int MAX_POS = 9;
        int pos = 0, pageCounter = 0;
        
        std::unordered_map<int, int> folderIndexMap;
        for (AppInfoFolder folder : entries.folders) {
            folderIndexMap[folder.pageId] = folder.index;
        }
        
        for (unsigned int i = 0; i < entries.icons.size(); i++) {
            // Reset position
            if (pos > MAX_POS) {
                pos = 0;
                pageCounter++;
            }
            
            // App/Game belongs to a folder
            if ((entries.icons[i].pageNo < 0) && (cfg.sort_folders != SortAppsOnly)) {
                std::unordered_map<int, int>::const_iterator it = folderIndexMap.find(entries.icons[i].pageId);

                if (it != folderIndexMap.end()) {
                    entries.icons[i].pos = it->second;
                    folderIndexMap[entries.icons[i].pageId]++;
                }
            }
            else if ((entries.icons[i].pageNo >= 0) && (cfg.sort_folders != SortFoldersOnly)) {
                entries.icons[i].pos = pos;
                entries.icons[i].pageId = entries.pages[pageCounter].pageId;
                pos++;
            }
        }
    }

    int Backup(void) {
        int ret = 0;
        std::string backup_path;
        
        if (!FS::FileExists("ux0:data/VITAHomebrewSorter/backup/app.db.bkp")) {
            backup_path = "ux0:data/VITAHomebrewSorter/backup/app.db.bkp";
        }
        else {
            backup_path = "ux0:data/VITAHomebrewSorter/backup/app.db";
        }
            
        if (R_FAILED(ret = FS::CopyFile(db_path, backup_path))) {
            return ret;
        }
            
        return 0;
    }

    int Restore(void) {
        int ret = 0;
        std::string restore_path;

        if (!FS::FileExists("ux0:data/VITAHomebrewSorter/backup/app.db")) {
            restore_path = "ux0:data/VITAHomebrewSorter/backup/app.db.bkp";
        }
        else {
            restore_path = "ux0:data/VITAHomebrewSorter/backup/app.db";
        }

        if (R_FAILED(ret = FS::CopyFile(restore_path, db_path))) {
            return ret;
        }
        
        return 0;
    }

    bool Compare(const std::string &db_name) {
        std::vector<std::string> app_entries;
        std::vector<std::string> loadout_entries;
        std::vector<std::string> diff_entries;

        sqlite3 *db = nullptr;
        int ret = sqlite3_open_v2(db_path, &db, SQLITE_OPEN_READWRITE, nullptr);
        if (ret != SQLITE_OK) {
            Log::Error("sqlite3_open_v2 failed to open %s\n", db_path);
            return false;
        }

        const std::string query = "SELECT title FROM tbl_appinfo_icon;";
        
        sqlite3_stmt *stmt = nullptr;
        ret = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);

        while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
            std::string entry = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            app_entries.push_back(entry);
        }
        
        if (ret != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return false;
        }

        sqlite3_finalize(stmt);
        sqlite3_close(db);

        const std::string loadout_path = "ux0:data/VITAHomebrewSorter/loadouts/" + db_name;
        
        ret = sqlite3_open_v2(loadout_path.c_str(), &db, SQLITE_OPEN_READWRITE, nullptr);
        if (ret != SQLITE_OK) {
            Log::Error("sqlite3_open_v2 failed to open %s\n", loadout_path.c_str());
            return false;
        }
        
        ret = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);

        while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
            std::string entry = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            loadout_entries.push_back(entry);
        }
        
        if (ret != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return false;
        }
        
        sqlite3_finalize(stmt);
        sqlite3_close(db);

        if (app_entries.empty() || loadout_entries.empty()) {
            return false;
        }
        
        std::set_difference(app_entries.begin(), app_entries.end(), loadout_entries.begin(), loadout_entries.end(),
        std::inserter(diff_entries, diff_entries.begin()));
        return (diff_entries.size() > 0);
    }
}
