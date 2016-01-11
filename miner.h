#ifndef MINER_H
#define MINER_H

#include "model/quotes.h"
#include "model/fitelement.h"
#include <list>
#include "miners/iminer.h"

class Miner : public IMiner
{
public:
	struct Result
	{
		int momentumSign;
		std::vector<FitElement> elements;
		std::string signature;
		double mean;
		double sigma;
		double mean_p;
		double mean_pos;
		double mean_neg;
		int count;
		int pos_returns;
		double p;

		double min_return;
		double max_return;
		double median;

		double min_low;
		double max_high;
	};

	struct Params
	{
		Params() : candleFit(0.1), volumeFit(0),
			patternLength(2),
			limit(-1),
			exitAfter(1),
			momentumOrder(-1),
			fitSignatures(false)
		{
		}
		double candleFit;
		double volumeFit;
		int patternLength;
		double limit;
		int exitAfter;
		int momentumOrder;
		bool fitSignatures;
	};

	Miner();
	Miner(const Params& p);
	virtual ~Miner();

	virtual void parseConfig(const Json::Value& root);

	virtual void setQuotes(const std::vector<Quotes::Ptr>& quotes);

	virtual void mine();

	virtual void makeReport(const ReportBuilder::Ptr& builder,
			const std::string& filename);

	struct Pattern
	{
		int momentumSign;
		std::vector<FitElement> elements;
		std::string signature;
	};

private:
	std::vector<Result> doMine(std::vector<Quotes::Ptr>& qlist);
	bool fit(const Pattern& f1, const Pattern& f2, int length);

private:
	Params m_params;
	std::vector<Quotes::Ptr> m_quotes;
	std::vector<Result> m_results;
	Json::Value m_reportConfig;
};

#endif
