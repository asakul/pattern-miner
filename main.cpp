
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

enum  optionIndex { UNKNOWN, HELP, INPUT_FILENAME, CANDLE_TOLERANCE, VOLUME_TOLERANCE, PATTERN_LENGTH };
const option::Descriptor usage[] = {
{ UNKNOWN, 0,"", "",        Arg::Unknown, "USAGE: patter-miner [options]\n\n"
                                          "Options:" },
{ HELP,    0,"", "help",    Arg::None,    "  \t--help  \tPrint usage and exit." },
{ INPUT_FILENAME ,0,"i","input-filename",Arg::Required,"  -i <filename>, \t--input-filename=<filename>  \tPath to quotes in finam format" },
{ PATTERN_LENGTH ,0,"p","pattern-length",Arg::PatternLength,"  -p <arg>, \t--pattern-length=<arg>"
                                          "  \tSpecifies pattern length." },
{ CANDLE_TOLERANCE, 0,"c","candle-tolerance", Arg::Double, "  -c <num>, \t--candle-tolerance=<num>  \tHow precise candle features should be matched." },
{ VOLUME_TOLERANCE, 0,"o","volume-tolerance", Arg::Double, "  -o <num>, \t--volume-tolerance=<num>  \tHow precise volumes should be matched." },

{ 0, 0, 0, 0, 0, 0 } };

int main(int argc, char** argv)
{
	setlocale(LC_NUMERIC, NULL);

	argc-=(argc>0); argv+=(argc>0); // skip program name argv[0] if present
	option::Stats stats(usage, argc, argv);

	option::Option options[stats.options_max], buffer[stats.buffer_max];

	option::Parser parse(usage, argc, argv, options, buffer);

	if (parse.error())
		return 1;

	if (options[HELP] || argc == 0)
	{
		int columns = getenv("COLUMNS")? atoi(getenv("COLUMNS")) : 80;
		option::printUsage(fwrite, stdout, usage, columns);
		return 0;
	}

	if(!options[INPUT_FILENAME])
	{
		printf("Should specify input filename (--input-filename)");
		return 1;
	}
	std::string inputFilename = options[INPUT_FILENAME].arg;

	if(!options[PATTERN_LENGTH])
	{
		printf("Should specify pattern length (--pattern-length)");
		return 1;
	}
	int patternLength = lexical_cast<int>(options[PATTERN_LENGTH].arg);

	if(!options[CANDLE_TOLERANCE])
	{
		printf("Should specify candle tolerance factor (--candle-tolerance)");
		return 1;
	}
	double candleTolerance = lexical_cast<double>(options[CANDLE_TOLERANCE].arg);
	double volumeTolerance = -1;
	if(options[VOLUME_TOLERANCE])
	{
		volumeTolerance = lexical_cast<double>(options[VOLUME_TOLERANCE].arg);
	}

	initLogging("pattern-mining.log", true);

	Quotes q;
	q.loadFromCsv(inputFilename);

	LOG(INFO) << "Loaded " << q.name() << ", " << q.length() << " points";

	Miner m(candleTolerance, volumeTolerance, patternLength, 20000);
	auto result = m.mine(q);

	std::sort(result.begin(), result.end(), [] (const Miner::Result& r1, const Miner::Result& r2) {
				return r1.count > r2.count;
			});

	for(const auto& r : result)
	{
		if(r.p > 0.1)
			continue;
		if(r.count < 5)
			continue;
		if(fabs(r.mean) < 0.0005)
			continue;
		LOG(INFO) << "Pattern: " << r.count << " occurences, mean = " << r.mean << "; minmax: " << r.min_return << "/" << r.max_return << "; median: " << r.median;
		LOG(INFO) << "+ returns: " << (double)r.pos_returns / r.count << "; p-value: " << r.p;
		int i = 0;
		for(const auto& e : r.elements)
		{
			LOG(INFO) << "C" << i << ": OHLCV:" << e.open << ":" <<
				e.high << ":" << e.low << ":" << e.close << ":" << e.volume;
			i++;
		}
	}
}

