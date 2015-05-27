#include "miner.h"
#include <cassert>
#include "log.h"
#include <cmath>

static const int MaxPatternLength = 32;

static void convertToRelativeUnits(Quotes& q, size_t startPos, int patternLength, FitElement* buffer)
{
	auto startPrice = q[startPos].open;
	double startVolume = q[startPos].volume;
	for(int i = 0; i < patternLength; i++)
	{
		buffer[i].open = q[startPos + i].open / startPrice;
		buffer[i].high = q[startPos + i].high / startPrice;
		buffer[i].low = q[startPos + i].low / startPrice;
		buffer[i].close = q[startPos + i].close / startPrice;
		buffer[i].volume = (double)q[startPos + i].volume / startVolume;
	}
}

static bool fit(FitElement* f1, FitElement* f2, double candleTolerance, double volumeTolerance, int length)
{
	double abs_min = 10000000;
	double abs_max = -10000000;
	for(int i = 0; i < length; i++)
	{
		abs_min = std::min(abs_min, std::min(f1[i].low, f2[i].low));
		abs_max = std::max(abs_max, std::max(f1[i].high, f2[i].high));
	}
	double tolerance = (abs_max - abs_min) * candleTolerance;
	for(int i = 0; i < length; i++)
	{
		if(fabs(f1[i].open - f2[i].open) > tolerance)
			return false;
		if(fabs(f1[i].close - f2[i].close) > tolerance)
			return false;
		if(fabs(f1[i].high - f2[i].high) > tolerance)
			return false;
		if(fabs(f1[i].low - f2[i].low) > tolerance)
			return false;
		if(volumeTolerance > 0)
		{
			if(fabs(f1[i].volume - f2[i].volume) > volumeTolerance)
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

std::vector<Miner::Result> Miner::mine(Quotes& q)
{
	std::vector<Result> result;
	FitElement fitBuffer[MaxPatternLength];
	FitElement scanBuffer[MaxPatternLength];
	int last_percent = 0;
	std::vector<int> scanned(q.length(), 0);
	for(size_t pos = m_params.patternLength; pos < q.length() - 1; pos++)
	{
		if(m_params.limit > 0)
		{
			if((double)pos / q.length() * 100 > (size_t)m_params.limit)
				break;
		}

		if(scanned[pos])
			continue;

		int current_percent = (double)pos / q.length() * 1000;
		if(current_percent != last_percent)
		{
			LOG(DEBUG) << (double)current_percent / 10 << "% done";
			last_percent = current_percent;
		}
		size_t startPos = pos - m_params.patternLength;
		convertToRelativeUnits(q, startPos, m_params.patternLength, fitBuffer);

		double mean = 0;
		int counter = 0;
		double min_return = 1.0;
		double max_return = -1.0;
		double min_low = 1.0;
		double max_high = -1.0;
		int pos_returns = 0;
		int neg_returns = 0;
		std::vector<double> returns;

		for(size_t scanPos = m_params.patternLength; scanPos < q.length() - m_params.patternLength - m_params.exitAfter; scanPos++)
		{
			convertToRelativeUnits(q, scanPos, m_params.patternLength, scanBuffer);
			if(fit(fitBuffer, scanBuffer, m_params.candleFit, m_params.volumeFit, m_params.patternLength))
			{
				size_t nextPos = scanPos + m_params.patternLength;
				size_t exitPos = scanPos + m_params.patternLength + m_params.exitAfter - 1;
				double this_return = (q[exitPos].close - q[nextPos].open) / q[nextPos].open;
				double this_low = (q[nextPos].low - q[nextPos].open) / q[nextPos].open;
				double this_high = (q[nextPos].high - q[nextPos].open) / q[nextPos].open;
				for(int offset = 0; offset < m_params.exitAfter; offset++)
				{
					this_low = std::min(this_low, (q[nextPos + offset].low - q[nextPos].open) / q[nextPos].open);
					this_high = std::max(this_high, (q[nextPos + offset].high - q[nextPos].open) / q[nextPos].open);
				}

				if(this_return > max_return)
					max_return = this_return;
				if(this_return < min_return)
					min_return = this_return;
				min_low = std::min(min_low, this_low);
				max_high = std::max(max_high, this_high);
				if(this_return > 0)
					pos_returns++;
				if(this_return <= 0)
					neg_returns++;
				mean += this_return;
				returns.push_back(this_return);
				counter++;
				scanned[scanPos] = 1;
			}
		}
		mean /= counter;
		if(counter > 1)
		{
			double sigma = sqrt(counter);
			double q = fabs(pos_returns - (double)counter / 2) / sigma;
			double p = (1 - erf(q));

			Result r;
			for(int i = 0; i < m_params.patternLength; i++)
			{
				r.elements.push_back(fitBuffer[i]);
			}
			r.mean = mean;
			r.count = counter;
			r.pos_returns = pos_returns;
			r.p = p;
			r.min_return = min_return;
			r.max_return = max_return;
			r.min_low = min_low;
			r.max_high = max_high;
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
	return result;
}

