
#include "log.h"
#include "model/quotes.h"
#include "miner.h"
#include "miners/ttminer.h"
#include "optionparser/optionparser.h"
#include <boost/lexical_cast.hpp>
#include "report/textreportbuilder.h"

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

enum MinerType { minerCandle, minerTime };

enum  optionIndex { UNKNOWN, HELP, INPUT_FILENAME, CANDLE_TOLERANCE, VOLUME_TOLERANCE, PATTERN_LENGTH,
	DEBUG_MODE, SEARCH_LIMIT,
	FILTER_P, FILTER_MEAN, FILTER_COUNT, FILTER_MEAN_P, FILTER_TRIVIAL,
	EXIT_AFTER,
	MINER_TYPE,
	OUTPUT_FILENAME
};
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
{ FILTER_MEAN_P ,0,"","filter-mean-p",Arg::Double,"  --filter-mean-p=<num>  \tFilter out results, which mean return p-value is higher than specified." },
{ FILTER_TRIVIAL ,0,"","filter-trivial",Arg::None,"  --filter-trivial  \tFilter out trivial results (like zero-sized candles)." },
{ FILTER_COUNT ,0,"","filter-count",Arg::Numeric,"  --filter-count=<num>  \tFilter out results that occured less times than specified." },
{ EXIT_AFTER ,0,"","exit-after",Arg::Numeric,"  --exit-after=<num>  \tSpecifies how many periods to hold position." },
{ MINER_TYPE ,0,"","miner-type",Arg::Required,"  --miner-type={c,t}  \tSpecifies miner type (default is 'c')." },
{ OUTPUT_FILENAME ,0,"","output-filename",Arg::Required,"  --output-filename=<filename>  \tSpecifies filename for generated report." },

{ 0, 0, 0, 0, 0, 0 } };

struct Settings
{
	Settings() : minerParams(),
		inputFilename(),
		debugMode(false),
		filterP(-1),
		filterMean(-1),
		filterCount(-1),
		filterMeanP(-1),
		filterTrivial(false),
		minerType(minerCandle)
	{
	}
	Miner::Params minerParams;
	TtMiner::Params ttminerParams;
	std::list<std::string> inputFilename;
	bool debugMode;
	double filterP;
	double filterMean;
	int filterCount;
	double filterMeanP;
	bool filterTrivial;
	MinerType minerType;
	std::string outputFilename;
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

	if(!options[PATTERN_LENGTH])
	{
		if(settings.minerType == minerCandle)
			throw std::runtime_error("Should specify pattern length (--pattern-length)");
	}
	else
	{
		settings.minerParams.patternLength = lexical_cast<int>(options[PATTERN_LENGTH].arg);
	}

	if(!options[CANDLE_TOLERANCE])
	{
		if(settings.minerType == minerCandle)
			throw std::runtime_error("Should specify candle tolerance factor (--candle-tolerance)");
	}
	else
	{
		settings.minerParams.candleFit = lexical_cast<double>(options[CANDLE_TOLERANCE].arg);
	}
	if(options[VOLUME_TOLERANCE])
	{
		settings.minerParams.volumeFit = lexical_cast<double>(options[VOLUME_TOLERANCE].arg);
	}

	if(options[SEARCH_LIMIT])
	{
		settings.minerParams.limit = lexical_cast<double>(options[SEARCH_LIMIT].arg);
		settings.ttminerParams.limit = lexical_cast<double>(options[SEARCH_LIMIT].arg);
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

	if(options[FILTER_MEAN_P])
	{
		settings.filterMeanP = lexical_cast<double>(options[FILTER_MEAN_P].arg);
	}

	if(options[FILTER_TRIVIAL])
	{
		settings.filterTrivial = true;
	}

	if(options[EXIT_AFTER])
	{
		int exitAfter = lexical_cast<int>(options[PATTERN_LENGTH].arg);
		if((exitAfter < 1) || (exitAfter > 100))
		{
			throw std::runtime_error("--exit-after should take values between 1 and 100");
		}
		settings.minerParams.exitAfter = exitAfter;
		settings.ttminerParams.exitAfter = exitAfter;
	}


	settings.debugMode = options[DEBUG_MODE] ? true : false;
	return settings;
}

int main(int argc, char** argv)
{
	setlocale(LC_NUMERIC, NULL);

	Settings s = parseOptions(argc, argv);

	initLogging("pattern-mining.log", s.debugMode);

	std::list<Quotes::Ptr> q;
	std::list<std::string> tickers;
	for(const auto& fname : s.inputFilename)
	{
		auto tq = std::make_shared<Quotes>();
		tq->loadFromCsv(fname);
		q.push_back(tq);
		LOG(INFO) << "Loaded " << tq->name() << ", " << tq->length() << " points";
		tickers.push_back(tq->name());
	}


	if(s.minerType == minerCandle)
	{
		LOG(DEBUG) << "Limit: " << s.minerParams.limit;
		Miner m(s.minerParams);
		auto result = m.mine(q);

		std::sort(result.begin(), result.end(), [] (const Miner::Result& r1, const Miner::Result& r2) {
				return r1.count > r2.count;
				});


		auto report = std::make_shared<TextReportBuilder>();
		report->start(s.outputFilename, TimePoint(0, 0), TimePoint(0, 0), tickers);

		int patternsCount = 0;
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

			if(s.filterMeanP > 0)
			{
				if(r.mean_p > s.filterMeanP)
					continue;
			}

			if(s.filterTrivial)
			{
				bool isTrivial = true;
				for(const auto& e : r.elements)
				{
					if((e.open != 1) || (e.high != 1) || (e.low != 1) || (e.close != 1))
					{
						isTrivial = false;
						break;
					}
				}
				if(isTrivial)
					continue;
			}

			report->begin_element("Pattern: " + std::to_string(r.count) + " occurences");
			report->insert_fit_elements(r.elements);
			report->insert_text("mean = " + std::to_string(r.mean) + "; rejecting H0 at p-value: " +
				  std::to_string(r.mean_p) + "; sigma = " + std::to_string(r.sigma));
			report->insert_text("Minmax returns: " + std::to_string(r.min_return) + "/" + std::to_string(r.max_return) +
					"; median return: " + std::to_string(r.median));
			report->insert_text("+ returns: " + std::to_string((double)r.pos_returns / r.count) +
					"; p-value: " + std::to_string(r.p));
			report->insert_text("min low: " + std::to_string(r.min_low) + "; max high: " + std::to_string(r.max_high));
			report->insert_text("mean +: " + std::to_string(r.mean_pos) + "; mean -: " + std::to_string(r.mean_neg));
			report->end_element();

			patternsCount += r.count;
		}
		report->insert_text("Total patterns: " + std::to_string(patternsCount));
		report->end();
	}
}

