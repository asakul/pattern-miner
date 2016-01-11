/*
 * iminer.h
 */

#ifndef MINERS_IMINER_H_
#define MINERS_IMINER_H_

#include "json/value.h"
#include "model/quotes.h"
#include "report/builder.h"
#include <memory>

class IMiner
{
public:
	typedef std::shared_ptr<IMiner> Ptr;
	IMiner();
	virtual ~IMiner();

	virtual void parseConfig(const Json::Value& root) = 0;

	virtual void setQuotes(const std::vector<Quotes::Ptr>& quotes) = 0;

	virtual void mine() = 0;

	virtual void makeReport(const ReportBuilder::Ptr& builder,
			const std::string& filename) = 0;
};

#endif /* MINERS_IMINER_H_ */
