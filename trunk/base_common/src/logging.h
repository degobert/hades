/*************************************************************************
  > File Name   : logging.h
  > Author      : dh
  > Mail        : dh  /dh03@163.com
  > Created Time: Mon 16 Oct 2017 10:45:14 AM CST
  > Version     : 1.0.0
 ************************************************************************/
#ifndef BASE_COMMON_LOGGING_H
#define BASE_COMMON_LOGGING_H

#include "qlog/qlog.h"
#include "base_common.h"

#define LOGGING_INIT(name) static const char* _cloud_ucs_logger = name
#define QLOG(level) \
    if ( (level) >= getLogLevel() ) qLogs(_cloud_ucs_logger, level)

#define LOG(level) \
    if ( (level) >= getLogLevel() ) qLogs(_cloud_ucs_logger, level)

extern int log_level ;
inline void setLogLevel(int _log_level) {
    log_level = _log_level;
}

inline int getLogLevel() {
    return log_level;
}

#define LOG_DEBUG( fmt, args...) \
    do { \
        if ( (DEBUG) >= getLogLevel() ) \
        { \
            qlog::qLogAll(_cloud_ucs_logger, DEBUG,  0, false, __FILE__, __LINE__, fmt, ##args); \
        } \
    } while(0)

#define LOG_INFO( fmt, args...) \
    do { \
        if ( (INFO) >= getLogLevel() ) \
        { \
            qlog::qLogAll(_cloud_ucs_logger, INFO,  0, false, __FILE__, __LINE__, fmt, ##args); \
        } \
    } while(0)

#define LOG_WARN( fmt, args...) \
    do { \
        if ( (WARN) >= getLogLevel() ) \
        { \
            qlog::qLogAll(_cloud_ucs_logger, WARN,  0, false, __FILE__, __LINE__, fmt, ##args); \
        } \
    } while(0)

#define LOG_ERROR( fmt, args...) \
    do { \
        if ( (ERROR) >= getLogLevel() ) \
        { \
            qlog::qLogAll(_cloud_ucs_logger, ERROR,  0, false, __FILE__, __LINE__, fmt, ##args); \
        } \
    } while(0)


DISPATCHER_BS

const int TRACE   = qlog::QLOG_TRACE;
const int DEBUG   = qlog::QLOG_DEBUG;
const int INFO    = qlog::QLOG_INFO;
const int WARNING = qlog::QLOG_WARN;
const int ERROR   = qlog::QLOG_ERROR;
const int FATAL   = qlog::QLOG_FATAL;

class Logging {
public:
    static bool Init(const std::string& filename);
};

class ScopedLoggingCtx {
public:
    ScopedLoggingCtx(const std::string& ctx);
    ~ScopedLoggingCtx();
};

namespace _logging_internal {
    class LoggingVoidify {
    public:
        LoggingVoidify() {}
        void operator&(std::ostream&) {}
    };
}




DISPATCHER_ES
#endif
