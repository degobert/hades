// configurator.cc (2014-01-09)

#include "configurator.h"
#include <sys/stat.h>

#include <log4cplus/helpers/loglog.h>
#include <log4cplus/hierarchylocker.h>
#include <log4cplus/hierarchy.h>
#include <log4cplus/spi/factory.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/nullappender.h>
#include <log4cplus/syslogappender.h>
#include <log4cplus/asyncappender.h>
#include <log4cplus/log4judpappender.h>
#include <log4cplus/helpers/fileinfo.h>

#include "fileappender.h"
#include "dynamicappender.h"
#include "socketappender.h"
#include "patternlayout.h"

using namespace log4cplus;

namespace qlog {

void initializeQlog();

PropertyConfigurator::PropertyConfigurator(const tstring& propertyFile,
    Hierarchy& hier, unsigned f)
    : log4cplus::PropertyConfigurator(propertyFile, hier, f)
    , origin_properties(propertyFile)
{
    init();
}


PropertyConfigurator::PropertyConfigurator(const helpers::Properties& props,
    Hierarchy& hier, unsigned f)
    : log4cplus::PropertyConfigurator(props, hier, f)
    , origin_properties( props )
{
    init();
}


PropertyConfigurator::PropertyConfigurator(tistream& propertyStream,
    Hierarchy& hier, unsigned f)
    : log4cplus::PropertyConfigurator(propertyStream, hier, f)
    , origin_properties(propertyStream)
{
    init();
}

void
PropertyConfigurator::init() {
    properties = origin_properties.getPropertySubset( LOG4CPLUS_TEXT("qlog.") );
}

void
PropertyConfigurator::configure()
{
    // Configure log4cplus internals.
    bool internal_debugging = false;
    if (properties.getBool (internal_debugging, LOG4CPLUS_TEXT ("configDebug")))
        helpers::getLogLog ().setInternalDebugging (internal_debugging);

    bool quiet_mode = false;
    if (properties.getBool (quiet_mode, LOG4CPLUS_TEXT ("quietMode")))
        helpers::getLogLog ().setQuietMode (quiet_mode);

    bool disable_override = false;
    if (properties.getBool (disable_override,
            LOG4CPLUS_TEXT ("disableOverride"))) {
    }

    initializeQlog();

    configureAppenders();
    configureLoggers();
    configureAdditivity();

    if (disable_override)
        h.disable (Hierarchy::DISABLE_OVERRIDE);

    // Erase the appenders so that we are not artificially keeping them "alive".
    appenders.clear ();
}

void
PropertyConfigurator::reconfigure() {
    properties = helpers::Properties(propertyFilename);
    origin_properties = properties;
    log4cplus::PropertyConfigurator::init();
    init();
    configure();
}

//////////////////////////////////////////////////////////////////////////////
// ConfigurationWatchDogThread implementation
//////////////////////////////////////////////////////////////////////////////

class ConfigurationWatchDogThread 
    : public log4cplus::thread::AbstractThread,
      public qlog::PropertyConfigurator
{
public:
    ConfigurationWatchDogThread(const tstring& file, unsigned int millis)
        : PropertyConfigurator(file)
        , waitMillis(millis < 1000 ? 1000 : millis)
        , shouldTerminate(false)
        , lock(NULL)
    {
        lastFileInfo.mtime = helpers::Time::gettimeofday ();
        lastFileInfo.size = 0;
        lastFileInfo.is_link = false;

        updateLastModInfo();
    }

    virtual ~ConfigurationWatchDogThread ()
    { }
    
    void terminate ()
    {
        shouldTerminate.signal ();
        join ();
    }

protected:
    virtual void run();
    virtual Logger getLogger(const tstring& name);
    virtual void addAppender(Logger &logger, SharedAppenderPtr& appender);
    
    bool checkForFileModification();
    void updateLastModInfo();
    
private:
    ConfigurationWatchDogThread (ConfigurationWatchDogThread const &);
    ConfigurationWatchDogThread & operator = (
        ConfigurationWatchDogThread const &);

    unsigned int const waitMillis;
    thread::ManualResetEvent shouldTerminate;
    helpers::FileInfo lastFileInfo;
    HierarchyLocker* lock;
};


void
ConfigurationWatchDogThread::run()
{
    while (! shouldTerminate.timed_wait (waitMillis))
    {
        bool modified = checkForFileModification();
        if(modified) {
            // Lock the Hierarchy
            HierarchyLocker theLock(h);
            lock = &theLock;

            // reconfigure the Hierarchy
            theLock.resetConfiguration();
            reconfigure();
            updateLastModInfo();

            // release the lock
            lock = NULL;
        }
    }
}


Logger
ConfigurationWatchDogThread::getLogger(const tstring& name)
{
    if(lock)
        return lock->getInstance(name);
    else
        return PropertyConfigurator::getLogger(name);
}


void
ConfigurationWatchDogThread::addAppender(Logger& logger,
                                         SharedAppenderPtr& appender)
{
    if(lock)
        lock->addAppender(logger, appender);
    else
        PropertyConfigurator::addAppender(logger, appender);
}


bool
ConfigurationWatchDogThread::checkForFileModification()
{
    helpers::FileInfo fi;

    if (helpers::getFileInfo (&fi, propertyFilename) != 0)
        return false;

    bool modified = fi.mtime > lastFileInfo.mtime
        || fi.size != lastFileInfo.size;

#if defined(LOG4CPLUS_HAVE_LSTAT)
    if (!modified && fi.is_link)
    {
        struct stat fileStatus;
        if (lstat(LOG4CPLUS_TSTRING_TO_STRING(propertyFilename).c_str(),
                &fileStatus) == -1)
            return false;

        helpers::Time linkModTime(fileStatus.st_mtime);
        modified = (linkModTime > fi.mtime);
    }
#endif

    return modified;
}



void
ConfigurationWatchDogThread::updateLastModInfo()
{
    helpers::FileInfo fi;

    if (helpers::getFileInfo (&fi, propertyFilename) == 0)
        lastFileInfo = fi;
}



ConfigureAndWatchThread::ConfigureAndWatchThread(const tstring& file,
    unsigned int millis)
    : watchDogThread(0)
{
    watchDogThread = new ConfigurationWatchDogThread(file, millis);
    watchDogThread->addReference ();
    watchDogThread->configure();
    watchDogThread->start();
}


ConfigureAndWatchThread::~ConfigureAndWatchThread()
{
    if (watchDogThread)
    {
        watchDogThread->terminate();
        watchDogThread->removeReference ();
    }
}

} /*qlog*/
