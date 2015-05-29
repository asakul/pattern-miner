
#include "htmlreportbuilder.h"

using namespace boost::filesystem;

HtmlReportBuilder::HtmlReportBuilder()
{
}

HtmlReportBuilder::~HtmlReportBuilder()
{
}

void HtmlReportBuilder::start(const std::string& filename,
		const TimePoint& start_time, const TimePoint& end_time,
		const std::list<std::string>& tickers)
{
	m_root = path(filename);
	create_directories(m_root);
	m_main.open(filename + "/index.html", std::ios_base::out);
	if(!m_main.good())
		throw std::runtime_error("Unable to open index");

	m_main << "<html>" << std::endl;
	m_main << "<head>" << std::endl;
	m_main << "<title>" << "Report" << "</title>" << std::endl;
	m_main << "</head>" << std::endl;
	m_main << "<body>" << std::endl;
}

void HtmlReportBuilder::begin_element(const std::string& title)
{
	m_main << "<h1 style=\"font-size: 16px; font-family: serif; \">" << title << "</h1>" << std::endl;
}

void HtmlReportBuilder::insert_fit_elements(const std::vector<FitElement>& elements)
{
	int index = 0;
	for(const auto& e : elements)
	{
		m_main << "C" << index << ": OHLCV:" << e.open << ":" <<
			e.high << ":" << e.low << ":" << e.close << ":" << e.volume << "<br />" << std::endl;
		index++;
	}
}

void HtmlReportBuilder::insert_text(const std::string& text)
{
	m_main << text << "<br />" << std::endl;
}

void HtmlReportBuilder::end_element()
{
}

void HtmlReportBuilder::end()
{
	m_main << "</body>" << std::endl;
	m_main << "</html>" << std::endl;
}

