#include <algorithm>
#include <cstdio>
#include <string>

#include "applist.h"
#include "fs.h"
#include "log.h"
#include "sqlite3.h"
#include "utils.h"

namespace AppList {
    constexpr char path[] = "ur0:shell/db/app.db";
    constexpr char path_edit[] = "ur0:shell/db/app.vhb.db";

    int Get(AppEntries *entries) {
        entries->icons.clear();
        entries->pages.clear();
        entries->folders.clear();

        sqlite3 *db;
        int ret = sqlite3_open_v2(path, &db, SQLITE_OPEN_READWRITE, nullptr);

        if (ret)
            return -1;

        std::string query = 
            std::string("SELECT info_icon.pageId, info_page.pageNo, info_icon.pos, info_icon.title, info_icon.titleId, info_icon.reserved01, ")
            + "info_icon.reserved02, info_icon.icon0Type "
            + "FROM tbl_appinfo_icon info_icon "
            + "INNER JOIN tbl_appinfo_page info_page "
            + "ON info_icon.pageId = info_page.pageId;";
        
        sqlite3_stmt *stmt;
        ret = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);

        while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
            AppInfoIcon icon;
            icon.pageId = std::stoi(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
            icon.pageNo = sqlite3_column_int(stmt, 1);
            icon.pos = sqlite3_column_int(stmt, 2);
            std::snprintf(icon.title, 128, "%s", sqlite3_column_text(stmt, 3));
            std::snprintf(icon.titleId, 16, "%s", sqlite3_column_text(stmt, 4));
            std::snprintf(icon.reserved01, 16, "%s", sqlite3_column_text(stmt, 5));
            std::snprintf(icon.reserved02, 128, "%s", sqlite3_column_text(stmt, 6));
            icon.folder = (std::stoi(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)))) == 7? true : false;
            entries->icons.push_back(icon);
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
                entries->pages.push_back(page);
            }
            else if (pageNo < 0) {
                folder.pageId = sqlite3_column_int(stmt, 0);
                entries->folders.push_back(folder);
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
        int ret = 0;
        
        if (R_FAILED(ret = FS::CopyFile(path, path_edit)))
            return ret;

        ret = sqlite3_open_v2(path_edit, &db, SQLITE_OPEN_READWRITE, nullptr);
        if (ret) {
            FS::RemoveFile(path_edit);
            return ret;
        }

        // Hacky workaround to avoid SQL's unique constraints. Please look away!
        for (int i = 0, counter = 10; i < entries.size(); i++, counter++) {
            std::string title = entries[i].title;
            std::string titleId = entries[i].titleId;
            std::string reserved01 = entries[i].reserved01;
            std::string reserved02 = entries[i].reserved02;
            std::string query = 
                std::string("UPDATE tbl_appinfo_icon ")
                + "SET pageId = " + std::to_string(entries[i].pageId) + ", pos = " + std::to_string(counter) + " "
                + "WHERE ";

            if ((title == "(null)") && (titleId == "(null)")) {
                if (reserved01 != "(null)")
                    query.append("reserved01 = " + reserved01 + ";");
                else if (reserved02 != "(null)")
                    query.append("reserved02 = '" + reserved02 + "';");
            }
            else {
                query.append((titleId == "(null)"? "title = '" + title + "'" : "titleId = '" + titleId + "'")
                + (entries[i].folder == true? " AND reserved01 = " + reserved01 + ";" : ";"));
            }
            
            ret = sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr);
            if (ret != SQLITE_OK) {
                Log::Error("sqlite3_exec1 error %s\n", query.c_str());
                sqlite3_close(db);
                FS::RemoveFile(path_edit);
                return ret;
            }
        }

        for (int i = 0; i < entries.size(); i++) {
            std::string title = entries[i].title;
            std::string titleId = entries[i].titleId;
            std::string reserved01 = entries[i].reserved01;
            std::string reserved02 = entries[i].reserved02;
            std::string query = 
                std::string("UPDATE tbl_appinfo_icon ")
                + "SET pageId = " + std::to_string(entries[i].pageId) + ", pos = " + std::to_string(entries[i].pos) + " "
                + "WHERE ";

            if ((title == "(null)") && (titleId == "(null)")) {
                if (reserved01 != "(null)")
                    query.append("reserved01 = " + reserved01 + ";");
                else if (reserved02 != "(null)")
                    query.append("reserved02 = '" + reserved02 + "';");
            }
            else {
                query.append((titleId == "(null)"? "title = '" + title + "'" : "titleId = '" + titleId + "'")
                + (entries[i].folder == true? " AND reserved01 = " + reserved01 + ";" : ";"));
            }

            ret = sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr);
            if (ret != SQLITE_OK) {
                Log::Error("sqlite3_exec2 error %s\n", query.c_str());
                sqlite3_close(db);
                FS::RemoveFile(path_edit);
                return ret;
            }
        }

        sqlite3_close(db);

        if (R_FAILED(ret = FS::CopyFile(path_edit, path)))
            return ret;

        FS::RemoveFile(path_edit);
        return 0;
    }

    bool SortAlphabeticalAsc(const AppInfoIcon &entryA, const AppInfoIcon &entryB) {
        std::string entryAname = sort_mode == 0? entryA.title : entryA.titleId;
        std::string entryBname = sort_mode == 0? entryB.title : entryB.titleId;

        std::transform(entryAname.begin(), entryAname.end(), entryAname.begin(), [](unsigned char c){ return std::tolower(c); });
        std::transform(entryBname.begin(), entryBname.end(), entryBname.begin(), [](unsigned char c){ return std::tolower(c); });

        if (entryAname.compare(entryBname) < 0)
            return true;

        return false;
    }

    bool SortAlphabeticalDesc(const AppInfoIcon &entryA, const AppInfoIcon &entryB) {
        std::string entryAname = sort_mode == 0? entryA.title : entryA.titleId;
        std::string entryBname = sort_mode == 0? entryB.title : entryB.titleId;

        std::transform(entryAname.begin(), entryAname.end(), entryAname.begin(), [](unsigned char c){ return std::tolower(c); });
        std::transform(entryBname.begin(), entryBname.end(), entryBname.begin(), [](unsigned char c){ return std::tolower(c); });

        if (entryBname.compare(entryAname) < 0)
            return true;

        return false;
    }

    void Sort(AppEntries *entries) {
        int pos = 0, pageCounter = 0;
        for (int i = 0; i < entries->icons.size(); i ++) {
            // Reset position
            if (pos > 9) {
                pos = 0;
                pageCounter++;
            }
            
            // App/Game belongs to a folder
            if (entries->icons[i].pageNo < 0) {
                for (int j = 0; j < entries->folders.size(); j++) {
                    if (entries->icons[i].pageId == entries->folders[j].pageId) {
                        entries->icons[i].pos = entries->folders[j].index;
                        entries->folders[j].index++;
                    }
                }
            }
            else {
                entries->icons[i].pos = pos;
                entries->icons[i].pageId = entries->pages[pageCounter].pageId;
                pos++;
            }
        }
    }

    int Backup(void) {
        int ret = 0;
        std::string backup_path;
        
        if (!FS::FileExists("ux0:data/VITAHomebrewSorter/backup/app.db.bkp"))
            backup_path = "ux0:data/VITAHomebrewSorter/backup/app.db.bkp";
        else
            backup_path = "ux0:data/VITAHomebrewSorter/backup/app.db";
            
        if (R_FAILED(ret = FS::CopyFile(path, backup_path)))
            return ret;
            
        return 0;
    }

    int Restore(void) {
        int ret = 0;
        std::string restore_path;

        if (!FS::FileExists("ux0:data/VITAHomebrewSorter/backup/app.db"))
            restore_path = "ux0:data/VITAHomebrewSorter/backup/app.db.bkp";
        else
            restore_path = "ux0:data/VITAHomebrewSorter/backup/app.db";

        if (R_FAILED(ret = FS::CopyFile(restore_path, path)))
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
