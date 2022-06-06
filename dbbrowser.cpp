#include <psp2/kernel/clib.h>

#include "dbbrowser.h"
#include "log.h"
#include "sqlite3.h"
#include "utils.h"

namespace DBBrowser {
    int GetTables(std::vector<std::string> &tables) {
        sqlite3 *db = nullptr;

        int ret = sqlite3_open_v2(db_path, &db, SQLITE_OPEN_READWRITE, nullptr);
        if (ret != SQLITE_OK) {
            Log::Error("sqlite3_open_v2 failed to open %s\n", db_path);
            return ret;
        }

        std::string query = std::string("SELECT tbl_name FROM sqlite_schema WHERE type = 'table';");

        sqlite3_stmt *stmt = nullptr;
        ret = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);

        while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
            char res[64];
            sceClibSnprintf(res, 64, "%s", sqlite3_column_text(stmt, 0));
            tables.push_back(res);
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

}
