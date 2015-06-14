
#ifndef MINMAXMINER_H_MGBU7XUN
#define MINMAXMINER_H_MGBU7XUN

#include <vector>
#include <list>
#include "model/quotes.h"
#include "model/fitelement.h"

class MinmaxMiner
{
public:
	struct Result
	{
		std::vector<ZigzagElement> elements;

		double mean;
		double mean_p;
		double sigma;
		int count;

		int pos_returns;
		int neg_returns;
		double p;

		double min_return;
		double max_return;
		double median;
	};

	struct Params
	{
		Params() : limit(-1),
			exitAfter(1),
			timeTolerance(2),
			zigzags(2),
			epsilon(6),
			priceTolerance(0.1),
			volumeTolerance(0.2)
		{
		}

		double limit;
		int exitAfter;
		int timeTolerance;
		int zigzags;
		int epsilon;
		double priceTolerance;
		double volumeTolerance;
	};

	MinmaxMiner(const Params& param);
	virtual ~MinmaxMiner();

	std::vector<Result> mine(std::list<Quotes::Ptr>& qlist);

private:
	bool matchZigzags(const Quotes::Ptr& q, size_t pos, const std::vector<ZigzagElement>& zigzags, double tolerance);

private:
	Params m_params;
};

#endif /* end of include guard: MINMAXMINER_H_MGBU7XUN */
