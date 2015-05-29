
#include "textreportbuilder.h"

TextReportBuilder::TextReportBuilder()
{
}

TextReportBuilder::~TextReportBuilder()
{
}

void TextReportBuilder::start(const std::string& filename,
		const TimePoint& start_time, const TimePoint& end_time,
		const std::list<std::string>& tickers)
{
	m_out.open(filename.c_str(), std::ios_base::out);
}

void TextReportBuilder::begin_element(const std::string& title)
{
	m_out << "=== " << title << " ===" << std::endl;
}

void TextReportBuilder::insert_fit_elements(const std::vector<FitElement>& elements)
{
	int index = 0;
	for(const auto& e : elements)
	{
		m_out << "C" << index << ": OHLCV:" << e.open << ":" <<
			e.high << ":" << e.low << ":" << e.close << ":" << e.volume << std::endl;
		index++;
	}
}

void TextReportBuilder::insert_text(const std::string& text)
{
	m_out << text << std::endl;
}

void TextReportBuilder::end_element()
{
}

void TextReportBuilder::end()
{
	m_out.close();
}

