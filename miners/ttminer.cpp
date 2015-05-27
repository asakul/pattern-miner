
#include "ttminer.h"
#include <map>
#include "log.h"

TtMiner::TtMiner(const TtMiner::Params& params) :
	m_params(params)
{
}

TtMiner::~TtMiner()
{
}

std::vector<TtMiner::Result> TtMiner::mine(Quotes& q)
{
	std::vector<Result> result;
	int last_percent = 0;
	std::vector<int> scanned(q.length(), 0);
	for(size_t pos = 0; pos < q.length(); pos++)
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

		double mean = 0;
		int counter = 0;
		double min_return = 1.0;
		double max_return = -1.0;
		double min_low = 1.0;
		double max_high = -1.0;
		int pos_returns = 0;
		int neg_returns = 0;
		std::vector<double> returns;

		for(size_t scanPos = 0; scanPos < q.length() - m_params.exitAfter; scanPos++)
		{
			if((q[scanPos].time.sec % 86400) == (q[pos].time.sec % 86400))
			{
				size_t exitPos = scanPos + m_params.exitAfter - 1;
				double this_return = (q[exitPos].close - q[scanPos].open) / q[scanPos].open;
				double this_low = (q[scanPos].low - q[scanPos].open) / q[scanPos].open;
				double this_high = (q[scanPos].high - q[scanPos].open) / q[scanPos].open;
				for(int offset = 0; offset < m_params.exitAfter; offset++)
				{
					this_low = std::min(this_low, (q[scanPos + offset].low - q[scanPos].open) / q[scanPos].open);
					this_high = std::max(this_high, (q[scanPos + offset].high - q[scanPos].open) / q[scanPos].open);
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
			double f = fabs(pos_returns - (double)counter / 2) / sigma;
			double p = (1 - erf(f));

			Result r;
			r.time = q[pos].time.sec % 86400;
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

