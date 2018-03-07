// Module:  Log4CPLUS
// File:    dynamicappender.cxx
// Created: 6/2003
// Author:  Tad E. Smith
//
//
// Copyright (C) Tad E. Smith  All rights reserved.
//
// This software is published under the terms of the Apache Software
// License version 1.1, a copy of which has been included with this
// distribution in the LICENSE.APL file.
//
// $Log: nullappender.cxx,v $
// Revision 1.2  2003/07/30 05:52:29  tcsmith
// Modified to remove "unused parameter" warning.
//
// Revision 1.1  2003/06/23 21:02:53  tcsmith
// Initial version.
//

#include <stdio.h>
#include <dlfcn.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/helpers/logloguser.h>
#include <log4cplus/spi/loggingevent.h>
#include "dynamicappender.h"

using namespace std;
using namespace log4cplus;
using namespace log4cplus::helpers;


namespace qlog {

///////////////////////////////////////////////////////////////////////////////
// DynamicAppender ctors and dtor
///////////////////////////////////////////////////////////////////////////////

DynamicAppender::DynamicAppender(const log4cplus::tstring& path
        , const log4cplus::tstring& conf)
    : mod_path_(path)
      , mod_conf_path_(conf)
      , init_func_(NULL)
      , process_func_(NULL)
      , so_handler_(NULL)
{
}


DynamicAppender::DynamicAppender(const log4cplus::helpers::Properties& properties)
: log4cplus::Appender(properties)
    , init_func_(NULL)
    , process_func_(NULL)
    , so_handler_(NULL)
{
    mod_path_ = properties.getProperty( LOG4CPLUS_TEXT("path"));
    mod_conf_path_ = properties.getProperty( LOG4CPLUS_TEXT("conf"));
}



DynamicAppender::~DynamicAppender()
{
    destructorImpl();
}



///////////////////////////////////////////////////////////////////////////////
// DynamicAppender public methods
///////////////////////////////////////////////////////////////////////////////

void
DynamicAppender::close()
{
    if (so_handler_ != NULL) {
        init_func_ = NULL;
        process_func_ = NULL;

        dlclose(so_handler_);
        so_handler_ = NULL;
    }
}



///////////////////////////////////////////////////////////////////////////////
// DynamicAppender protected methods
///////////////////////////////////////////////////////////////////////////////

// This method does not need to be locked since it is called by
// doAppend() which performs the locking
void
DynamicAppender::append(const spi::InternalLoggingEvent& event)
{
    if (so_handler_ == NULL) {
        if (!LoadSo()) {
            getLogLog().error("Load module failed");
            return;
        }
    }

    if (process_func_ == NULL) {
        getLogLog().error("LogDispatchProcess func pointer should not be null");
        return;
    }

    LogAttr attr;
    attr.sec_ = event.getTimestamp().sec();
    attr.usec_ = event.getTimestamp().usec();
    attr.host_ = event.getNDC();
    attr.level_ = event.getLogLevel();

    process_func_(&attr, event.getMessage().c_str(), event.getMessage().size());
}

bool DynamicAppender::LoadSo()
{
    if (so_handler_ != NULL) {
        return true;
    }

    char strbuf[4096];

    if (mod_path_.empty()) {
        getLogLog().error("module path should not be emtpy");
        return false;
    }

    if (mod_conf_path_.empty()) {
        getLogLog().error("module config file should not be emtpy");
        return false;
    }

    so_handler_ = dlopen(mod_path_.c_str(), RTLD_LAZY);
    if (so_handler_ == NULL) {
        sprintf(strbuf, "Failed to load module \"%s\" : %s", mod_path_.c_str(), dlerror());
        getLogLog().error(strbuf);
        return false;
    }

    void *fn = dlsym(so_handler_, "LogDispatchInit");
    if (fn == NULL) {
        sprintf(strbuf, "Failed to get LogDispatchInit function \"%s\" : %s", mod_path_.c_str(), dlerror());
        getLogLog().error(strbuf);
        dlclose(so_handler_);
        so_handler_ = NULL;
        return false;
    }
    init_func_ = (InitFunc)fn;

    fn = dlsym(so_handler_, "LogDispatchProcess");
    if (fn == NULL) {
        sprintf(strbuf, "Failed to get LogDispatchProcess function \"%s\" : %s", mod_path_.c_str(), dlerror());
        getLogLog().error(strbuf);
        dlclose(so_handler_);
        so_handler_ = NULL;
        init_func_ = NULL;
        return false;
    }
    process_func_ = (ProcessFunc)fn;

    //call LogDispatchInit for mod
    if(!init_func_(mod_conf_path_.c_str())) {
        sprintf(strbuf, "call LogDispatchInit function failed");
        getLogLog().error(strbuf);
        dlclose(so_handler_);
        so_handler_ = NULL;
        init_func_ = NULL;
        process_func_ = NULL;
        return false;
    }

    return true;
}

} /*qlog*/



