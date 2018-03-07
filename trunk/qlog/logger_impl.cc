// logger_impl.cc (2014-03-06)

//#include <glog/logging.h>
#include <algorithm>

#include <log4cplus/spi/loggingevent.h>
#include <log4cplus/helpers/loglog.h>
#include "logger_impl.h"
#include "socketappender.h"

using namespace log4cplus;
using namespace log4cplus::helpers;

namespace qlog {

void LoggerImpl::callAppenders(
        const log4cplus::spi::InternalLoggingEvent& event)
{
    //LOG(INFO) << "Enter callAppenders";
    int writes = 0;
    for(const LoggerImpl* c = this; c != NULL;
            c = (LoggerImpl *)c->parent.get()) {
        log4cplus::spi::LoggerImpl* p = (log4cplus::spi::LoggerImpl *)c;
        if (p->getName() == "root") {
            writes += p->appendLoopOnAppenders(event);
        }
        else {
            writes += c->appendLoopOnAppenders(event);
        }

        if(!c->additive) {
            break;
        }   
    } 
    //LOG(INFO) << "Enter callAppenders";
}

void LoggerImpl::callAppenders(
        const log4cplus::spi::InternalLoggingEvent& event, bool is_relay_msg)
{
    //LOG(INFO) << "Enter callAppenders";
    int writes = 0;
    for(const LoggerImpl* c = this; c != NULL;
            c = (LoggerImpl *)c->parent.get()) {
        log4cplus::spi::LoggerImpl* p = (log4cplus::spi::LoggerImpl *)c;
        if (p->getName() == "root") {
            writes += p->appendLoopOnAppenders(event);
        }
        else {
            writes += c->appendLoopOnAppenders(event, is_relay_msg);
        }

        if(!c->additive) {
            break;
        }   
    } 
    //LOG(INFO) << "Enter callAppenders";
}

void LoggerImpl::addAppender(log4cplus::SharedAppenderPtr newAppender)
{
    if(newAppender == NULL) {
        getLogLog().warn( LOG4CPLUS_TEXT("Tried to add NULL appender") );
        return;
    }

    pthread_rwlock_wrlock(&rwlock_);

    ListType::iterator it = 
        std::find(appenderList.begin(), appenderList.end(), newAppender);
    if(it == appenderList.end()) {
        appenderList.push_back(newAppender);
    }
    pthread_rwlock_unlock(&rwlock_);
}

AppenderAttachableImpl::ListType LoggerImpl::getAllAppenders()
{
    pthread_rwlock_rdlock(&rwlock_);

    AppenderAttachableImpl::ListType l = appenderList;

    pthread_rwlock_unlock(&rwlock_);

    return l;
}

log4cplus::SharedAppenderPtr LoggerImpl::getAppender(
        const log4cplus::tstring& name)
{
    //LOG(INFO) << "Enter getAppender";

    pthread_rwlock_rdlock(&rwlock_);

    log4cplus::SharedAppenderPtr p(NULL);

    for(ListType::iterator it=appenderList.begin(); 
            it!=appenderList.end(); 
            ++it)
    {
        if((*it)->getName() == name) {
            p = *it;
        }
    }

    pthread_rwlock_unlock(&rwlock_);

    return p;
}

void LoggerImpl::removeAllAppenders()
{
    pthread_rwlock_wrlock(&rwlock_);

    appenderList.erase(appenderList.begin(), appenderList.end());

    pthread_rwlock_unlock(&rwlock_);
}

void LoggerImpl::removeAppender(log4cplus::SharedAppenderPtr appender)
{
    if (appender == NULL) {
        getLogLog().warn( LOG4CPLUS_TEXT("Tried to remove NULL appender") );
        return;
    }

    pthread_rwlock_wrlock(&rwlock_);

    ListType::iterator it =
        std::find(appenderList.begin(), appenderList.end(), appender);
    if(it != appenderList.end()) {
        appenderList.erase(it);
    }

    pthread_rwlock_unlock(&rwlock_);
}

int LoggerImpl::appendLoopOnAppenders(
        const log4cplus::spi::InternalLoggingEvent& event) const
{
    int count = 0;

    pthread_rwlock_rdlock(const_cast<pthread_rwlock_t *>(&rwlock_));

    for(ListType::const_iterator it=appenderList.begin();
            it!=appenderList.end();
            ++it)
    {
        ++count;
        (*it)->doAppend(event);
    }

    pthread_rwlock_unlock(const_cast<pthread_rwlock_t *>(&rwlock_));

    return count;
}

int LoggerImpl::appendLoopOnAppenders(
        const log4cplus::spi::InternalLoggingEvent& event, bool is_relay_msg) const
{
    int count = 0;

    pthread_rwlock_rdlock(const_cast<pthread_rwlock_t *>(&rwlock_));

    for(ListType::const_iterator it=appenderList.begin();
            it!=appenderList.end();
            ++it)
    {
        ++count;
        SocketAppender* append = NULL;
        if ((append = dynamic_cast<SocketAppender*>(it->get()))) {
            append->doAppend(event, is_relay_msg);
        } else {
            (*it)->doAppend(event);
        }
    }

    pthread_rwlock_unlock(const_cast<pthread_rwlock_t *>(&rwlock_));

    return count;
}

} //namespace qlog
