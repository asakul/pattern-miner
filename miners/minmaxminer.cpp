
#include "minmaxminer.h"
#include "log.h"
#include <cassert>
#include <cmath>
#include <boost/math/distributions.hpp> 

using namespace boost::math;
static const int MaxZigzags = 32;

static const double alpha[] = { 0.00001, 0.0001, 0.001, 0.01, 0.05, 0.10, 0.25, 0.5, 1 };

static bool isExtremum(const Quotes::Ptr& q, size_t pos, int epsilon, bool minimum)
{
	if(((int)pos < epsilon) || (pos > q->length() - epsilon - 1))
	{
		return false;
	}
	auto value = q->at(pos).close;
	for(size_t p = pos - epsilon; p <= pos + epsilon; p++)
	{
		if(p == pos)
			continue;

		if(minimum)
		{
			if(q->at(p).close < value)
				return false;
		}
		else
		{
			if(q->at(p).close > value)
				return false;
		}
	}
	return true;
}

static bool isMinimum(const Quotes::Ptr& q, size_t pos, int epsilon)
{
	return isExtremum(q, pos, epsilon, true);
}

static bool isMaximum(const Quotes::Ptr& q, size_t pos, int epsilon)
{
	return isExtremum(q, pos, epsilon, false);
}

static std::vector<ZigzagElement> findZigzags(const Quotes::Ptr& q, size_t start_pos, int epsilon, int zigzags)
{
	assert(zigzags > 1);

	std::vector<ZigzagElement> result;

	double unitPrice = 0;
	double unitVolume = 0;
	size_t firstPos = 0;
	size_t pos = start_pos;
	while(pos < q->length())
	{
		bool minimum = isMinimum(q, pos, epsilon);
		bool maximum = isMaximum(q, pos, epsilon);
		if(minimum || maximum)
		{
			if(result.empty())
			{
				unitPrice = q->at(pos).close;
				unitVolume = q->at(pos).volume;
				firstPos = pos;
				ZigzagElement el;
				el.time = 0;
				el.price = 1;
				el.volume = 1;
				el.minimum = minimum;
				result.push_back(el);
			}
			else
			{
				ZigzagElement el;
				el.time = pos - firstPos;
				el.price = q->at(pos).close / unitPrice;
				el.volume = q->at(pos).volume / unitVolume;
				el.minimum = minimum;
				result.push_back(el);
			}
		}
		if(result.size() >= (size_t)zigzags)
			break;
		pos++;
	}
	
	return result;
}

bool MinmaxMiner::matchZigzags(const Quotes::Ptr& q, size_t pos, const std::vector<ZigzagElement>& zigzags, double tolerance, int momentumSign)
{
	auto currentZigzags = findZigzags(q, pos, m_params.epsilon, zigzags.size());
	if(currentZigzags.size() != zigzags.size())
		return false;

	int thisMomentumSign = 0;
	if(m_params.momentumOrder > 0)
	{
		if((int)pos - m_params.momentumOrder < 0)
			thisMomentumSign = 0;
		else
			thisMomentumSign = q->at(pos - m_params.momentumOrder).close - q->at(pos).open > 0 ? 1 : -1;
	}

	if(thisMomentumSign != momentumSign)
		return false;

	for(size_t i = 0; i < currentZigzags.size(); i++)
	{
		if(fabs(currentZigzags[i].price - zigzags[i].price) > tolerance)
			return false;
		if(m_params.volumeTolerance > 0)
		{
			if(fabs(currentZigzags[i].volume - zigzags[i].volume) > m_params.volumeTolerance)
				return false;
		}
		if(abs(currentZigzags[i].time - zigzags[i].time) > m_params.timeTolerance)
			return false;
		if(currentZigzags[i].minimum != zigzags[i].minimum)
			return false;
	}
	return true;
}

MinmaxMiner::MinmaxMiner()
{

}

MinmaxMiner::MinmaxMiner(const Params& params) : m_params(params)
{
}

MinmaxMiner::~MinmaxMiner()
{
}

std::vector<MinmaxMiner::Result> MinmaxMiner::doMine(std::vector<Quotes::Ptr>& qlist)
{
	std::vector<MinmaxMiner::Result> result;

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
		for(size_t pos = 0; pos < qbase->length() - 1; pos++)
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
			auto zigzags = findZigzags(qbase, pos, m_params.epsilon, m_params.zigzags);
			if(zigzags.size() < (size_t)m_params.zigzags)
				continue;

			int baseMomentumSign = 0;
			if(m_params.momentumOrder > 0)
			{
				if((int)pos - m_params.momentumOrder < 0)
					baseMomentumSign = 0;
				else
					baseMomentumSign = qbase->at(pos - m_params.momentumOrder).close - qbase->at(pos).open > 0 ? 1 : -1;
			}

			double mean = 0;
			int counter = 0;
			double min_return = 1.0;
			double max_return = -1.0;
			double mean_pos = 0;
			double mean_neg = 0;
			int pos_returns = 0;
			int neg_returns = 0;
			double sigma = 0;
			std::vector<double> returns;

			double abs_min = 10000000;
			double abs_max = -10000000;
			for(size_t i = 0; i < zigzags.size(); i++)
			{
				abs_min = std::min(abs_min, zigzags[i].price);
				abs_max = std::max(abs_max, zigzags[i].price);
			}
			double tolerance = (abs_max - abs_min) * m_params.priceTolerance;

			int scanIndex = 0;
			for(const auto& qscan : qlist)
			{
				for(size_t scanPos = 0; scanPos < qscan->length(); scanPos++)
				{
					if(matchZigzags(qscan, scanPos, zigzags, tolerance, baseMomentumSign))
					{
						size_t lastPos = scanPos + zigzags.back().time + m_params.epsilon;
						size_t exitPos = lastPos + m_params.exitAfter;

						double lastPrice = qscan->at(lastPos).close;
						double exitPrice = qscan->at(exitPos).close;

						double ret = (exitPrice - lastPrice) / lastPrice;

						min_return = std::min(min_return, ret);
						max_return = std::max(max_return, ret);

						if(exitPrice > lastPrice)
						{
							mean_pos += ret;
							pos_returns++;
						}
						else
						{
							mean_neg += ret;
							neg_returns++;
						}
						mean += ret;
						sigma += ret * ret;
						returns.push_back(ret);

						counter++;
						scanned[scanIndex + scanPos] = 1;
					}
				}
				scanIndex += qscan->length();
			}

			if(counter > 1)
			{
				mean = mean / counter;
				sigma = sigma / counter;
				Result r;
				r.momentumSign = baseMomentumSign;
				r.elements = zigzags;
				r.mean = mean;
				r.sigma = sqrt(sigma - mean * mean);
				r.count = counter;
				r.pos_returns = pos_returns;
				r.neg_returns = neg_returns;
				r.min_return = min_return;
				r.max_return = max_return;

				double binomial_sigma = sqrt(counter);
				double q = fabs(pos_returns - (double)counter / 2) / binomial_sigma;
				double p = (1 - erf(q));
				r.p = p;

				students_t dist(counter - 1);

				double students_factor = r.sigma / sqrt(counter);
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


void MinmaxMiner::parseConfig(const Json::Value& root)
{
	auto minerRoot = root["miner"];
	m_params.limit = root.get("sample-percentage", -1).asDouble();
	m_params.exitAfter = root.get("exit-after", 2).asUInt();
	m_params.momentumOrder = root.get("momentum-order", -1).asInt();
	m_params.zigzags = root.get("zigzags", 2).asInt();
	m_params.epsilon = root.get("epsilon", 6).asInt();
	m_params.priceTolerance = root.get("price-tolerance", 0.1).asDouble();
	m_params.volumeTolerance = root.get("volume-tolerance", -1).asDouble();
	m_params.momentumOrder = root.get("momentum-order", 0).asInt();

	auto reportConfig = root["report"];
	m_reportConfig.swap(reportConfig);
}

void MinmaxMiner::setQuotes(const std::vector<Quotes::Ptr>& quotes)
{
	m_quotes = quotes;
}

void MinmaxMiner::mine()
{
	m_results = doMine(m_quotes);
}

void MinmaxMiner::makeReport(const ReportBuilder::Ptr& builder,
		const std::string& filename)
{
	std::sort(m_results.begin(), m_results.end(), [] (const MinmaxMiner::Result& r1, const MinmaxMiner::Result& r2) {
			return r1.count > r2.count;
			});

	auto outputFilename = m_reportConfig.get("output-filename", filename).asString();
	double filterP = m_reportConfig.get("filter-p", 0).asDouble();
	double filterMean = m_reportConfig.get("filter-mean", 0).asDouble();
	double filterMeanP = m_reportConfig.get("filter-mean-p", 0).asDouble();
	int filterCount = m_reportConfig.get("filter-count", 0).asInt();
	bool filterTrivial = m_reportConfig.get("filter-trivial", false).asBool();

	builder->start(outputFilename, TimePoint(0, 0), TimePoint(0, 0), std::list<std::string>());
	builder->begin_element("Parameters:");
	builder->insert_text("Price tolerance: " + std::to_string(m_params.priceTolerance));
	builder->insert_text("Volume tolerance: " + std::to_string(m_params.volumeTolerance));
	builder->insert_text("Time tolerance: " + std::to_string(m_params.timeTolerance));
	builder->insert_text("Zigzags: " + std::to_string(m_params.zigzags));
	builder->insert_text("Epsilon: " + std::to_string(m_params.epsilon));
	builder->insert_text("Exit after: " + std::to_string(m_params.exitAfter) + " periods");
	if(filterP > 0)
		builder->insert_text("Filter binomial p-value: < " + std::to_string(filterP));
	if(filterMean > 0)
		builder->insert_text("Filter absolute mean value: <" + std::to_string(filterMean));
	if(filterMeanP > 0)
		builder->insert_text("Filter absolute mean p-value: <" + std::to_string(filterMeanP));
	if(filterCount > 0)
		builder->insert_text("Filter pattern occurences: >" + std::to_string(filterCount));
	builder->end_element();

	int patternsCount = 0;
	for(const auto& r : m_results)
	{
		if(filterP > 0)
		{
			if(r.p > filterP)
				continue;
		}

		if(filterMean > 0)
		{
			if(fabs(r.mean) < filterMean)
				continue;
		}

		if(filterMeanP > 0)
		{
			if(r.mean_p > filterMeanP)
				continue;
		}

		if(filterCount > 0)
		{
			if(r.count < filterCount)
				continue;
		}

		if(filterTrivial)
		{
			bool isTrivial = true;
			for(const auto& e : r.elements)
			{
				if(e.price != 1)
				{
					isTrivial = false;
					break;
				}
			}
			if(isTrivial)
				continue;
		}

		builder->begin_element("Pattern: " + std::to_string(r.count) + " occurences");
		for(const auto& el : r.elements)
		{
			builder->insert_text("Z" + std::to_string(el.time) + ":" + std::to_string(el.price) + "/" + std::to_string(el.volume) + "(" + (el.minimum ? std::string("min") : std::string("max")) + ")");
		}
		builder->insert_text("mean = " + std::to_string(r.mean) + "; rejecting H0 at p-value: " +
			  std::to_string(r.mean_p) + "; sigma = " + std::to_string(r.sigma));
		builder->insert_text("Minmax returns: " + std::to_string(r.min_return) + "/" + std::to_string(r.max_return) +
				"; median return: " + std::to_string(r.median));
		builder->insert_text("+ returns: " + std::to_string((double)r.pos_returns / r.count) +
				"; p-value: " + std::to_string(r.p));
		builder->insert_text("Momentum sign: " + std::to_string(r.momentumSign));
		builder->end_element();

		patternsCount += r.count;
	}
	builder->begin_element("Total patterns: " + std::to_string(patternsCount));
	builder->end_element();
	builder->end();
}

