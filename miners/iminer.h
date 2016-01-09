/*
 * iminer.h
 */

#ifndef MINERS_IMINER_H_
#define MINERS_IMINER_H_

#include "json/value.h"
#include "model/quotes.h"

class IMiner
{
public:
	IMiner();
	virtual ~IMiner();

	virtual void parseConfig(const Json::Value& root) = 0;

	virtual void setQuotes(const Quotes::Ptr& quotes) = 0;

	virtual void startMining() = 0;

};

#endif /* MINERS_IMINER_H_ */
