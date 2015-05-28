#ifndef QUOTES_H
#define QUOTES_H

#include <vector>
#include "candle.h"
#include <string>
#include <memory>

class Quotes
{
public:
	typedef std::shared_ptr<Quotes> Ptr;

	Quotes(const std::string& name = std::string());
	virtual ~Quotes();

	void loadFromCsv(const std::string& filename);
	
	Candle operator[](size_t index) const;
	Candle at(size_t index) const;

	size_t length() const;

	std::string name() const;
	
private:
	std::vector<Candle> m_candles;
	std::string m_name;

};

#endif
