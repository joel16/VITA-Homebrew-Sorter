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
            std::string("SELECT info_icon.pageId, info_page.pageNo, info_icon.pos, info_icon.title, info_icon.titleId, info_icon.reserved01, info_icon.icon0Type ")
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
            std::snprintf(entry.title, 128, "%s", reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
            std::snprintf(entry.titleId, 16, "%s", sqlite3_column_text(stmt, 4));
            if (std::string(entry.titleId) == "(null)")
                entry.reserved01 = (std::stoi(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5))));
            entry.folder = (std::stoi(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)))) == 7? true : false;
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
            AppInfoPage page;
            AppInfoFolder folder;
            int pageNo = sqlite3_column_int(stmt, 1);

            if (pageNo >= 0) {
                page.pageId = sqlite3_column_int(stmt, 0);
                page.pageNo = pageNo;
                pages.push_back(page);
            }
            else if (pageNo < 0) {
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
            std::string title = entries[i].title;
            std::string titleId = entries[i].titleId;
            std::string query = 
                std::string("UPDATE tbl_appinfo_icon ")
                + "SET pageId = " + std::to_string(entries[i].pageId) + ", pos = " + std::to_string(counter) + " "
                + "WHERE ";

            if ((title == "(null)") && (titleId == "(null)"))
                query.append("reserved01 = " + std::to_string(entries[i].reserved01) + ";");
            else {
                query.append((titleId == "(null)"? "title = '" + title + "'" : "titleId = '" + titleId + "'")
                + (entries[i].folder == true? " AND reserved01 = " + std::to_string(entries[i].reserved01) + ";" : ";"));
            }
            
            ret = sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr);
            if (ret != SQLITE_OK) {
                Log::Error("sqlite3_exec error %s\n", query.c_str());
                sqlite3_close(db);
                AppList::Restore(); // Restore from backup incase sort fails
                return ret;
            }
        }

        for (int i = 0; i < entries.size(); i++) {
            std::string title = entries[i].title;
            std::string titleId = entries[i].titleId;
            std::string query = 
                std::string("UPDATE tbl_appinfo_icon ")
                + "SET pageId = " + std::to_string(entries[i].pageId) + ", pos = " + std::to_string(entries[i].pos) + " "
                + "WHERE ";

            if ((title == "(null)") && (titleId == "(null)"))
                query.append("reserved01 = " + std::to_string(entries[i].reserved01) + ";");
            else {
                query.append((titleId == "(null)"? "title = '" + title + "'" : "titleId = '" + titleId + "'")
                + (entries[i].folder == true? " AND reserved01 = " + std::to_string(entries[i].reserved01) + ";" : ";"));
            }

            ret = sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr);
            if (ret != SQLITE_OK) {
                Log::Error("sqlite3_exec error %s\n", query.c_str());
                sqlite3_close(db);
                AppList::Restore(); // Restore from backup incase sort fails
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
        // Create an original backup for first time use.
        std::string backup_path;

        if (!FS::FileExists("ux0:/data/VITAHomebrewSorter/backup/app.db.bkp"))
            backup_path = "ux0:/data/VITAHomebrewSorter/backup/app.db.bkp";
        else
            backup_path = "ux0:/data/VITAHomebrewSorter/backup/app.db";
        
        unsigned char *data = nullptr;
        SceOff size = 0;

        int ret = 0;
        if (R_FAILED(ret = FS::ReadFile(path, &data, &size)))
            return ret;

        if (FS::FileExists(backup_path)) {
            if (R_FAILED(ret = FS::RemoveFile(backup_path)))
                return ret;
        }

        if (R_FAILED(ret = FS::WriteFile(backup_path, data, size)))
            return ret; 
        
        return 0;
    }

    int Restore(void) {
        std::string restore_path;
        unsigned char *data = nullptr;
        SceOff size = 0;

        if (!FS::FileExists("ux0:/data/VITAHomebrewSorter/backup/app.db"))
            restore_path = "ux0:/data/VITAHomebrewSorter/backup/app.db.bkp";
        else
            restore_path = "ux0:/data/VITAHomebrewSorter/backup/app.db";

        int ret = 0;
        if (R_FAILED(ret = FS::ReadFile(restore_path.c_str(), &data, &size)))
            return ret;
            
        if (FS::FileExists(path)) {
            if (R_FAILED(ret = FS::RemoveFile(path)))
                return ret;
        }

        if (R_FAILED(ret = FS::WriteFile(path, data, size)))
            return ret; 
        
        return 0;
    }

    bool Compare(const char *db_name) {
        std::vector<std::string> app_entries;
        std::vector<std::string> loadout_entries;
        std::vector<std::string> diff_entries;

        sqlite3 *db;
        int ret = sqlite3_open_v2(path, &db, SQLITE_OPEN_READWRITE, nullptr);

        if (ret)
            return false;

        std::string query = "SELECT title FROM tbl_appinfo_icon;";
        
        sqlite3_stmt *stmt;
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

        std::string db_path = "ux0:data/VITAHomebrewSorter/loadouts/" + std::string(db_name);
        ret = sqlite3_open_v2(db_path.c_str(), &db, SQLITE_OPEN_READWRITE, nullptr);

        if (ret)
            return false;
        
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

        if (app_entries.empty() || loadout_entries.empty())
            return false;
        
        std::set_difference(app_entries.begin(), app_entries.end(), loadout_entries.begin(), loadout_entries.end(),
        std::inserter(diff_entries, diff_entries.begin()));
        return (diff_entries.size() > 0);
    }
}
