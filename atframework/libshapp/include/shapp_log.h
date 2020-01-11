

#ifndef ATFRAME_LIBSHAPP_SHAPP_LOG_H
#define ATFRAME_LIBSHAPP_SHAPP_LOG_H
#pragma once

#include "log/log_wrapper.h"

namespace  util{
    namespace log {
        extern int init_log();
    }
}



#define LOGF_TRACE WLOGTRACE
#define LOGF_DEBUG WLOGDEBUG
#define LOGF_INFO  WLOGINFO
#define LOGF_WARN  WLOGWARNING
#define LOGF_ERROR WLOGERROR
#define LOGF_FATAL WLOGFATAL

#endif //ATFRAME_LIBSHAPP_SHAPP_LOG_H
