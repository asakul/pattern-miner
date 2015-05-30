
#include "htmlreportbuilder.h"
#include "log.h"
#include "cairo/cairo.h"

using namespace boost::filesystem;

static const int CandleHeight = 150;

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
	create_directories(filename + "/images");
	m_main.open(filename + "/index.html", std::ios_base::out);
	if(!m_main.good())
		throw std::runtime_error("Unable to open index");

	m_imageCounter = 0;

	m_main << "<html>" << std::endl;
	m_main << "<head>" << std::endl;
	m_main << "<title>" << "Report" << "</title>" << std::endl;
	m_main << "</head>" << std::endl;
	m_main << "<body>" << std::endl;
}

void HtmlReportBuilder::begin_element(const std::string& title)
{
	m_main << "<h1 style=\"clear: both; font-size: 16px; font-family: serif; \">" << title << "</h1>" << std::endl;
}

void HtmlReportBuilder::insert_fit_elements(const std::vector<FitElement>& elements)
{
	m_imageCounter++;
	int index = 0;

	cairo_surface_t* surface;
	cairo_t* cr;
	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 100 * elements.size(), 200);
	cr = cairo_create(surface);
	
	double min = elements[0].low;
	double max = elements[0].high;
	double vmax = elements[0].volume;
	for(const auto& e : elements)
	{
		min = std::min(min, e.low);
		max = std::max(max, e.high);
		vmax = std::max(vmax, e.volume);
	}
	double k = (double)CandleHeight / (max - min);
	double kv = 40. / vmax;
	for(const auto& e : elements)
	{
		int center_x = 50 + index * 100;
		cairo_set_source_rgb(cr, 0, 0, 0);
		cairo_move_to(cr, center_x, CandleHeight - k * (e.high - min));
		cairo_line_to(cr, center_x, CandleHeight - k * (e.low - min));
		cairo_stroke(cr);
		if(e.close > e.open)
		{
			cairo_set_source_rgb(cr, 0, 1, 0);
			cairo_rectangle(cr, center_x - 20, CandleHeight - k * (e.close - min), 40, k * (e.close - e.open));
		}
		else
		{
			cairo_set_source_rgb(cr, 1, 0, 0);
			cairo_rectangle(cr, center_x - 20, CandleHeight - k * (e.open - min), 40, k * (e.open - e.close));
		}
		cairo_fill(cr);

		cairo_set_source_rgb(cr, 0, 0, 0);
		cairo_rectangle(cr, center_x - 5, 200 - kv * e.volume, 10, kv * e.volume);
		cairo_fill(cr);

		index++;
	}

	std::string imageFilename = m_root.string() + "/images/" + std::to_string(m_imageCounter) + ".png";
	cairo_surface_write_to_png(surface, imageFilename.c_str());

	m_main << "<img style=\"float:left; \" src=\"images/" << std::to_string(m_imageCounter) + ".png" << "\" />" << std::endl;
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

