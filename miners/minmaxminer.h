
#ifndef MINMAXMINER_H_MGBU7XUN
#define MINMAXMINER_H_MGBU7XUN

#include <vector>
#include <list>
#include "model/quotes.h"
#include "model/fitelement.h"
#include "miners/iminer.h"

class MinmaxMiner : public IMiner
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
		int momentumSign;
	};

	struct Params
	{
		Params() : limit(-1),
			exitAfter(1),
			timeTolerance(2),
			zigzags(2),
			epsilon(6),
			priceTolerance(0.1),
			volumeTolerance(-1),
			momentumOrder(0)
		{
		}

		double limit;
		int exitAfter;
		int timeTolerance;
		int zigzags;
		int epsilon;
		double priceTolerance;
		double volumeTolerance;
		int momentumOrder;
	};

	MinmaxMiner();
	MinmaxMiner(const Params& param);
	virtual ~MinmaxMiner();

	virtual void parseConfig(const Json::Value& root);

	virtual void setQuotes(const std::vector<Quotes::Ptr>& quotes);

	virtual void mine();

	virtual void makeReport(const ReportBuilder::Ptr& builder,
			const std::string& filename);

private:
	std::vector<Result> doMine(std::vector<Quotes::Ptr>& qlist);
	bool matchZigzags(const Quotes::Ptr& q, size_t pos, const std::vector<ZigzagElement>& zigzags, double tolerance, int momentumSign);

private:
	Params m_params;
	std::vector<Quotes::Ptr> m_quotes;
	std::vector<Result> m_results;
	Json::Value m_reportConfig;
};

#endif /* end of include guard: MINMAXMINER_H_MGBU7XUN */
