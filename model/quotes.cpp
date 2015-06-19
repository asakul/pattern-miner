
#include "quotes.h"
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <time.h>
#include "log.h"

#ifdef WIN32
#define timegm _mkgmtime
#endif

using namespace boost::algorithm;
using namespace boost;

template <typename T>
size_t getIndex(const std::vector<T>& v, const T& value)
{
	for(size_t i = 0; i < v.size(); i++)
	{
		if(trim_copy(v[i]) == value)
			return i;
	}
	throw std::runtime_error("Unable to find element: " + value);
}

static TimePoint parseTime(const std::string& date, const std::string& time)
{
	int year = lexical_cast<int>(date.substr(0, 4));
	int month = lexical_cast<int>(date.substr(4, 2));
	int day = lexical_cast<int>(date.substr(6, 2));
	int hour = lexical_cast<int>(time.substr(0, 2));
	int minute = lexical_cast<int>(time.substr(2, 2));
	int second = lexical_cast<int>(time.substr(4, 2));

	struct tm tm;
	tm.tm_year = year - 1900;
	tm.tm_mon = month - 1;
	tm.tm_mday = day;
	tm.tm_hour = hour;
	tm.tm_min = minute;
	tm.tm_sec = second;

	return TimePoint(timegm(&tm), 0);
}

Quotes::Quotes(const std::string& name) : m_name(name)
{
}

Quotes::~Quotes()
{
}

void Quotes::loadFromCsv(const std::string& filename)
{
	std::fstream in;
	in.open(filename.c_str(), std::ios_base::in);
	if(!in.good())
		throw std::runtime_error("Unable to open file: " + filename);

	std::string header;
	std::getline(in, header);

	std::vector<std::string> headerParts;

	split(headerParts, header, boost::is_any_of(","));

	size_t colTicker = getIndex(headerParts, std::string("<TICKER>"));
	size_t colDate = getIndex(headerParts, std::string("<DATE>"));
	size_t colTime = getIndex(headerParts, std::string("<TIME>"));
	size_t colOpen = getIndex(headerParts, std::string("<OPEN>"));
	size_t colHigh = getIndex(headerParts, std::string("<HIGH>"));
	size_t colLow = getIndex(headerParts, std::string("<LOW>"));
	size_t colClose = getIndex(headerParts, std::string("<CLOSE>"));
	size_t colVol = getIndex(headerParts, std::string("<VOL>"));

	std::string line;
	while(!in.eof())
	{
		std::getline(in, line);
		std::vector<std::string> lineParts;
		split(lineParts, line, boost::is_any_of(","));
		
		if(lineParts.size() < headerParts.size())
			break;

		if(m_name.empty())
		{
			m_name = lineParts[colTicker];
		}

		auto strDate = lineParts[colDate];
		auto strTime = lineParts[colTime];
		auto candleTime = parseTime(strDate, strTime);
		auto strOpen = lineParts[colOpen];
		auto strHigh = lineParts[colHigh];
		auto strLow = lineParts[colLow];
		auto strClose = lineParts[colClose];
		auto strVolume = lineParts[colVol];

		m_candles.emplace_back(lexical_cast<price_t>(strOpen),
				lexical_cast<price_t>(strHigh),
				lexical_cast<price_t>(strLow),
				lexical_cast<price_t>(strClose),
				lexical_cast<unsigned long>(trim_copy(strVolume)),
				candleTime);
	}
}

Candle Quotes::operator[](size_t index) const
{
	return m_candles[index];
}

Candle Quotes::at(size_t index) const
{
	return m_candles[index];
}

size_t Quotes::length() const
{
	return m_candles.size();
}

std::string Quotes::name() const
{
	return m_name;
}

