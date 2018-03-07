#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <vector>
#include <memory>
#include <log4cplus/logger.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/fstreams.h>
#include <log4cplus/socketappender.h>
#include <log4cplus/hierarchy.h>
#include <log4cplus/ndc.h>
#include <boost/thread/tss.hpp>
#include "configurator.h"
#include "qlog.h"

using namespace std;
using namespace log4cplus;
using namespace log4cplus::spi;
using namespace log4cplus::helpers;

namespace qlog {

//////////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////////
void initializeQlog();

/////////////////////////////////////////////////////
// private
/////////////////////////////////////////////////////

/**
 * auto init, has appender config
 */
static bool isAppenderInit = false;

/**
 * levelת�����ַ���
 *
 * @param ll
 *
 * @return
 */
static const tstring LevelString(LogLevel ll)
{
    return getLogLevelManager().toString(ll);
}

/**
 * ��ʾ��־���Ŀ����Ϣ
 *
 * @param appender
 */
static void viewAppender(SharedAppenderPtr& appender)
{
    std::ostringstream _qbuf;
    _qbuf << LOG4CPLUS_STRING_TO_TSTRING("\t�����:")
        << appender->getName();
    _qbuf << LOG4CPLUS_STRING_TO_TSTRING(", �����ֵ:")
        << LevelString(appender->getThreshold());

    FilterPtr currentFilter = appender->getFilter();
    int i = 0;
    while(currentFilter.get()) {
        i++;
        currentFilter = currentFilter->next;
    }
    if ( i > 0 ) _qbuf << LOG4CPLUS_STRING_TO_TSTRING(", ��") << i
        << LOG4CPLUS_STRING_TO_TSTRING("��������");

}

/**
 * ��ʾĳ�����־��ȫ�����Ŀ����Ϣ
 *
 * @param logger
 */
static void ListAppenders(log4cplus::Logger& logger)
{
    SharedAppenderPtrList mlist = logger.getAllAppenders();
    for ( SharedAppenderPtrList::iterator it=mlist.begin();
            it!=mlist.end(); ++it )
        viewAppender(*it);
}

/**
 * ��ʾȱʡ��־��Ϣ
 */
static void viewRoot()
{
    log4cplus::Logger root = log4cplus::Logger::getRoot();
    ListAppenders(root);
}

/**
 * ��ʾĳ�����־��Ϣ
 *
 * @param logger
 */
static void viewLogger(log4cplus::Logger& logger)
{
    std::ostringstream _qbuf;
    _qbuf << LOG4CPLUS_STRING_TO_TSTRING("�����:") << logger.getName();
    _qbuf << LOG4CPLUS_STRING_TO_TSTRING(", �������:") 
        << LevelString(logger.getChainedLogLevel());
    if ( logger.getLogLevel() == NOT_SET_LOG_LEVEL )
        _qbuf << LOG4CPLUS_STRING_TO_TSTRING("(�̳�)");
    _qbuf << LOG4CPLUS_STRING_TO_TSTRING(", ����:") 
        << logger.getParent().getName();
    if ( logger.getAdditivity() )
        _qbuf << LOG4CPLUS_STRING_TO_TSTRING(", �̳��������");
    else
        _qbuf << LOG4CPLUS_STRING_TO_TSTRING(", �Զ����������");
    //gfyan
    //LogLog::getLogLog()->info(_qbuf.str());

    ListAppenders(logger);
}

/**
 * ���ԭ���ã��������ò���������־
 *
 * @param props  ��������
 *
 * @return -false:�Ƿ���ʽ
 *         -true: �������óɹ���������isAppenderInit
 */
static bool reConfig(const Properties& props)
{
    //������appender
    Properties checks = props.getPropertySubset("qlog.appender.");
    if (checks.size() == 0) {
        LogLog::getLogLog()->error("Invalid qlog config format");
        return false;
    }

    log4cplus::Logger::getRoot().getDefaultHierarchy().resetConfiguration();
    qlog::PropertyConfigurator tmp(props);
    tmp.configure();
    isAppenderInit = true;

    return true;
}

/**
 * ����������־��¼���ļ�
 *
 * @param filename ��־�ļ���ǰ׺("","./","../myapp-")
 * @param level ��¼����
 */
static void defaultFile(const std::string& filename, Level level) {
    std::string files = filename;
    std::string ll = LevelString(level);

    ConfigList defaultSet;
    if (level < QLOG_DEBUG) {
        ll += ", ROOT_TRACE";
        //trace.log save below DEBUG range message
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_TRACE", "RollingFileAppender"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_TRACE.File", files+"trace.log"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_TRACE.MaxFileSize", "1000MB"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_TRACE.MaxBackupIndex", "5"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_TRACE.layout", "PatternLayout"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_TRACE.layout.ConversionPattern",
                    "%D [%c] %m%n"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_TRACE.filters.1",
                    "LogLevelRangeFilter"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_TRACE.filters.1.LogLevelMin", "ALL"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_TRACE.filters.1.LogLevelMax", "DEBUG"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_TRACE.filters.1.AcceptOnMatch",
                    "true"));
    }
    if (level < QLOG_INFO) {
        ll += ", ROOT_DEBUG";
        //debug.log save DEBUG to INFO-1 range message
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_DEBUG", "RollingFileAppender"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_DEBUG.File", files+"debug.log"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_DEBUG.MaxFileSize", "1000MB"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_DEBUG.MaxBackupIndex", "5"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_DEBUG.layout", "PatternLayout"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_DEBUG.layout.ConversionPattern",
                    "%D [%c] %m%n"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_DEBUG.filters.1",
                    "LogLevelRangeFilter"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_DEBUG.filters.1.LogLevelMin", "DEBUG"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_DEBUG.filters.1.LogLevelMax", "INFO"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_DEBUG.filters.1.AcceptOnMatch",
                    "true"));
    }
    if (level < QLOG_WARN) {
        ll += ", ROOT_INFO";
        //info.log save INFO to WARN-1 range message
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_INFO", "RollingFileAppender"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_INFO.File", files+"info.log"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_INFO.MaxFileSize", "100MB"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_INFO.MaxBackupIndex", "10"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_INFO.layout", "PatternLayout"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_INFO.layout.ConversionPattern",
                    "%D [%c] %m%n"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_INFO.filters.1",
                    "LogLevelRangeFilter"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_INFO.filters.1.LogLevelMin", "INFO"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_INFO.filters.1.LogLevelMax", "WARN"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_INFO.filters.1.AcceptOnMatch", "true"));
    }
    if (level < QLOG_OFF) {
        ll += ", ROOT_ERROR";
        //error.log save WARN to OFF-1 range message
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_ERROR", "RollingFileAppender"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_ERROR.File", files+"error.log"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_ERROR.MaxFileSize", "100MB"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_ERROR.MaxBackupIndex", "10"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_ERROR.Threshold", "WARN"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_ERROR.layout", "PatternLayout"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_ERROR.layout.ConversionPattern",
                    "%D [%c] [PID=%P] [%t] [%l] %m%n"));
    }
    if (level >= QLOG_OFF) {
        ll += ", ROOT_NULL";
        defaultSet.push_back( KeyValue("qlog.appender.ROOT_NULL",
                    "NullAppender"));
    }

    defaultSet.push_back(KeyValue("qlog.rootLogger", ll));
    qLogConfig(defaultSet);
}

/**
 * ����������־��¼����Ļ
 *
 * @param level ��¼����
 */
static void defaultConsole(Level level) {
    std::string ll = LevelString(level);

    ConfigList defaultSet;
    if (level < QLOG_WARN) {
        ll += ", ROOT_INFO";
        //ROOT_INFO match below WARN range message
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_INFO",  "ConsoleAppender"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_INFO.ImmediateFlush",  "true"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_INFO.layout", "PatternLayout"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_INFO.layout.ConversionPattern",
                    "%D [%p] [%c] %m%n"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_INFO.filters.1",
                    "LogLevelRangeFilter"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_INFO.filters.1.LogLevelMin", "ALL"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_INFO.filters.1.LogLevelMax", "WARN"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_INFO.filters.1.AcceptOnMatch", "true"));
    }
    if (level < QLOG_OFF) {
        ll += ", ROOT_ERROR";
        //ROOT_ERROR match WARN to OFF-1 range message
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_ERROR", "ConsoleAppender"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_ERROR.ImmediateFlush",  "true"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_ERROR.logToStdErr", "true"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_ERROR.layout", "PatternLayout"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_ERROR.layout.ConversionPattern",
                    "%D [%p] [%c] [%l] %m%n"));
        defaultSet.push_back(KeyValue(
                    "qlog.appender.ROOT_ERROR.Threshold", "WARN"));
    }
    if (level >= QLOG_OFF)
    {
        ll += ", ROOT_NULL";
        defaultSet.push_back(KeyValue("qlog.appender.ROOT_NULL", "NullAppender"
                    ));
    }

    defaultSet.push_back(KeyValue("qlog.rootLogger", ll));
    qLogConfig(defaultSet);
}

/**
 * ��ʼ��log
 */
static void init(void)
{
    if ( isAppenderInit == true )
        return;

    //LogLog::getLogLog()->info("init, log to console");
    qlog::initializeQlog();
    defaultConsole(QLOG_INFO);
}

/////////////////////////////////////////////////////
// public
/////////////////////////////////////////////////////

/**
 * ��ʾ��ǰȫ�������־��Ϣ
 */
void qLogViewConfig()
{
    viewRoot();

    LoggerList mlist = log4cplus::Logger::getRoot().getCurrentLoggers();
    for ( LoggerList::iterator it=mlist.begin(); it!=mlist.end(); ++it )
        viewLogger(*it);

    std::cout << endl;
}

/**
 * ���������־����
 */
void qLogCleanConfig()
{
    //LogLog::getLogLog()->info("log to null");
    log4cplus::Logger::getRoot().getDefaultHierarchy().resetConfiguration();
    isAppenderInit = false;
}

/**
 * �������ļ�������־����
 *
 * @param configfile �����ļ�����
 */
void qLogConfig(const std::string& configfile)
{
    if (configfile.length() == 0) {
        LogLog::getLogLog()->error("qLogConfig() no configfile");
        return;
    }

    //����ļ��Ƿ����, ת��Ϊistream
    tifstream file;
    file.open(LOG4CPLUS_TSTRING_TO_STRING(configfile).c_str());
    if (!file) {
        LogLog::getLogLog()->error(
                "qLogConfig() Unable to open file: " + configfile);
        return;
    }

    //LogLog::getLogLog()->info( "log from config file: "+ configfile );
    Properties props(file);
    reConfig(props);
}

/**
 * ��key/value���������־����
 *
 * @param mlist  key/value����
 */
void qLogConfig(ConfigList& mlist)
{
    Properties props;
    for ( ConfigList::iterator it=mlist.begin(); it!=mlist.end(); ++it )
        props.setProperty(it->first, it->second);

    //LogLog::getLogLog()->info("qLogConfig(ConfigList)");
    reConfig(props);
}


/**
 * ����������־
 *
 * @note ���filename����CONSOLE����־��ʾ����Ļ�ϡ�
 * @param level    ��¼����
 * @param filename ��־�ļ���ǰ׺("CONSOLE","","/home/s/log/","../myapp_")
 */
void qLogQuickConfig(Level level, const std::string& filename )
{
    //if (log4cplus::helpers::toUpper(filename) == "CONSOLE")
    //LogLog::getLogLog()->info("log to file: " + filename );
    if (filename == "CONSOLE")
    {
        defaultConsole(level);
        return;
    }

    defaultFile(filename,level);
}

//���ʹ�õ��߳̿⣬�����������ļ�
static bool isConfigFile(const std::string& configfile)
{
    if(configfile.length() == 0) {
        return false;
    }
    //����ļ��Ƿ����, ת��Ϊistream
    tifstream file;
    file.open(LOG4CPLUS_TSTRING_TO_STRING(configfile).c_str());
    if (!file) {
        return false;
    }

    //������appender
    Properties props(file);
    Properties checks = props.getPropertySubset("qlog.appender.");
    if (checks.size() == 0) {
        return false;
    }

    return true;
}

qLogMonitor::qLogMonitor(const std::string& filename, unsigned int millis) 
    : handle(NULL)
{
    if ( isConfigFile(filename) )
        log4cplus::Logger::getRoot().getDefaultHierarchy().resetConfiguration();
    else
        qLogQuickConfig();

    qlog::ConfigureAndWatchThread* configureThread =
        new qlog::ConfigureAndWatchThread(filename, millis);
    handle = (void*)configureThread;
    isAppenderInit = true;
}

qLogMonitor::~qLogMonitor()
{
    delete (qlog::ConfigureAndWatchThread*)handle;
}

/**
 * ��/�ر����������Ϣ
 *
 * @param isDebug
 */
void qLogInternalDebug(bool isDebug)
{
    if (isDebug)
        LogLog::getLogLog()->setInternalDebugging(true);
    else
        LogLog::getLogLog()->setQuietMode(true);
}

//NDC,�ݲ�����
void qLogPush(const std::string& message)
{
    //NDCContextCreator _context("loop");
    getNDC().push(message);
}

void qLogPop()
{
    getNDC().pop();
}

void qLogRemove()
{
    getNDC().remove();
}

namespace {

struct LogBuffer {
public:
    LogBuffer() : buffer_((char*)malloc(kMaxLogSize)) {}
    ~LogBuffer() { free(buffer_); }

    char* buffer() const { return buffer_; }

    static LogBuffer& instance() {
        LogBuffer* lb = instance_.get();
        if (!lb) {
            lb = new LogBuffer();
            instance_.reset(lb);
        }
        return *lb;
    }
private:
    char* buffer_;

    static boost::thread_specific_ptr<LogBuffer> instance_;
};

boost::thread_specific_ptr<LogBuffer> LogBuffer::instance_;

}

//����֧�ֱ�θ�ʽ�����
void qLogAllv(const std::string& name,
        int level,
        int eNo,
        bool bThrow,
        const char* file,
        int line,
        const char* fmt,
        va_list ap) {
    if ( fmt == NULL )
        return;

    init();

    log4cplus::Logger logger = (name.empty())?
        log4cplus::Logger::getRoot() : log4cplus::Logger::getInstance(name);

    bool bLog = logger.isEnabledFor(level);
    if ( bLog || bThrow ) {
        char* logbuf = LogBuffer::instance().buffer();
        assert(logbuf);

        // �����Ϣд��LogBuf
        int cnt = ::vsnprintf(logbuf, kMaxLogSize, fmt, ap);

        if ( cnt >= (int)kMaxLogSize )
            cnt = kMaxLogSize - 1;

        // ��β������Ϣд��LogBuf
        if ( eNo != 0 ) {
            if ( cnt == 0 || *(logbuf + cnt - 1) != '\n' ) {
                cnt += ::snprintf( logbuf + cnt, kMaxLogSize - cnt
                        , " -- (%d)%s", eNo, strerror(eNo) );
            } else {
                --cnt;
                cnt += ::snprintf( logbuf + cnt, kMaxLogSize - cnt
                        , " -- (%d)%s\n", eNo, strerror(eNo) );
            }

            if ( cnt >= (int)kMaxLogSize ) {
                cnt = kMaxLogSize - 1;
            }
        }

        if ( bLog )
            logger.forcedLog(level, std::string(logbuf, cnt), file, line);

        if ( bThrow )
            throw std::runtime_error( std::string(logbuf, cnt) );
    }
}

void qLogAll(const std::string& name,
        int level,
        int eNo,
        bool bThrow,
        const char* file,
        int line,
        const char* fmt,
        ...) {
    va_list ap;
    va_start(ap,fmt);
    qLogAllv(name, level, eNo, bThrow, file, line, fmt, ap);
    va_end(ap);
}

//����֧����ʽ���
void qLogAllStream(const std::string& name
        , int level, const std::string& message, const char* file, int line)
{
    log4cplus::Logger logger = (name.empty())?
        log4cplus::Logger::getRoot() : log4cplus::Logger::getInstance(name);
    logger.forcedLog(level, message, file, line);
}

//����֧����ʽ���
bool isLog(const std::string& name, int level)
{
    init();
    log4cplus::Logger logger = (name.empty())?
        log4cplus::Logger::getRoot() : log4cplus::Logger::getInstance(name);
    return logger.isEnabledFor(level);
}

} // namespace qlog

extern "C" void cLogAll(const char* cname, int level, int eNo, const char* file,
        int line, const char* fmt,...) {
	if ( fmt == NULL )
		return;

	std::string name;
	if ( cname )
		name = std::string(cname);

	qlog::init();

	log4cplus::Logger logger = (name.empty())?
        log4cplus::Logger::getRoot() : log4cplus::Logger::getInstance(name);

	bool bLog = logger.isEnabledFor(level);
	if ( bLog )
	{
        char* logbuf = qlog::LogBuffer::instance().buffer();

		// �����Ϣд��LogBuf
		va_list ap;
		va_start(ap,fmt);
		int cnt = ::vsnprintf(logbuf, qlog::kMaxLogSize, fmt, ap );
		va_end(ap);

		if ( cnt >= (int)qlog::kMaxLogSize )
			cnt = qlog::kMaxLogSize - 1;

		// ��β������Ϣд��LogBuf
		if ( eNo != 0 )
		{
			if ( cnt == 0 || *(logbuf + cnt - 1) != '\n' )
				cnt += ::snprintf(logbuf + cnt, qlog::kMaxLogSize - cnt,
                        " -- (%d)%s", eNo, strerror(eNo) );
			else
			{
				--cnt;
				cnt += snprintf(logbuf + cnt, qlog::kMaxLogSize - cnt,
                        " -- (%d)%s\n", eNo, strerror(eNo) );
			}
			if ( cnt >= (int)qlog::kMaxLogSize )
				cnt = qlog::kMaxLogSize - 1;
		}

		logger.forcedLog(level, std::string(logbuf, cnt), file, line);
	}
}

extern "C" void cLogCleanConfig() {
	qlog::qLogCleanConfig();
}

extern "C" void cLogConfig(const char* configfile) {
	string conf;
	if ( configfile )
		conf = string(configfile);
	qlog::qLogConfig(conf);
}

