#pragma once

namespace Log {
    int Init(void);
    int Exit(void);
    int Error(const char *format, ...);
}
