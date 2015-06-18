#include "miner.h"
#include <cassert>
#include "log.h"
#include <cmath>
#include <boost/math/distributions.hpp> 

using namespace boost::math;

static const int MaxPatternLength = 32;

static const double alpha[] = { 0.00001, 0.0001, 0.001, 0.01, 0.05, 0.10, 0.25, 0.5, 1 };

struct Pattern
{
	int momentumSign;
	std::vector<FitElement> elements;
};

static Pattern convertToRelativeUnits(Quotes& q, size_t startPos, int patternLength, int momentumOrder)
{
	auto startPrice = q[startPos].open;
	double startVolume = q[startPos].volume;
	Pattern pattern;
	for(int i = 0; i < patternLength; i++)
	{
		FitElement el;
		el.open = q[startPos + i].open / startPrice;
		el.high = q[startPos + i].high / startPrice;
		el.low = q[startPos + i].low / startPrice;
		el.close = q[startPos + i].close / startPrice;
		el.volume = (double)q[startPos + i].volume / startVolume;
		pattern.elements.push_back(el);
	}

	if(momentumOrder > 0)
	{
		if((int)startPos - momentumOrder < 0)
			pattern.momentumSign = 0;
		else
			pattern.momentumSign = q[startPos - momentumOrder].close - q[startPos].open > 0 ? 1 : -1;
	}
	else
	{
		pattern.momentumSign = 0;
	}
	return pattern;
}

static bool fit(const Pattern& f1, const Pattern& f2, double candleTolerance, double volumeTolerance, int length)
{
	if(f1.momentumSign != f2.momentumSign)
		return false;

	double abs_min = 10000000;
	double abs_max = -10000000;
	for(int i = 0; i < length; i++)
	{
		abs_min = std::min(abs_min, std::min(f1.elements[i].low, f2.elements[i].low));
		abs_max = std::max(abs_max, std::max(f1.elements[i].high, f2.elements[i].high));
	}
	double tolerance = (abs_max - abs_min) * candleTolerance;
	for(int i = 0; i < length; i++)
	{
		if(fabs(f1.elements[i].open - f2.elements[i].open) > tolerance)
			return false;
		if(fabs(f1.elements[i].close - f2.elements[i].close) > tolerance)
			return false;
		if(fabs(f1.elements[i].high - f2.elements[i].high) > tolerance)
			return false;
		if(fabs(f1.elements[i].low - f2.elements[i].low) > tolerance)
			return false;
		if((f1.elements[i].open - f1.elements[i].close) * (f2.elements[i].open - f2.elements[i].close) < 0)
			return false;
		if(volumeTolerance > 0)
		{
			if(fabs(f1.elements[i].volume - f2.elements[i].volume) > volumeTolerance)
				return false;
		}
	}
	return true;
}

Miner::Miner(const Params& p) : m_params(p)
{
	assert(m_params.patternLength < MaxPatternLength);
}

Miner::~Miner()
{
}

std::vector<Miner::Result> Miner::mine(std::list<Quotes::Ptr>& qlist)
{
	std::vector<Result> result;
	int total_positions = 0;
	for(const auto& q : qlist)
	{
		total_positions += q->length();
	}
	std::vector<int> scanned(total_positions, 0);
	int baseIndex = 0;
	for(const auto& qbase : qlist)
	{
		int last_percent = 0;
		for(size_t pos = m_params.patternLength; pos < qbase->length() - 1; pos++)
		{
			if(m_params.limit > 0)
			{
				if((double)pos / qbase->length() * 100 > (size_t)m_params.limit)
					break;
			}

			if(scanned[baseIndex + pos])
				continue;

			int current_percent = (double)pos / qbase->length() * 1000;
			if(current_percent != last_percent)
			{
				LOG(DEBUG) << qbase->name() << ": " << (double)current_percent / 10 << "% done";
				last_percent = current_percent;
			}
			size_t startPos = pos - m_params.patternLength;
			Pattern basePattern = convertToRelativeUnits(*qbase, startPos, m_params.patternLength, m_params.momentumOrder);

			double mean = 0;
			int counter = 0;
			double min_return = 1.0;
			double max_return = -1.0;
			double min_low = 1.0;
			double max_high = -1.0;
			double mean_pos = 0;
			double mean_neg = 0;
			int pos_returns = 0;
			int neg_returns = 0;
			std::vector<double> returns;

			int scanIndex = 0;
			for(const auto& qscan : qlist)
			{
				for(size_t scanPos = m_params.patternLength; scanPos < qscan->length() - m_params.patternLength - m_params.exitAfter; scanPos++)
				{
					Pattern thisPattern = convertToRelativeUnits(*qscan, scanPos, m_params.patternLength, m_params.momentumOrder);
					if(fit(basePattern, thisPattern, m_params.candleFit, m_params.volumeFit, m_params.patternLength))
					{
						size_t nextPos = scanPos + m_params.patternLength;
						size_t exitPos = scanPos + m_params.patternLength + m_params.exitAfter - 1;
						double this_return = (qscan->at(exitPos).close - qscan->at(nextPos).open) / qscan->at(nextPos).open;
						double this_low = (qscan->at(nextPos).low - qscan->at(nextPos).open) / qscan->at(nextPos).open;
						double this_high = (qscan->at(nextPos).high - qscan->at(nextPos).open) / qscan->at(nextPos).open;
						for(int offset = 0; offset < m_params.exitAfter; offset++)
						{
							this_low = std::min(this_low, (qscan->at(nextPos + offset).low - qscan->at(nextPos).open) / qscan->at(nextPos).open);
							this_high = std::max(this_high, (qscan->at(nextPos + offset).high - qscan->at(nextPos).open) / qscan->at(nextPos).open);
						}

						if(this_return > max_return)
							max_return = this_return;
						if(this_return < min_return)
							min_return = this_return;
						min_low = std::min(min_low, this_low);
						max_high = std::max(max_high, this_high);
						if(this_return > 0)
						{
							mean_pos += this_return;
							pos_returns++;
						}
						if(this_return <= 0)
						{
							mean_neg += this_return;
							neg_returns++;
						}
						mean += this_return;
						returns.push_back(this_return);
						counter++;
						scanned[scanIndex + scanPos] = 1;
					}
				}
				scanIndex += qscan->length();
			}
			if(pos_returns > 0)
			{
				mean_pos /= pos_returns;
			}
			if(neg_returns > 0)
			{
				mean_neg /= neg_returns;
			}
			if(counter > 1)
			{
				mean /= counter;
				double sigma = 0;
				double binomial_sigma = sqrt(counter);
				for(double r : returns)
				{
					sigma += (r - mean) * (r - mean);
				}
				if(counter > 2)
					sigma /= (counter - 1);
				else
					sigma = 0;
				sigma = sqrt(sigma);


				double q = fabs(pos_returns - (double)counter / 2) / binomial_sigma;
				double p = (1 - erf(q));

				Result r;
				r.momentumSign = basePattern.momentumSign;
				r.elements = basePattern.elements;

				students_t dist(counter - 1);

				double students_factor = sigma / sqrt(counter);
				r.mean_p  = 1;
				for(auto a : alpha)
				{
					double T = quantile(complement(dist, a / 2));
					if((mean > 0) && (mean - T * students_factor > 0))
					{
						r.mean_p = a;
						break;
					}

					if((mean < 0) && (mean + T * students_factor < 0))
					{
						r.mean_p = a;
						break;
					}
				}
				r.mean = mean;
				r.sigma = sigma;
				r.count = counter;
				r.pos_returns = pos_returns;
				r.p = p;
				r.min_return = min_return;
				r.max_return = max_return;
				r.min_low = min_low;
				r.max_high = max_high;
				r.mean_pos = mean_pos;
				r.mean_neg = mean_neg;
				if((counter % 2) == 0)
				{
					r.median = 0.5 * (returns[counter / 2 - 1] + returns[counter / 2]);
				}
				else
				{
					r.median = returns[counter / 2];
				}
				result.push_back(r);
			}
		}
		baseIndex += qbase->length();
	}
	return result;
}

