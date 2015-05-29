
#ifndef BUILDER_H_VSEPZ1XQ
#define BUILDER_H_VSEPZ1XQ

#include <string>
#include <list>
#include <vector>
#include <memory>
#include "model/candle.h"
#include "model/fitelement.h"

class ReportBuilder
{
public:
	typedef std::shared_ptr<ReportBuilder> Ptr;

	virtual ~ReportBuilder() {}

	virtual void start(const std::string& filename,
			const TimePoint& start_time, const TimePoint& end_time,
			const std::list<std::string>& tickers) = 0;

	virtual void begin_element(const std::string& title) = 0;
	virtual void insert_fit_elements(const std::vector<FitElement>& elements) = 0;
	virtual void insert_text(const std::string& text) = 0;
	virtual void end_element() = 0;

	virtual void end() = 0;
};


#endif /* end of include guard: BUILDER_H_VSEPZ1XQ */
