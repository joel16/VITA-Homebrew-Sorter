/*
 * SQLite Vita R/W Demo - Copyright © 2017 VitaSmith
 * Based on libftpvita sample - Copyright © 2015 Sergi Granell (xerpi)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>

#include <psp2/ctrl.h>
#include <psp2/sqlite.h>
#include <psp2/display.h>
#include <psp2/apputil.h>
#include <psp2/sysmodule.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/io/fcntl.h>

#include "console.h"
#include "sqlite3.h"

static char* db_path   = "ux0:data/sqlite_demo.db";
static char* db_schema = "CREATE TABLE Demo (" \
	"NAME TEXT NOT NULL UNIQUE," \
	"VALUE INTEGER NOT NULL," \
	"PRIMARY KEY(VALUE)" \
	")";
static int row_nr = 0;

static int callback(void *data, int argc, char **argv, char **column_name)
{
	printf("ROW %d:\n", row_nr++);
	for (int i = 0; i < argc; i++)
		printf("  %s = %s\n", column_name[i], argv[i] ? argv[i] : "NULL");
	return 0;
}

int main()
{
	int rc;
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	char *errmsg = NULL;
	char sql[64];
	SceUID fd;
	SceCtrlData pad;

	init_video();
	console_init();

	/* Initialize SQLite for R/W */
	printf("Initializing SQLite...\n");
	sceSysmoduleLoadModule(SCE_SYSMODULE_SQLITE);
	sqlite3_rw_init();

	/* Open or create the database */
	fd = sceIoOpen(db_path, SCE_O_RDONLY, 0777);
	if (fd > 0) {
		sceIoClose(fd);
		printf("Opening existing database '%s'...\n", db_path);
		rc = sqlite3_open_v2(db_path, &db, SQLITE_OPEN_READWRITE, NULL);
	} else {
		printf("Creating new database '%s'...\n", db_path);
		rc = sqlite3_open_v2(db_path, &db, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE, NULL);
		if (rc != SQLITE_OK)
			goto out;
		printf("Creating schema '%s'...\n", db_path);
		rc = sqlite3_exec(db, db_schema, NULL, NULL, &errmsg);
		if (rc != SQLITE_OK)
			goto out;
	}

	/* Get the current maximum value */
	printf("Querying MAX(VALUE)...\n");
	rc = sqlite3_prepare_v2(db, "SELECT MAX(VALUE) FROM Demo", -1, &stmt, NULL);
	if (rc != SQLITE_OK)
		goto out;
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_ROW)
		goto out;
	int max = sqlite3_column_int(stmt, 0) + 1;
	rc = sqlite3_finalize(stmt);

	/* Insert new value */
	snprintf(sql, sizeof(sql), "INSERT INTO Demo VALUES('TEST_%03d', %d)", max, max);
	printf("Running SQL: `%s`\n", sql);
	rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
	if (rc != SQLITE_OK)
		goto out;

	/* Dump the table */
	printf("Dumping database...\n\n");
	console_set_color(GREEN);
	row_nr = 1;
	rc = sqlite3_exec(db, "SELECT * FROM Demo", callback, NULL, &errmsg);
	if (rc != SQLITE_OK)
		goto out;

out:
	if (rc != SQLITE_OK) {
		console_set_color(RED);
		printf("ERROR %d: %s\n", rc, (errmsg != NULL) ? errmsg : sqlite3_errmsg(db));
	}
	sqlite3_free(errmsg);
	if (db != NULL)
		sqlite3_close(db);
	sqlite3_rw_exit();
	console_set_color(CYAN);
	printf("\nPress X to exit.\n");
	do {
		sceCtrlPeekBufferPositive(0, &pad, 1);
	} while (!(pad.buttons & SCE_CTRL_CROSS));
	console_exit();
	end_video();
	sceKernelExitProcess(0);
	return 0;
}
