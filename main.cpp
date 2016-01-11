
#include "log.h"
#include "model/quotes.h"
#include "miner.h"
#include "miners/ttminer.h"
#include "miners/minmaxminer.h"
#include "optionparser/optionparser.h"
#include <boost/lexical_cast.hpp>
#include "report/textreportbuilder.h"
#include "report/htmlreportbuilder.h"
#include "miners/iminer.h"
#include "json/value.h"
#include "json/reader.h"

using namespace boost;

struct Arg: public option::Arg
{
	static void printError(const char* msg1, const option::Option& opt, const char* msg2)
	{
		fprintf(stderr, "%s", msg1);
		fwrite(opt.name, opt.namelen, 1, stderr);
		fprintf(stderr, "%s", msg2);
	}

	static option::ArgStatus Unknown(const option::Option& option, bool msg)
	{
		if (msg) printError("Unknown option '", option, "'\n");
		return option::ARG_ILLEGAL;
	}

	static option::ArgStatus Required(const option::Option& option, bool msg)
	{
		if (option.arg != 0)
			return option::ARG_OK;

		if (msg) printError("Option '", option, "' requires an argument\n");
		return option::ARG_ILLEGAL;
	}

	static option::ArgStatus NonEmpty(const option::Option& option, bool msg)
	{
		if (option.arg != 0 && option.arg[0] != 0)
			return option::ARG_OK;

		if (msg) printError("Option '", option, "' requires a non-empty argument\n");
		return option::ARG_ILLEGAL;
	}

	static option::ArgStatus Numeric(const option::Option& option, bool msg)
	{
		char* endptr = 0;
		if (option.arg != 0 && strtol(option.arg, &endptr, 10)){};
		if (endptr != option.arg && *endptr == 0)
			return option::ARG_OK;

		if (msg) printError("Option '", option, "' requires a numeric argument\n");
		return option::ARG_ILLEGAL;
	}

	static option::ArgStatus PatternLength(const option::Option& option, bool msg)
	{
		char* endptr = 0;
		long i = strtol(option.arg, &endptr, 10);
		if (option.arg != 0 && i){};
		if (endptr != option.arg && *endptr == 0 && (i >= 2) && (i <= 31))
			return option::ARG_OK;

		if (msg) printError("Option '", option, "' requires a numeric argument between 2 and 31\n");
		return option::ARG_ILLEGAL;
	}

	static option::ArgStatus Double(const option::Option& option, bool msg)
	{
		char* endptr = 0;
		double i = strtod(option.arg, &endptr);
		if (option.arg != 0 && i){};
		if (endptr != option.arg && *endptr == 0)
			return option::ARG_OK;

		if (msg) printError("Option '", option, "' requires a numeric argument\n");
		return option::ARG_ILLEGAL;
	}
};

enum MinerType { minerCandle, minerTime, minerZigzag };

enum  optionIndex { UNKNOWN, HELP, INPUT_FILENAME,
	DEBUG_MODE,
	MINER_TYPE,
	OUTPUT_FILENAME,
	REPORT_TYPE,
	CONFIG_FILE
};
const option::Descriptor usage[] = {
{ UNKNOWN, 0,"", "",        Arg::Unknown, "USAGE: patter-miner [options]\n\n"
                                          "Options:" },
{ HELP,    0,"", "help",    Arg::None,    "  \t--help  \tPrint usage and exit." },
{ INPUT_FILENAME ,0,"i","input-filename",Arg::Required,"  -i <filename>, \t--input-filename=<filename>  \tPath to quotes in finam format" },
{ DEBUG_MODE, 0, "d", "debug", Arg::None, "  \t-d, \t--debug"
											"  \tEnables debug output" },
{ MINER_TYPE ,0,"","miner-type",Arg::Required,"  --miner-type={c,t,z}  \tSpecifies miner type (default is 'c')." },
{ OUTPUT_FILENAME ,0,"","output-filename",Arg::Required,"  --output-filename=<filename>  \tSpecifies filename for generated report." },
{ REPORT_TYPE ,0,"","report-type",Arg::Required,"  --report-type={html,txt}  \tSpecifies report format." },
{ CONFIG_FILE ,0,"","config", Arg::Required,"  --config=<filename>  \tSpecifies config file." },
{ 0, 0, 0, 0, 0, 0 } };

enum ReportType
{
	ReportTypeUnknown,
	Html,
	Txt
};

struct Settings
{
	Settings() : debugMode(false),
		minerType(minerCandle),
		reportType(Html)
	{
	}
	std::list<std::string> inputFilename;
	std::string outputFilename;
	std::string configFilename;
	bool debugMode;
	MinerType minerType;
	ReportType reportType;
};

static Settings parseOptions(int argc, char** argv)
{
	Settings settings;

	argc-=(argc>0); argv+=(argc>0); // skip program name argv[0] if present
	option::Stats stats(usage, argc, argv);

	option::Option* options = new option::Option[stats.options_max];
	option::Option* buffer = new option::Option[stats.buffer_max];

	option::Parser parse(usage, argc, argv, options, buffer);

	if(parse.error())
		throw std::runtime_error("Unable to parse options");

	if(options[HELP] || argc == 0)
	{
		int columns = getenv("COLUMNS")? atoi(getenv("COLUMNS")) : 80;
		option::printUsage(fwrite, stdout, usage, columns);
		exit(0);
	}

	if(!options[INPUT_FILENAME])
	{
		throw std::runtime_error("Should specify input filename (--input-filename)");
	}
	for(option::Option* opt = options[INPUT_FILENAME].first(); opt; opt = opt->next())
	{
		settings.inputFilename.push_back(std::string(opt->arg));
	}

	if(options[MINER_TYPE])
	{
		if(options[MINER_TYPE].arg[0] == 'c')
		{
			settings.minerType = minerCandle;
		}
		else if(options[MINER_TYPE].arg[0] == 't')
		{
			settings.minerType = minerTime;
		}
		else if(options[MINER_TYPE].arg[0] == 'z')
		{
			settings.minerType = minerZigzag;
		}
		else
		{
			throw std::runtime_error("Miner type should be 'c' or 't'");
		}
	}

	if(!options[OUTPUT_FILENAME])
	{
		throw std::runtime_error("Should specify output filename (--output-filename)");
	}
	settings.outputFilename = options[OUTPUT_FILENAME].arg;

	if(!options[CONFIG_FILE])
	{
		throw std::runtime_error("Should specify config filename (--config)");
	}
	settings.configFilename = options[CONFIG_FILE].arg;

	if(options[REPORT_TYPE])
	{
		std::string t = options[REPORT_TYPE].arg;
		if((t == "h") || (t == "html"))
		{
			settings.reportType = Html;
		}
		else if((t == "t") || (t == "txt") || (t == "text"))
		{
			settings.reportType = Txt;
		}
		else
		{
			throw std::runtime_error("Unknown report type: " + t);
		}
	}

	settings.debugMode = options[DEBUG_MODE] ? true : false;
	return settings;
}

int main(int argc, char** argv)
{
	setlocale(LC_NUMERIC, NULL);

	Settings s = parseOptions(argc, argv);

	initLogging("pattern-mining.log", s.debugMode);

	std::vector<Quotes::Ptr> q;
	std::list<std::string> tickers;
	for(const auto& fname : s.inputFilename)
	{
		auto tq = std::make_shared<Quotes>();
		tq->loadFromCsv(fname);
		q.push_back(tq);
		LOG(INFO) << "Loaded " << tq->name() << ", " << tq->length() << " points";
		tickers.push_back(tq->name());
	}

	ReportBuilder::Ptr report;
	if(s.reportType == ReportType::Html)
	{
		report = std::make_shared<HtmlReportBuilder>();
	}
	else if(s.reportType == ReportType::Txt)
	{
		report = std::make_shared<TextReportBuilder>();
	}
	else
	{
		throw std::runtime_error("Invalid report type requested");
	}

	IMiner::Ptr miner;
	if(s.minerType == minerCandle)
	{
		miner = std::make_shared<Miner>();
	}
	else if(s.minerType == minerZigzag)
	{
		miner = std::make_shared<MinmaxMiner>();
	}

	std::ifstream configFile(s.configFilename, std::fstream::binary);
	if(!configFile.good())
		throw std::runtime_error("Unable to open config file");
	Json::Value root;
	configFile >> root;

	miner->parseConfig(root);
	miner->setQuotes(q);
	miner->mine();
	miner->makeReport(report, s.outputFilename);
}

