#include "socketappender.h"
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <string.h>

#include <functional>
#include <boost/make_shared.hpp>

#include <log4cplus/layout.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/spi/loggingevent.h>
#include <log4cplus/helpers/stringhelper.h>
#include <log4cplus/helpers/pointer.h>

#include "compression.h"
#include "socket_pool.h"

using namespace std;
using namespace log4cplus;
using namespace log4cplus::helpers;

#define LOG4CPLUS_MESSAGE_VERSION 20
#define LOG4CPLUS_MESSAGE_VERSION_COMPRESS 30

//////////////////////////////////////////////////////////////////////////////
// SocketAppender ctors and dtor
//////////////////////////////////////////////////////////////////////////////

//add by hanyugang
#if !defined(_WIN32)
#include <unistd.h>
#include <errno.h>
#include <climits>
#include <sys/stat.h>
#endif

using namespace log4cplus;
using namespace log4cplus::helpers;

namespace qlog {

const int DEFAULT_MAX_CONNECTS = 10;
const int DEFAULT_SEND_TIMEOUT = 5;
const int DEFAULT_CONNECT_TIMEOUT = 50;
const int DEFAULT_CONNECT_CHECK_INTERVAL = 10;

const tstring DEFAULT_RECORD_PATH = LOG4CPLUS_TEXT(
        "/home/s/tmp/qlog_missed_%Y%m%d.log");
const tstring DEFAULT_APPENDER_LOG_PATH = LOG4CPLUS_TEXT(
        "/home/s/tmp/qlog_socket_appender_%Y%m%d.log");
const int DEFAULT_SEND_BUFFER_SIZE = 512;

const int kDefaultRecvTimeout = 10;
const int kDefaultRetry = 1;

SocketAppender::SocketAppender(const log4cplus::tstring& host,
        const log4cplus::tstring& serverName,
        uint32_t send_timeout,
        time_t recordInterval,
        tstring recordPath)
    : reconnectDelay(500)
      , port(9998)
      , serverName(serverName)
      , lostCount(0)
      , successCount(0)
      , recordTime(Time::gettimeofday())
      , sendTimeout(send_timeout)
      , recvTimeout(kDefaultRecvTimeout)
      , recordInterval(recordInterval)
      , recordPath(recordPath)
      , recordFile(NULL)
      , currentPath(LOG4CPLUS_TEXT(""))
      , compress(LOG4CPLUS_TEXT("no"))
      , maxConnections(DEFAULT_MAX_CONNECTS)
      , connectTimeout(DEFAULT_CONNECT_TIMEOUT)
      , connectCheckInterval(DEFAULT_CONNECT_CHECK_INTERVAL)
      , maxRetry(kDefaultRetry)
      , cachedTime(true)
      , isInternalRelayMsg(false)
{
    InitSockets(host, port);

    char buf[PATH_MAX];
    gethostname(buf, PATH_MAX);
    this->serverName += LOG4CPLUS_C_STR_TO_TSTRING(buf);

}

SocketAppender::SocketAppender(const Properties& properties)
    : Appender(properties)
      , reconnectDelay(500)
      , port(9998)
      , lostCount(0)
      , successCount(0)
      , recordTime(Time::gettimeofday())
      , sendTimeout(DEFAULT_SEND_TIMEOUT)
      , recvTimeout(kDefaultRecvTimeout)
      , sendBufferSize(DEFAULT_SEND_BUFFER_SIZE)
      , recordInterval(0)
      , recordPath(DEFAULT_RECORD_PATH)
      , recordFile(NULL)
      , currentPath(LOG4CPLUS_TEXT(""))
      , compress(LOG4CPLUS_TEXT("no"))
      , maxConnections(DEFAULT_MAX_CONNECTS)
      , connectTimeout(DEFAULT_CONNECT_TIMEOUT)
      , connectCheckInterval(DEFAULT_CONNECT_CHECK_INTERVAL)
      , maxRetry(kDefaultRetry)
      , cachedTime(true)
      , isInternalRelayMsg(false)
{
    tstring raw_host = properties.getProperty( LOG4CPLUS_TEXT("host") );
    if (raw_host.empty()) {
        raw_host = properties.getProperty( LOG4CPLUS_TEXT("Host") );
    }

    if ( properties.exists( LOG4CPLUS_TEXT("port") ) ) {
        tstring tmp = properties.getProperty( LOG4CPLUS_TEXT("port") );
        port = atoi(LOG4CPLUS_TSTRING_TO_STRING(tmp).c_str());
    } else if (properties.exists( LOG4CPLUS_TEXT("Port") ) ) {
        tstring tmp = properties.getProperty( LOG4CPLUS_TEXT("Port") );
        port = atoi(LOG4CPLUS_TSTRING_TO_STRING(tmp).c_str());
    }


    serverName = properties.getProperty(LOG4CPLUS_TEXT("ServerName"));
    char buf[PATH_MAX];
    gethostname(buf, PATH_MAX);
    this->serverName += LOG4CPLUS_C_STR_TO_TSTRING(buf);
    if (properties.exists( LOG4CPLUS_TEXT("ReconnectDelay"))) {
        tstring tmp = properties.getProperty( LOG4CPLUS_TEXT("ReconnectDelay") );
        reconnectDelay = atoi(LOG4CPLUS_TSTRING_TO_STRING(tmp).c_str());
    }


    if (properties.exists(LOG4CPLUS_TEXT ("MaxConnections"))) {
        tstring tmp = properties.getProperty(LOG4CPLUS_TEXT ("MaxConnections"));
        int max_connects = atoi(LOG4CPLUS_TSTRING_TO_STRING(tmp).c_str());

        if (max_connects > 0) {
            maxConnections= max_connects;
        }
    }

    if (properties.exists(LOG4CPLUS_TEXT ("ConnectTimeout"))) {
        tstring tmp = properties.getProperty(LOG4CPLUS_TEXT ("ConnectTimeout"));
        int connect_timeout = atoi(LOG4CPLUS_TSTRING_TO_STRING(tmp).c_str());

        if (connect_timeout > 0) {
            connectTimeout = connect_timeout;
        }
    }

    if (properties.exists(LOG4CPLUS_TEXT ("ConnectCheckInterval"))) {
        tstring tmp = properties.getProperty(
                LOG4CPLUS_TEXT ("ConnectCheckInterval"));
        int check_interval = atoi(LOG4CPLUS_TSTRING_TO_STRING(tmp).c_str());
        if (check_interval > 0) {
            connectCheckInterval = check_interval;
        }
    }

    if (properties.exists(LOG4CPLUS_TEXT ("TimeoutUSec"))) {
        tstring tmp = properties.getProperty(LOG4CPLUS_TEXT ("TimeoutUSec"));
        int usec = atoi(LOG4CPLUS_TSTRING_TO_STRING(tmp).c_str());
        if (usec > 0) {
            sendTimeout = usec / 1000;
            if (sendTimeout == 0) {
                sendTimeout = 1;
            }
        }
        if (sendTimeout > 5000) {
            sendTimeout = 5000;
        }
    }

    if (properties.exists(LOG4CPLUS_TEXT ("SendTimeout"))) {
        tstring tmp = properties.getProperty(LOG4CPLUS_TEXT ("SendTimeout"));
        int msec = atoi(LOG4CPLUS_TSTRING_TO_STRING(tmp).c_str());
        if (msec > 0) {
            sendTimeout = msec;
        }
        if (sendTimeout > 5000) {
            sendTimeout = 5000;
        }
    }

    if (properties.exists(LOG4CPLUS_TEXT ("RecvTimeout"))) {
        tstring tmp = properties.getProperty(LOG4CPLUS_TEXT ("RecvTimeout"));
        int msec = atoi(LOG4CPLUS_TSTRING_TO_STRING(tmp).c_str());
        if (msec > 0) {
            recvTimeout = msec;
        }
        if (recvTimeout > 5000) {
            recvTimeout = 5000;
        }
    }

    if (properties.exists(LOG4CPLUS_TEXT("RecordInterval"))) {
        tstring tmp = properties.getProperty(LOG4CPLUS_TEXT("RecordInterval"));
        recordInterval = atoi(LOG4CPLUS_TSTRING_TO_STRING(tmp).c_str());
    }

    if (properties.exists(LOG4CPLUS_TEXT("RecordPath"))) {
        recordPath = properties.getProperty(LOG4CPLUS_TEXT("RecordPath"));
    }

    if (properties.exists(LOG4CPLUS_TEXT("BufferSize"))) {
        tstring tmp = properties.getProperty(LOG4CPLUS_TEXT("BufferSize"));
        sendBufferSize = atoi(LOG4CPLUS_TSTRING_TO_STRING(tmp).c_str());
    }
    if (properties.exists(LOG4CPLUS_TEXT("Compress"))) {
        tstring tmp = properties.getProperty(LOG4CPLUS_TEXT("Compress"));
        compress = tmp;
    }

    if (properties.exists(LOG4CPLUS_TEXT("MaxRetry"))) {
        tstring tmp = properties.getProperty("MaxRetry");
        int retry = atoi(tmp.c_str());
        if (retry >= 0) {
            maxRetry = retry;
        }
    }
    properties.getBool (cachedTime, LOG4CPLUS_TEXT("CachedTime"));
    properties.getBool (isInternalRelayMsg, LOG4CPLUS_TEXT("isInternalRelayMsg"));

    InitSockets(raw_host, port);
}

SocketAppender::~SocketAppender()
{
    destructorImpl();
}

//////////////////////////////////////////////////////////////////////////////
// SocketAppender public methods
//////////////////////////////////////////////////////////////////////////////

void
SocketAppender::close()
{
    thread::MutexGuard guard (access_mutex);
    //	recordMissed(false);
    if (recordFile != NULL)
    {
        fclose(recordFile);
        recordFile = NULL;
    }

    closed = true;
}

bool
SocketAppender::appendSocket(
        const log4cplus::spi::InternalLoggingEvent& event) {
    pthread_mutex_t* mtx = *(pthread_mutex_t**)(&access_mutex);

    if (cachedTime) {
        if (latestTime < event.getTimestamp()) {
            latestTime = event.getTimestamp();
        }
    } else {
        latestTime = log4cplus::helpers::Time::gettimeofday();
    }

    bool ok = false;
    int buflen = 0;
    ssize_t sent = 0;
    SocketBuffer* buffer = NULL;

    recordMissed(latestTime);
    Socket* sock = sockets->Get(mtx, latestTime);
    if (!sock) {
        char strbuf[4096];
        sprintf(strbuf, "[%d] SocketAppender::append()- "
                "no connection to be used", getpid());
        getLogLog().error(strbuf);
        goto finish;
    }
    access_mutex.unlock();

    buffer = socketBuffer.get();
    if (!buffer) {
        buffer = new SocketBuffer(kMaxMsgSize + sizeof(buflen));
        if (!buffer) {
            char strbuf[4096];
            sprintf(strbuf, "[%d] SocketAppender::append()- "
                    "create socket buffer failed", getpid());
            getLogLog().error(strbuf);
            goto finish;
        }
        socketBuffer.reset(buffer);
    }
    buffer->appendInt(0);
    if (!convertToBuffer(buffer, event, serverName, compress, isInternalRelayMsg)) {
        char strbuf[4096];
        sprintf(strbuf, "[%d] SocketAppender::append()- "
                "serialize message failed", getpid());
        getLogLog().error(strbuf);
        goto finish;
    }
    buflen = htonl(buffer->getSize() - sizeof(buflen));
    memcpy(buffer->getBuffer(), &buflen, sizeof(buflen));

retry:
    sent = ::send(sock->socket(), buffer->getBuffer(), buffer->getSize(),
            MSG_NOSIGNAL);
    if (sent < 0) {
        if (errno == EINTR) {
            goto retry;
        } else if (errno == EAGAIN) {
            std::stringstream errmsg;
            errmsg << "send to " << sock->peer()->host()
                << ":" << sock->peer()->port()
                << " with size=" << buffer->getSize()
                << " timedout";
            getLogLog().error(errmsg.str());
        } else {
            std::stringstream errmsg;
            errmsg << "send to " << sock->peer()->host()
                << ":" << sock->peer()->port()
                << " with size=" << buffer->getSize()
                << " " << strerror(errno);
            getLogLog().error(errmsg.str());
            sock->Close();
        }
    } else if (sent < (ssize_t)buffer->getSize()) {
        std::stringstream errmsg;
        errmsg << "send to " << sock->peer()->host()
            << ":" << sock->peer()->port()
            << " with size=" << buffer->getSize()
            << " sent partial " << sent << " bytes";
        getLogLog().error(errmsg.str());
        sock->Close();
    } else {
        ok = true;
    }

finish:
    if (buffer) {
        buffer->Clear();
    }
    if (sock) {
        access_mutex.lock();
        sockets->Release(sock);
    }
    return ok;
}

void
SocketAppender::append(const spi::InternalLoggingEvent& event)
{
    bool ok = appendSocket(event);
    if (!ok) {
        int retryCount = maxRetry;
        while (retryCount--) {
            ok = appendSocket(event);
            if (ok) {
                break;
            }
        }
    }
    if (ok) {
        successCount++;
    } else {
        lostCount++;
    }
}

void
SocketAppender::doAppend(const log4cplus::spi::InternalLoggingEvent& event,
        bool is_relay_msg)
{
    if (is_relay_msg  && isInternalRelayMsg) {
        helpers::getLogLog().debug(
                LOG4CPLUS_TEXT("Do not allow transmit this appender named [") 
                + name
                + LOG4CPLUS_TEXT("]."));
        return;
    }
    log4cplus::Appender::doAppend(event);
}


void 
SocketAppender::recordMissed(log4cplus::helpers::Time now)
{
    if (recordInterval <= 0) {
        return;
    }

    //Time now = Time::gettimeofday();
    if (now > recordTime) {
        tstring actualPath = now.getFormattedTime(LOG4CPLUS_TEXT(recordPath), false);

        if (actualPath != currentPath) {
            //switch missing file
            if (recordFile != NULL) {
                fclose(recordFile);
                recordFile = NULL;
            }
            currentPath = actualPath;
        }
        if (recordFile == NULL) {
            OpenMissingFile();
        }

        if (recordFile != NULL) {
            fprintf(recordFile,
                    "[%s] [PID:%d] Missed logs: %d | Success logs: %d\n",
                    LOG4CPLUS_TSTRING_TO_STRING(
                        now.getFormattedTime(LOG4CPLUS_TEXT("%Y-%m-%d %H:%M:%S")
                            , false)).c_str(),
                    getpid(), lostCount, successCount);
            fflush(recordFile);
        }

        recordTime = now + Time(recordInterval);
        lostCount = 0;
        successCount = 0;
    }
}

void SocketAppender::OpenMissingFile()
{
    recordFile = fopen(LOG4CPLUS_TSTRING_TO_STRING(currentPath).c_str(), "a+");
    if (recordFile == NULL 
            || access(LOG4CPLUS_TSTRING_TO_STRING(currentPath).c_str(), W_OK) < 0)
    {
        char strbuf[4096];
        sprintf(strbuf, "[%d] Failed to access the log file: %s"
                , getpid()
                , LOG4CPLUS_TSTRING_TO_STRING(currentPath).c_str());
        getLogLog().error(strbuf);
        return;
    }

    int fn = fileno(recordFile);

    int rc = fcntl(fn, F_SETFD, FD_CLOEXEC);
    if (rc == -1) {
        getLogLog().warn(
                LOG4CPLUS_TEXT("Failed to set FD_CLOEXEC flag for missed file.")
                );
    }
    struct stat filestat;
    if (fstat(fn, &filestat) < 0) {
        getLogLog().error(LOG4CPLUS_TEXT("Failed to get file stat."));
    } else {
        mode_t filemode = filestat.st_mode | S_IWOTH | S_IWGRP;
        fchmod(fn, filemode);
    }
}

void 
SocketAppender::InitSockets(log4cplus::tstring rawhost, int defaultPort) {
    SocketOptions options;
    options.snd_timeout = sendTimeout;
    options.snd_buffer = sendBufferSize * 1024;
    options.rcv_timeout = recvTimeout;
    options.keep_alive = true;
    options.cloexec = true;
    options.nonblock = false;
    options.connect_delay = reconnectDelay;
    options.connect_timeout = connectTimeout;
    options.poll_interval = connectCheckInterval;
    options.count = maxConnections;
    options.check = false;

    sockets.reset(new SocketPool(options));

    std::vector<tstring> hosts_ports;
    tstring host;
    remove_copy_if(rawhost.begin()
            , rawhost.end()
            , std::back_inserter (host) /*gfyan*/
            , bind1st(equal_to<tchar>(), LOG4CPLUS_TEXT(' ')));

    tokenize(host, ',', back_insert_iterator<vector<tstring> >(hosts_ports));
    for (vector<tstring>::size_type i = 0 ; i < hosts_ports.size() ; ++i) {
        tstring address = trim(hosts_ports[i]);
        tstring host;
        uint16_t port;

        if (address.find(LOG4CPLUS_TEXT("unix://")) == 0) {
            host = address;
            port = 0;
        } else {
            size_t pos = address.rfind(':');
            host = address.substr(0, pos);

            if (tstring::npos != pos || address.length() <= pos + 1) {
                tstring tmp = address.substr(pos + 1);
                port = atoi(LOG4CPLUS_TSTRING_TO_STRING(tmp).c_str());
            } else {
                port = defaultPort;
            }
        }
        sockets->AddPeer(host, port);
    }
}

bool convertToSocketBuffer(SocketBuffer *socket_buffer
    , const log4cplus::spi::InternalLoggingEvent& event
    , const log4cplus::tstring& serverName
    , const log4cplus::tstring& compress
    , bool  isInternalRelayMsg)
{
    assert(socket_buffer != NULL);

    if (toLower(compress) != LOG4CPLUS_TEXT("no")) {
        socket_buffer->appendByte(LOG4CPLUS_MESSAGE_VERSION_COMPRESS);
    }
    else {
        socket_buffer->appendByte(LOG4CPLUS_MESSAGE_VERSION);
    }

#ifndef UNICODE
    socket_buffer->appendByte(1);
#else
    socket_buffer->appendByte(2);
#endif

    socket_buffer->appendString(serverName);
    socket_buffer->appendString(event.getLoggerName());
    socket_buffer->appendInt(event.getLogLevel());
    socket_buffer->appendString(event.getNDC());
    socket_buffer->appendString(event.getThread());

    if (isInternalRelayMsg) {
        socket_buffer->appendString("1");
    } else {
        socket_buffer->appendString("0");
    }

    socket_buffer->appendInt( static_cast<unsigned int>(event.getTimestamp().sec()) );
    socket_buffer->appendInt( static_cast<unsigned int>(event.getTimestamp().usec()) );
    socket_buffer->appendString(event.getFile());
    socket_buffer->appendInt(event.getLine());

    if (toLower(compress) != LOG4CPLUS_TEXT("no")) {
        string messageString = LOG4CPLUS_TSTRING_TO_STRING(event.getMessage());
        string destString;
        long dest_length = q_compressBound(messageString.length());
        char* buf = new char[dest_length];
        compress_ret ret = q_compress(messageString.c_str(), messageString.length(), buf, &dest_length, Z_DEFLATE);
        if (ret == COMPRESS_OK) {
            destString.assign(buf, dest_length);
            socket_buffer->appendString(destString);
            delete[] buf;
            return true;
        }
        else {
            char tmp[1024];
            sprintf(tmp, "[%d] helpers::readFromsocket_buffer->) Compress failed: %d", getpid(), ret);
            //gfyan
            LogLog *loglog = log4cplus::helpers::LogLog::getLogLog();
            loglog->warn(LOG4CPLUS_TEXT(tmp));
            //LOG(tmp);

            delete[] buf;
            return false;
        }
    }

    socket_buffer->appendString(event.getMessage());

    return true;
}

bool convertToBuffer(SocketBuffer *socket_buffer
    , const log4cplus::spi::InternalLoggingEvent& event
    , const log4cplus::tstring& serverName
    , const log4cplus::tstring& compress
    , bool  isInternalRelayMsg)
{
    return convertToSocketBuffer(socket_buffer
            , event
            , serverName
            , compress
            , isInternalRelayMsg);
}

log4cplus::tstring& 
ltrim(log4cplus::tstring& ss)
{
    tstring::iterator p=find_if(ss.begin(),ss.end(),not1(ptr_fun(::isspace)));
    ss.erase(ss.begin(),p);   
    return ss;
}

log4cplus::tstring& 
rtrim(log4cplus::tstring& ss)
{
    tstring::reverse_iterator  p=find_if(ss.rbegin(),ss.rend(),not1(ptr_fun(::isspace)));   
    ss.erase(p.base(),ss.end());   
    return ss;
}

log4cplus::tstring& 
trim(log4cplus::tstring& ss)
{
    return ltrim(rtrim(ss));
}

} /*qlog*/
