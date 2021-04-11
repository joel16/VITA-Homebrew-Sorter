#ifndef _VITA_HB_SORTER_UTILS_H_
#define _VITA_HB_SORTER_UTILS_H_

#include <psp2/ctrl.h>
#include <psp2/system_param.h>

/// Checks whether a result code indicates success.
#define R_SUCCEEDED(res)   ((res)>=0)
/// Checks whether a result code indicates failure.
#define R_FAILED(res)      ((res)<0)
/// Returns the level of a result code.

extern int SCE_CTRL_ENTER, SCE_CTRL_CANCEL;

namespace Utils {
    int InitAppUtil(void);
    int EndAppUtil(void);
    int GetEnterButton(void);
    int GetCancelButton(void);
    int GetDateFormat(void);
    void GetDateString(char string[24], SceSystemParamDateFormat format, SceDateTime *time);
}

#endif
