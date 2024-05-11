#pragma once

#include <psp2/system_param.h>

/// Checks whether a result code indicates success.
#define R_SUCCEEDED(res) ((res)>=0)
/// Checks whether a result code indicates failure.
#define R_FAILED(res)    ((res)<0)
/// Returns the level of a result code.

constexpr char db_path[] = "ur0:shell/db/app.db";

namespace Utils {
    int InitAppUtil(void);
    int EndAppUtil(void);
    int GetDateFormat(void);
    void GetDateString(char string[24], SceSystemParamDateFormat format, SceDateTime &time);
}
