#ifndef CANDLE_H
#define CANDLE_H

#include <ctime>

typedef double price_t;

struct TimePoint
{
	TimePoint(time_t s, int n) :
		sec(s), nsec(n)
	{
	}

	time_t sec;
	int nsec;
};

struct Candle
{
	Candle(price_t o, price_t h, price_t l, price_t c, unsigned int vol) : 
		open(o),
		high(h),
		low(l),
		close(c),
		volume(vol)
	{
	}

	price_t open;
	price_t high;
	price_t low;
	price_t close;
	unsigned int volume;
};

#endif
