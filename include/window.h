#ifndef _WINDOW_H
#define _WINDOW_H

#include "shader.h"
#include "mathvector.h"
#include "fbtexture.h"
#include "fbobject.h"
#include "scenenode.h"
#include "staticdrawables.h"
#include "matrix4.h"
#include "texture.h"
#include "reseatable_reference.h"
#include "aabb_space_partitioning.h"
#include "glstatemanager.h"
#include "graphics_renderers.h"
#include "graphics_config.h"

#include <SDL/SDL.h>

#include <string>
#include <ostream>
#include <map>
#include <list>
#include <vector>

class SCENENODE;

class WINDOW_SDL
{
private:
	// configuration variables, internal data
	int w, h;
	SDL_Surface * surface;
	bool initialized;
	unsigned int fsaa;
	
	void ChangeDisplay(const int width, const int height, const int bpp, const int dbpp, const bool fullscreen, 
			   unsigned int antialiasing, std::ostream & info_output, std::ostream & error_output);
	
public:
	WINDOW_SDL() : surface(NULL),initialized(false),fsaa(1) {}
	~WINDOW_SDL() {}
	
	void Init(const std::string & windowcaption,
				unsigned int resx, unsigned int resy, unsigned int bpp,
				unsigned int depthbpp, bool fullscreen,
				unsigned int antialiasing,
				std::ostream & info_output, std::ostream & error_output);
	void Deinit();
	void SwapBuffers(std::ostream & error_output);
	void Screenshot(std::string filename);
	int GetW() const {return w;}
	int GetH() const {return h;}
	float GetWHRatio() const {return (float)w/h;}
};

#endif

