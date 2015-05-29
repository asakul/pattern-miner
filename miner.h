#ifndef MINER_H
#define MINER_H

#include "model/quotes.h"
#include "model/fitelement.h"
#include <list>

class Miner
{
public:
	struct Result
	{
		std::vector<FitElement> elements;
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
			exitAfter(1)
		{
		}
		double candleFit;
		double volumeFit;
		int patternLength;
		double limit;
		int exitAfter;
	};

	Miner(const Params& p);
	virtual ~Miner();

	std::vector<Result> mine(std::list<Quotes::Ptr>& qlist);

private:
	Params m_params;
};

#endif
