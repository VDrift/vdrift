#ifndef _GAME_RENDER_H
#define _GAME_RENDER_H

#include "parallel_task.h"
#include "graphics.h"

#if defined(unix) || defined(__unix) || defined(__unix__)
#include <GL/glx.h>
#endif

#include <ostream>

class GAME_RENDER : public PARALLEL_TASK::TASK
{
	private:
		GRAPHICS_SDLGL * graphics;
		std::ostream * error_output;
		bool have_context;
		
#if defined(unix) || defined(__unix) || defined(__unix__)
		GLXContext ctx;
		Window draw;
		Display * disp;
	public:
		void SetGLXInfo(Display * ndisp, Window ndraw, GLXContext nctx)
		{
			disp = ndisp;
			draw = ndraw;
			ctx = nctx;
		}
#endif
	
	public:
		GAME_RENDER() : graphics(NULL),error_output(NULL),have_context(false) {}
		void SetGraphicsSubsystem(GRAPHICS_SDLGL & g) {graphics = &g;}
		void SetErrorOutput(std::ostream & err) {error_output = &err;}
		virtual void PARALLEL_TASK_EXECUTE();
		virtual void PARALLEL_TASK_SETUP();

		void SetHaveContext ( bool value )
		{
			have_context = value;
		}
	
		bool GetHaveContext() const
		{
			return have_context;
		}
	
	
};

#endif
