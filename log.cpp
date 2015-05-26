
#include "log.h"

INITIALIZE_EASYLOGGINGPP

void logfile_roll(const char* filename, size_t s)
{
	// TODO
}

void initLogging(const std::string& filename, bool debug)
{
	el::Configurations defaultConf;
	defaultConf.setToDefault();
	defaultConf.setGlobally(el::ConfigurationType::Enabled, "true");
	defaultConf.setGlobally(el::ConfigurationType::ToFile, "true");
	defaultConf.setGlobally(el::ConfigurationType::ToStandardOutput, "true");
	defaultConf.setGlobally(el::ConfigurationType::MaxLogFileSize, "20000000");
	defaultConf.setGlobally(el::ConfigurationType::Filename, filename);
	if(!debug)
	{
		defaultConf.setGlobally(el::ConfigurationType::Format,
			"%datetime <%levshort> %msg");
		defaultConf.set(el::Level::Trace, el::ConfigurationType::Enabled, "false");
		defaultConf.set(el::Level::Debug, el::ConfigurationType::Enabled, "false");
	}
	else
	{
		defaultConf.setGlobally(el::ConfigurationType::Format,
			"[%thread] %datetime [%func] <%levshort> %msg");
	}

	el::Loggers::reconfigureAllLoggers(defaultConf);
	el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);
	el::Loggers::addFlag(el::LoggingFlag::StrictLogFileSizeCheck);
	el::Helpers::installPreRollOutCallback(logfile_roll);
}
