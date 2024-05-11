#include <psp2/apputil.h>
#include <psp2/common_dialog.h>
#include <psp2/rtc.h>
#include <psp2/kernel/clib.h>

#include "log.h"
#include "utils.h"

namespace Utils {
    int InitAppUtil(void) {
        SceAppUtilInitParam init;
        SceAppUtilBootParam boot;
        sceClibMemset(&init, 0, sizeof(SceAppUtilInitParam));
        sceClibMemset(&boot, 0, sizeof(SceAppUtilBootParam));
        
        int ret = 0;
        
        if (R_FAILED(ret = sceAppUtilInit(&init, &boot))) {
            Log::Error("sceAppUtilInit failed: 0x%lx\n", ret);
            return ret;
        }
        
        SceCommonDialogConfigParam param;
        sceCommonDialogConfigParamInit(&param);

        if (R_FAILED(ret = sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, reinterpret_cast<int *>(&param.language)))) {
            Log::Error("sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG) failed: 0x%lx\n", ret);
            return ret;
        }

        if (R_FAILED(ret = sceCommonDialogSetConfigParam(&param))) {
            Log::Error("sceCommonDialogSetConfigParam failed: 0x%lx\n", ret);
            return ret;
        }
        
        return 0;
    }

    int EndAppUtil(void) {
        int ret = 0;
        
        if (R_FAILED(ret = sceAppUtilShutdown())) {
            Log::Error("sceAppUtilShutdown failed: 0x%lx\n", ret);
            return ret;
        }
        
        return 0;
    }

    int GetDateFormat(void) {
        int format = 0, ret = 0;

        if (R_FAILED(ret = sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_DATE_FORMAT, &format))) {
            Log::Error("sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_DATE_FORMAT) failed: 0x%lx\n", ret);
            return ret;
        }
        
        return format;
    }

    // From VitaShell by TheOfficialFloW -> https://github.com/TheOfficialFloW/VitaShell/blob/master/utils.c#L349
    static void ConvertLocalTimeToUTC(SceDateTime &time_utc, SceDateTime &time_local) {
        // sceRtcGetTick and other sceRtc functions fails with year > 9999
        int year_local = time_local.year;
        int year_delta = year_local < 9999 ? 0 : year_local - 9998;
        time_local.year -= year_delta;
        
        SceRtcTick tick;
        sceRtcGetTick(&time_local, &tick);
        time_local.year = year_local;
        
        sceRtcConvertLocalTimeToUtc(&tick, &tick);
        sceRtcSetTick(&time_utc, &tick);  
        time_utc.year += year_delta;
    }

    // From VitaShell by TheOfficialFloW -> https://github.com/TheOfficialFloW/VitaShell/blob/master/utils.c#L364
    void GetDateString(char string[24], SceSystemParamDateFormat format, SceDateTime &time) {
        SceDateTime local_time;
        Utils::ConvertLocalTimeToUTC(local_time, time);
        
        switch (format) {
            case SCE_SYSTEM_PARAM_DATE_FORMAT_YYYYMMDD:
                sceClibSnprintf(string, 24, "%04d/%02d/%02d", local_time.year, local_time.month, local_time.day);
                break;
                
            case SCE_SYSTEM_PARAM_DATE_FORMAT_DDMMYYYY:
                sceClibSnprintf(string, 24, "%02d/%02d/%04d", local_time.day, local_time.month, local_time.year);
                break;
                
            case SCE_SYSTEM_PARAM_DATE_FORMAT_MMDDYYYY:
                sceClibSnprintf(string, 24, "%02d/%02d/%04d", local_time.month, local_time.day, local_time.year);
                break;
        }
    }
}
