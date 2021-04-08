#ifndef _VITA_HB_SORTER_LOG_H_
#define _VITA_HB_SORTER_LOG_H_

namespace Log {
    int Init(void);
    int Exit(void);
    int Error(const char *format, ...);
}

#endif
