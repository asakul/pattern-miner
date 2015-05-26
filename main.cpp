
#include "log.h"
#include "model/quotes.h"
#include "miner.h"

int main(int argc, char** argv)
{
	setlocale(LC_NUMERIC, NULL);

	initLogging("pattern-mining.log", true);

	Quotes q;
	q.loadFromCsv("data/quotes/GAZP_070101_151231_15.txt");

	LOG(INFO) << "Loaded " << q.name() << ", " << q.length() << " points";

	Miner m(0.2, -1, 2, 20000);
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

