#include "game_render.h"

#include <SDL/SDL.h>

#if defined(WIN32) || defined(_WIN32) || defined (__WIN32) || defined(__WIN32__) \
	|| defined (_WIN64) || defined(__CYGWIN__) || defined(__MINGW32__)

#elif defined(__APPLE__)

#elif defined(unix) || defined(__unix) || defined(__unix__)
#include <GL/glx.h>
#include <SDL/SDL_syswm.h>
#endif

#include <iostream>
#include <cassert>

void GAME_RENDER::PARALLEL_TASK_SETUP()
{
		//we need the rendering context assigned to our thread
	if (!have_context)
	{
		//std::cout << "Acquiring context..." << std::endl;
#if defined(WIN32) || defined(_WIN32) || defined (__WIN32) || defined(__WIN32__) \
		|| defined (_WIN64) || defined(__CYGWIN__) || defined(__MINGW32__)

#elif defined(__APPLE__)

#elif defined(unix) || defined(__unix) || defined(__unix__)
		SDL_SysWMinfo info;
		SDL_VERSION(&info.version);
		SDL_GetWMInfo(&info);
		info.info.x11.lock_func();
		
		int attrListDbl[] = { GLX_RGBA, GLX_DOUBLEBUFFER, 
			GLX_RED_SIZE, 4, 
		GLX_GREEN_SIZE, 4, 
		GLX_BLUE_SIZE, 4, 
		GLX_DEPTH_SIZE, 16,
		None };
		XVisualInfo*vi = glXChooseVisual(disp, 0, attrListDbl);
		GLXContext ctx2 = glXCreateContext(disp, vi, ctx, GL_TRUE);
				
				//glXCopyContext(disp, ctx, ctx2, GL_ALL_ATTRIB_BITS);
				
				//std::cout << info.info.x11.display << ", " << info.info.x11.gfxdisplay << ", " << (void*)info.info.x11.window << std::endl;
				//std::cout << glXGetCurrentDisplay() << ", " << glXGetCurrentDrawable() << ", " << glXGetCurrentContext() << std::endl;
				//glXMakeCurrent(info.info.x11.display, info.info.x11.window, ctx);
				//std::cout << disp << ", " << (void*)draw << ", " << ctx << std::endl;
		glXMakeCurrent(disp, draw, ctx2);
		info.info.x11.unlock_func();
#endif
		have_context = true;
		
		//std::cout << "Acquired context" << std::endl;
	}
}

void GAME_RENDER::PARALLEL_TASK_EXECUTE()
{
	assert(graphics);
	assert(error_output);
	
	graphics->BeginScene(*error_output);
	graphics->DrawScene(*error_output);
	graphics->EndScene(*error_output);
}
