
#ifndef FITELEMENT_H_HCUQKJR8
#define FITELEMENT_H_HCUQKJR8

struct FitElement
{
	double open;
	double high;
	double low;
	double close;
	double volume;
};

struct ZigzagElement
{
	int time;
	double price;
	double volume;
	bool minimum;
};


#endif /* end of include guard: FITELEMENT_H_HCUQKJR8 */
