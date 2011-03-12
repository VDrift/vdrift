#ifndef _WINDOW_H
#define _WINDOW_H

#include <string>
#include <ostream>

class SDL_Surface;

class WINDOW_SDL
{
private:
	// configuration variables, internal data
	int w, h;
	SDL_Surface * surface;
	bool initialized;
	unsigned int fsaa;
	
	void ChangeDisplay(const int width, const int height, const int bpp, const int dbpp, const bool fullscreen, 
			   unsigned int antialiasing, bool enableGL3, std::ostream & info_output, std::ostream & error_output);
	
public:
	WINDOW_SDL() : surface(NULL),initialized(false),fsaa(1) {}
	~WINDOW_SDL() {}
	
	void Init(const std::string & windowcaption,
				unsigned int resx, unsigned int resy, unsigned int bpp,
				unsigned int depthbpp, bool fullscreen,
				unsigned int antialiasing,
				bool enableGL3,
				std::ostream & info_output, std::ostream & error_output);
	void Deinit();
	void SwapBuffers(std::ostream & error_output);
	void Screenshot(std::string filename);
	int GetW() const {return w;}
	int GetH() const {return h;}
	float GetWHRatio() const {return (float)w/h;}
};

#endif

