#ifndef SOUNDFILTER_H
#define SOUNDFILTER_H
/*
#define MAX_FILTER_ORDER 5

class SOUNDFILTER
{
public:
	SOUNDFILTER();
	
	void ClearState();
	
	//the coefficients are arrays like xcoeff[neworder+1], note ycoeff[0] is ignored
	void SetFilter(const int neworder, const float * xcoeff, const float * ycoeff);
	
	void Filter(int * chan1, int * chan2, const int len);
	
	//yc0 is ignored
	void SetFilterOrder1(float xc0, float xc1, float yc1); 
	
	//yc0 is ignored
	void SetFilterOrder0(float xc0); 

private:
	int order;
	float xc[MAX_FILTER_ORDER+1];
	float yc[MAX_FILTER_ORDER+1];
	float statex[2][MAX_FILTER_ORDER+1];
	float statey[2][MAX_FILTER_ORDER+1];
};
*/

#define MAX_FILTER_ORDER 5

class SOUNDFILTER
{
private:
	int order;
	float xc[MAX_FILTER_ORDER+1];
	float yc[MAX_FILTER_ORDER+1];
	float statex[2][MAX_FILTER_ORDER+1];
	float statey[2][MAX_FILTER_ORDER+1];

public:
	SOUNDFILTER() : order(0) {ClearState();}
	
	void ClearState() {for (int c = 0; c < 2; c++) for (int i = 0; i < MAX_FILTER_ORDER; i++) {statex[c][i] = 0.0;statey[c][i] = 0.0;}}
	void SetFilter(const int neworder, const float * xcoeff, const float * ycoeff); //the coefficients are arrays like xcoeff[neworder+1], note ycoeff[0] is ignored
	void Filter(int * chan1, int * chan2, const int len);
	void SetFilterOrder1(float xc0, float xc1, float yc1); //yc0 is ignored
	void SetFilterOrder0(float xc0); //yc0 is ignored
};

#endif // SOUNDFILTER_H
