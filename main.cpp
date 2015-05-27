
#include "log.h"
#include "model/quotes.h"
#include "miner.h"
#include "optionparser/optionparser.h"
#include <boost/lexical_cast.hpp>

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

enum  optionIndex { UNKNOWN, HELP, INPUT_FILENAME, CANDLE_TOLERANCE, VOLUME_TOLERANCE, PATTERN_LENGTH,
	DEBUG_MODE, SEARCH_LIMIT,
	FILTER_P, FILTER_MEAN, FILTER_COUNT,
	EXIT_AFTER };
const option::Descriptor usage[] = {
{ UNKNOWN, 0,"", "",        Arg::Unknown, "USAGE: patter-miner [options]\n\n"
                                          "Options:" },
{ HELP,    0,"", "help",    Arg::None,    "  \t--help  \tPrint usage and exit." },
{ INPUT_FILENAME ,0,"i","input-filename",Arg::Required,"  -i <filename>, \t--input-filename=<filename>  \tPath to quotes in finam format" },
{ PATTERN_LENGTH ,0,"p","pattern-length",Arg::PatternLength,"  -p <arg>, \t--pattern-length=<arg>"
                                          "  \tSpecifies pattern length." },
{ CANDLE_TOLERANCE, 0,"c","candle-tolerance", Arg::Double, "  -c <num>, \t--candle-tolerance=<num>  \tHow precise candle features should be matched." },
{ VOLUME_TOLERANCE, 0,"o","volume-tolerance", Arg::Double, "  -o <num>, \t--volume-tolerance=<num>  \tHow precise volumes should be matched." },
{ DEBUG_MODE, 0, "d", "debug", Arg::None, "  \t-d, \t--debug"
											"  \tEnables debug output" },
{ SEARCH_LIMIT, 0,"s","search-limit", Arg::Double, "  -s <num>, \t--search-limit=<num>"
                                                   "  \tHow many patterns should be tested (percentage)." },

{ FILTER_P ,0,"","filter-p",Arg::Double,"  --filter-p=<num>  \tFilter out results, which p-value is higher than specified." },
{ FILTER_MEAN ,0,"","filter-mean",Arg::Double,"  --filter-mean=<num>  \tFilter out results, which mean absolute value is lower than specified." },
{ FILTER_COUNT ,0,"","filter-count",Arg::Numeric,"  --filter-count=<num>  \tFilter out results that occured less times than specified." },
{ EXIT_AFTER ,0,"","exit-after",Arg::Numeric,"  --exit-after=<num>  \tSpecifies how many periods to hold position." },

{ 0, 0, 0, 0, 0, 0 } };

struct Settings
{
	Settings() : minerParams(),
		inputFilename(),
		debugMode(false),
		filterP(-1),
		filterMean(-1),
		filterCount(-1)
	{
	}
	Miner::Params minerParams;
	std::string inputFilename;
	bool debugMode;
	double filterP;
	double filterMean;
	int filterCount;
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
		throw std::runtime_error("");
	}

	if(!options[INPUT_FILENAME])
	{
		throw std::runtime_error("Should specify input filename (--input-filename)");
	}
	settings.inputFilename = options[INPUT_FILENAME].arg;

	if(!options[PATTERN_LENGTH])
	{
		throw std::runtime_error("Should specify pattern length (--pattern-length)");
	}
	settings.minerParams.patternLength = lexical_cast<int>(options[PATTERN_LENGTH].arg);

	if(!options[CANDLE_TOLERANCE])
	{
		throw std::runtime_error("Should specify candle tolerance factor (--candle-tolerance)");
	}
	settings.minerParams.candleFit = lexical_cast<double>(options[CANDLE_TOLERANCE].arg);
	if(options[VOLUME_TOLERANCE])
	{
		settings.minerParams.volumeFit = lexical_cast<double>(options[VOLUME_TOLERANCE].arg);
	}

	if(options[SEARCH_LIMIT])
	{
		settings.minerParams.limit = lexical_cast<double>(options[SEARCH_LIMIT].arg);
	}

	if(options[FILTER_P])
	{
		settings.filterP = lexical_cast<double>(options[FILTER_P].arg);
	}

	if(options[FILTER_MEAN])
	{
		settings.filterMean = lexical_cast<double>(options[FILTER_MEAN].arg);
	}

	if(options[FILTER_COUNT])
	{
		settings.filterCount = lexical_cast<double>(options[FILTER_COUNT].arg);
	}

	if(options[EXIT_AFTER])
	{
		int exitAfter = lexical_cast<int>(options[PATTERN_LENGTH].arg);
		if((exitAfter < 1) || (exitAfter > 100))
		{
			throw std::runtime_error("--exit-after should take values between 1 and 100");
		}
		settings.minerParams.exitAfter = exitAfter;
	}

	settings.debugMode = options[DEBUG_MODE] ? true : false;
	return settings;
}

int main(int argc, char** argv)
{
	setlocale(LC_NUMERIC, NULL);

	Settings s = parseOptions(argc, argv);

	initLogging("pattern-mining.log", s.debugMode);

	Quotes q;
	q.loadFromCsv(s.inputFilename);

	LOG(INFO) << "Loaded " << q.name() << ", " << q.length() << " points";

	Miner m(s.minerParams);
	auto result = m.mine(q);

	std::sort(result.begin(), result.end(), [] (const Miner::Result& r1, const Miner::Result& r2) {
				return r1.count > r2.count;
			});

	for(const auto& r : result)
	{
		if(s.filterP > 0)
		{
			if(r.p > s.filterP)
				continue;
		}

		if(s.filterMean > 0)
		{
			if(fabs(r.mean) < s.filterMean)
				continue;
		}

		if(s.filterCount > 0)
		{
			if(r.count < s.filterCount)
				continue;
		}

		LOG(INFO) << "Pattern: " << r.count << " occurences, mean = " << r.mean << "; minmax: " << r.min_return << "/" << r.max_return << "; median: " << r.median;
		LOG(INFO) << "+ returns: " << (double)r.pos_returns / r.count << "; p-value: " << r.p;
		LOG(INFO) << "min low: " << r.min_low << "; max high: " << r.max_high;
		int i = 0;
		for(const auto& e : r.elements)
		{
			LOG(INFO) << "C" << i << ": OHLCV:" << e.open << ":" <<
				e.high << ":" << e.low << ":" << e.close << ":" << e.volume;
			i++;
		}
	}
}

