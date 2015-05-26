#ifndef MINER_H
#define MINER_H

#include "model/quotes.h"
#include <list>

struct FitElement
{
	double open;
	double high;
	double low;
	double close;
	double volume;
};

class Miner
{
public:
	struct Result
	{
		std::vector<FitElement> elements;
		double mean;
		int count;
		int pos_returns;
		double p;

		double min_return;
		double max_return;
		double median;
	};

	Miner(double candleFit, double volumeFit, int patternLength, int limit);
	virtual ~Miner();

	std::vector<Result> mine(Quotes& q);

private:
	double m_candleFit;
	double m_volumeFit;
	int m_patternLength;
	int m_limit;
};

#endif
