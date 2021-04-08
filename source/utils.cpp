#include <psp2/apputil.h>
#include <psp2/kernel/clib.h>
#include <psp2/common_dialog.h>
#include <psp2/system_param.h>
#include <cstdio>

#include "utils.h"

int SCE_CTRL_ENTER, SCE_CTRL_CANCEL;

namespace Utils {
    static SceCtrlData pad, old_pad;

    int InitAppUtil(void) {
        SceAppUtilInitParam init;
        SceAppUtilBootParam boot;
        sceClibMemset(&init, 0, sizeof(SceAppUtilInitParam));
        sceClibMemset(&boot, 0, sizeof(SceAppUtilBootParam));
        
        int ret = 0;
        
        if (R_FAILED(ret = sceAppUtilInit(&init, &boot))) {
            printf("sceAppUtilInit failed: 0x%lx\n", ret);
            return ret;
        }
        
        SceCommonDialogConfigParam param;
        sceCommonDialogConfigParamInit(&param);

        if (R_FAILED(ret = sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, reinterpret_cast<int *>(&param.language)))) {
            printf("sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG) failed: 0x%lx\n", ret);
            return ret;
        }

        if (R_FAILED(ret = sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, reinterpret_cast<int *>(&param.enterButtonAssign)))) {
            printf("sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON) failed: 0x%lx\n", ret);
            return ret;
        }

        if (R_FAILED(ret = sceCommonDialogSetConfigParam(&param))) {
            printf("sceCommonDialogSetConfigParam failed: 0x%lx\n", ret);
            return ret;
        }
        
        return 0;
    }

    int EndAppUtil(void) {
        int ret = 0;
        
        if (R_FAILED(ret = sceAppUtilShutdown())) {
            printf("sceAppUtilShutdown failed: 0x%lx\n", ret);
            return ret;
        }
        
        return 0;
    }

    int GetEnterButton(void) {
        int button = 0, ret = 0;
        if (R_FAILED(ret = sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, &button))) {
            printf("sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON) failed: 0x%lx\n", ret);
            return ret;
        }
        
        if (button == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE)
            return SCE_CTRL_CIRCLE;
        else
            return SCE_CTRL_CROSS;
        
        return 0;
    }

    int GetCancelButton(void) {
        int button = 0, ret = 0;
        if (R_FAILED(ret = sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, &button))) {
            printf("sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON) failed: 0x%lx\n", ret);
            return ret;
        }
        
        if (button == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE)
            return SCE_CTRL_CROSS;
        else
            return SCE_CTRL_CIRCLE;
        
        return 0;
    }
}
