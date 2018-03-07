#ifndef QLOG_QLOG_H_
#define QLOG_QLOG_H_

#include <stdarg.h>
#include <cerrno>
#include <cstring>
#include <string>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "level.h"

namespace qlog {

const size_t kMaxLogSize = 2 << 20;

/**
 * key/value�������Ͷ���
 */
typedef std::pair<std::string, std::string> KeyValue;
typedef std::vector<KeyValue> ConfigList;

/**
 * �������ļ�������־����
 *
 * @param configfile �����ļ�����
 */
void qLogConfig(const std::string& configfile);

/**
 * ��key/value���������־����
 *
 * @param mlist  key/value����
 */
void qLogConfig(ConfigList& mlist);

/**
 * ����������־
 *
 * @note ���filename����CONSOLE����־��ʾ����Ļ�ϡ�
 * @param level    ��¼����
 * @param filename ��־�ļ���ǰ׺("CONSOLE","","/home/s/log/","../myapp_")
 */
void qLogQuickConfig(Level level=QLOG_INFO, const std::string& filename="CONSOLE");

/**
 * ��ʾ��ǰȫ�������־��Ϣ
 */
void qLogViewConfig();

/**
 * ���������־����
 */
void qLogCleanConfig();

/**
 * �ڶ������������ڼ��������ļ�����̬����
 */
class qLogMonitor {
public:
    qLogMonitor(const std::string& filename, unsigned int millis = 60 * 1000);
    ~qLogMonitor();

private:
    // Disable copy
    qLogMonitor(const qLogMonitor&);
    qLogMonitor& operator=(qLogMonitor&);
    void* handle;
};

/**
 * �ж���־�Ƿ��¼
 *
 * @param name     ��־���
 * @param level    ��־����
 *
 * @return true: ��¼��־
 */
bool isLog(const std::string& name, int level);

void qLogInternalDebug(bool isDebug);

void qLogAllv(const std::string& name
        , int level
        , int eNo
        , bool bThrow
        , const char* file
        , int line
        , const char* fmt, va_list ap);

//����֧�ֱ�θ�ʽ�����
void qLogAll(const std::string& name
        , int level
        , int eNo
        , bool bThrow
        , const char* file
        , int line
        , const char* fmt,...)
    __attribute__ ((format (printf, 7, 8)));

//����֧����ʽ���
void qLogAllStream(const std::string& name
        , int level
        , const std::string& message
        , const char* file
        , int line);

/**
 * qlog��stream�ӿ�
 */
class logstream : public std::ostringstream {
public:
    logstream(const std::string& name
            , int level
            , int eNo
            , const char* file
            , int line)
        : m_name(name)
          , m_level(level)
          , m_eNo(eNo)
          , m_file(file)
          , m_line(line) {}

    logstream& l_value() { return *this; }

    ~logstream()
    {
        if ( isLog(m_name, m_level) ) {
            logstream& _s = *this;
            if ( m_eNo != 0 ) {
                _s << " -- (" << m_eNo << ')' << strerror(m_eNo);
            }

            qLogAllStream(m_name, m_level, _s.str(), m_file, m_line);
        }
    }

private:
    std::string m_name;
    int m_level;
    int m_eNo;
    const char* m_file;
    int m_line;
    logstream( const logstream& );
    logstream& operator=( const logstream& );
};

} // namespace qlog

/**
 * ��μ�¼level������Ϣ����־������ name �У�fmt��ʽ�� printf ��ͬ.
 */
#define qLog(name, level, fmt, ...)	\
    qlog::qLogAll(name, level, 0, false, __FILE__, __LINE__, fmt, ## __VA_ARGS__ )

/**
 * ��μ�¼������Ϣ����־��� name �У�fmt��ʽ�� printf ��ͬ.
 */
#define qLogTrace(name, fmt, ...)	\
    qlog::qLogAll(name, qlog::QLOG_TRACE, 0, false, __FILE__, __LINE__, fmt, ## __VA_ARGS__ )

/**
 * ��μ�¼������Ϣ����־��� name �У�fmt��ʽ�� printf ��ͬ.
 */
#define qLogDebug(name, fmt, ...)	\
    qlog::qLogAll(name, qlog::QLOG_DEBUG, 0, false, __FILE__, __LINE__, fmt, ## __VA_ARGS__ )

/**
 * ��μ�¼��ʾ��Ϣ����־��� name �У�fmt��ʽ�� printf ��ͬ.
 */
#define qLogInfo(name, fmt, ...)	\
    qlog::qLogAll(name, qlog::QLOG_INFO,  0, false, __FILE__, __LINE__, fmt, ## __VA_ARGS__ )


#define qLogWarn(name, fmt, ...)	\
    qlog::qLogAll(name, qlog::QLOG_WARN, 0, false, __FILE__, __LINE__, fmt, ## __VA_ARGS__ )

/**
 * ��μ�¼Ӧ�ô�����Ϣ����־��� name �У�fmt��ʽ�� printf ��ͬ.
 */
#define qAppError(name, fmt, ...)	\
    qlog::qLogAll(name, qlog::QLOG_ERROR, 0, false, __FILE__, __LINE__, fmt, ## __VA_ARGS__ )

/**
 * ��μ�¼ϵͳ������Ϣ����־��� name �У�fmt��ʽ�� printf ��ͬ.(����ϵͳ������Ϣ)
 */
#define qSysError(name, fmt, ...)	\
    qlog::qLogAll(name, qlog::QLOG_ERROR, errno, false, __FILE__, __LINE__, fmt, ## __VA_ARGS__ )

/**
 * ��μ�¼Ӧ�ô�����Ϣ����־��� name �в��׳��쳣���� 0��fmt��ʽ�� printf ��ͬ.
 */
#define qAppThrow(name, fmt, ...)	\
    qlog::qLogAll(name, qlog::QLOG_ERROR, 0, true, __FILE__, __LINE__, fmt, ## __VA_ARGS__ )

/**
 * ��μ�¼ϵͳ������Ϣ����־��� name �в��׳��쳣���� errno��fmt��ʽ�� printf ��ͬ.
 */
#define qSysThrow(name, fmt, ...)	\
    qlog::qLogAll(name, qlog::QLOG_ERROR, errno, true, __FILE__, __LINE__, fmt, ## __VA_ARGS__ )

/**
 * ��μ�¼������Ϣ����־��� name �У�fmt��ʽ�� printf ��ͬ.(����eNo��Ӧ��ϵͳ������Ϣ)
 */
#define qLogError(name, eNo, fmt, ...)	\
    qlog::qLogAll(name, qlog::QLOG_ERROR, eNo, false, __FILE__, __LINE__, fmt, ## __VA_ARGS__ )

/**
 * ��μ�¼������Ϣ����־��� name �в��׳��쳣���� eNo��fmt��ʽ�� printf ��ͬ.
 */
#define qLogThrow(name, eNo, fmt, ...)	\
    qlog::qLogAll(name, qlog::QLOG_ERROR, eNo, true, __FILE__, __LINE__, fmt, ## __VA_ARGS__ )


/**
 * ��ʽ��¼level������Ϣ����־��� name ��.
 */
#define qLogs(name,level)	 \
    qlog::logstream(name,level,0,__FILE__,__LINE__).l_value()

/**
 * ��ʽ��¼������Ϣ����־��� name ��.
 */
#define qLogTraces(name)	 \
    qlog::logstream(name,qlog::QLOG_TRACE,0,__FILE__,__LINE__).l_value()

/**
 * ��ʽ��¼������Ϣ����־��� name ��.
 */
#define qLogDebugs(name)	 \
    qlog::logstream(name,qlog::QLOG_DEBUG,0,__FILE__,__LINE__).l_value()

/**
 * ��ʽ��¼��ʾ��Ϣ����־��� name ��.
 */
#define qLogInfos(name)		 \
    qlog::logstream(name,qlog::QLOG_INFO, 0,__FILE__,__LINE__).l_value()

/**
 * ��ʽ��¼��ʾ��Ϣ����־��� name ��.
 */
#define qLogWarns(name)		 \
    qlog::logstream(name,qlog::QLOG_WARN, 0,__FILE__,__LINE__).l_value()

/**
 * ��ʽ��¼Ӧ�ô�����Ϣ����־��� name ��.
 */
#define qAppErrors(name) 	 \
    qlog::logstream(name,qlog::QLOG_ERROR,0,__FILE__,__LINE__).l_value()

/**
 * ��ʽ��¼ϵͳ������Ϣ����־��� name ��.(����ϵͳ������Ϣ)
 */
#define qSysErrors(name) 	 \
    qlog::logstream(name,qlog::QLOG_ERROR,errno,__FILE__,__LINE__).l_value()

/**
 * ��ʽ��¼������Ϣ����־��� name ��.(����eNo��Ӧ��ϵͳ������Ϣ)
 */
#define qLogErrors(name,eNo) \
    qlog::logstream(name,qlog::QLOG_ERROR,eNo,__FILE__,__LINE__).l_value()

/**
 * ��¼Ӧ�ô�����Ϣ����־��� name �в��׳��쳣���� 0��logEventΪ�����.
 */
#define qAppThrows(name, logEvent) \
    qLogAndThrows(name, qlog::QLOG_ERROR, 0, logEvent)

/**
 * ��¼ϵͳ������Ϣ����־��� name �в��׳��쳣���� errno��logEventΪ�����.
 */
#define qSysThrows(name, logEvent) \
    qLogAndThrows(name, qlog::QLOG_ERROR, errno, logEvent)

/**
 * ��¼������Ϣ����־��� name �в��׳��쳣���� eNo��logEventΪ�����.
 */
#define qLogThrows(name, eNo, logEvent) \
    qLogAndThrows(name, qlog::QLOG_ERROR, eNo, logEvent)



#define qLogAndThrows(name, level, eNo, logStream) \
    do \
    { \
        int m_eNo = eNo; \
        std::ostringstream _qbuf; \
        _qbuf << logStream; \
        if ( m_eNo != 0 ) \
            _qbuf << " -- (" << m_eNo << ')' << strerror(m_eNo); \
        if ( qlog::isLog(name, level) ) \
            qlog::qLogAllStream(name, level, _qbuf.str(), __FILE__, __LINE__); \
        throw std::runtime_error(_qbuf.str()); \
    } while(0)



#endif  // QLOG_QLOG_H_

