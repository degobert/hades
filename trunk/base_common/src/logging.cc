/*************************************************************************
  > File Name   : logging.cc
  > Mail        : dh  /dh03@163.com
  > Created Time: Mon 16 Oct 2017 10:45:41 AM CST
  > Version     : 1.0.0
 ************************************************************************/
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <log4cplus/ndc.h>

#include "logging.h"

int log_level = -1;

DISPATCHER_BS

using namespace std;

bool Logging::Init(const std::string& filename) {
    struct stat st;
    // If the config file is not exist, or is not a regular file or a symlink,
    // the path is invalid.
    if (stat(filename.c_str(), &st) < 0 || !(S_ISREG(st.st_mode) || S_ISLNK(st.st_mode))) {
        std::cerr << "Logging init failed! '" << filename << "' (" << strerror(errno) << ")\n";
        return false;
    }
    qlog::qLogConfig(filename);
    return true;
}

ScopedLoggingCtx::ScopedLoggingCtx(const std::string& ctx) {
    log4cplus::getNDC().push(ctx);
}

ScopedLoggingCtx::~ScopedLoggingCtx() {
    log4cplus::getNDC().pop();
}





DISPATCHER_ES
