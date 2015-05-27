#ifndef TTMINER_H_03QH29KG
#define TTMINER_H_03QH29KG

#include "model/quotes.h"

class TtMiner
{
public:
	struct Result
	{
		int time;
		double mean;
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
		Params() : limit(-1),
			exitAfter(1)
		{
		}

		double limit;
		int exitAfter;
	};

	TtMiner(const Params& params);
	virtual ~TtMiner();

	std::vector<Result> mine(Quotes& q);

private:
	Params m_params;
};

#endif /* end of include guard: TTMINER_H_03QH29KG */
