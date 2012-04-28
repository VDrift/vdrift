#ifndef SOUNDFILTER_H
#define SOUNDFILTER_H

#define MAX_FILTER_ORDER 5

class SOUNDFILTER
{
public:
	SOUNDFILTER();

	// reset filter
	void ClearState();

	// the coefficients are arrays like xcoeff[neworder+1], note ycoeff[0] is ignored
	void SetFilter(const int neworder, const float * xcoeff, const float * ycoeff);

	// apply filter
	void Filter(int * chan1, int * chan2, const int len);

	// yc0 is ignored
	void SetFilterOrder1(float xc0, float xc1, float yc1);

	// yc0 is ignored
	void SetFilterOrder0(float xc0);

private:
	int order;
	float xc[MAX_FILTER_ORDER+1];
	float yc[MAX_FILTER_ORDER+1];
	float statex[2][MAX_FILTER_ORDER+1];
	float statey[2][MAX_FILTER_ORDER+1];
};

#endif // SOUNDFILTER_H
