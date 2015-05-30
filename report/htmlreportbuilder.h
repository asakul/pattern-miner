
#ifndef HTMLREPORTBUILDER_H_X1LDRYZB
#define HTMLREPORTBUILDER_H_X1LDRYZB


#include "builder.h"
#include <fstream>
#include <boost/filesystem.hpp>

class HtmlReportBuilder : public ReportBuilder
{
public:
	HtmlReportBuilder();
	virtual ~HtmlReportBuilder();

	virtual void start(const std::string& filename,
			const TimePoint& start_time, const TimePoint& end_time,
			const std::list<std::string>& tickers);

	virtual void begin_element(const std::string& title);
	virtual void insert_fit_elements(const std::vector<FitElement>& elements);
	virtual void insert_text(const std::string& text);
	virtual void end_element();

	virtual void end();

private:
	boost::filesystem::path m_root;
	std::fstream m_main;
	int m_imageCounter;
};

#endif /* end of include guard: HTMLREPORTBUILDER_H_X1LDRYZB */
