#ifndef _FBTEXTURE_H
#define _FBTEXTURE_H

#ifdef __APPLE__
#include <GLExtensionWrangler/glew.h>
#include <OpenGL/gl.h>
#else
#include <GL/glew.h>
#include <GL/gl.h>
#endif

#include <iostream>

class FBTEXTURE_GL
{
	private:
		GLuint fbtexture;
		GLuint renderbuffer_depth;
		GLuint framebuffer_object;
		bool loaded;
		bool inited;
		int sizew, sizeh;
		bool rectangle;
		bool depth;
		bool alpha;

	public:
		FBTEXTURE_GL() : loaded(false),inited(false),sizew(0),sizeh(0),rectangle(false),depth(false),alpha(false) {}
		~FBTEXTURE_GL() {DeInit();}
		void Init(int sizex, int sizey, bool rect, bool newdepth, bool filternearest, bool newalpha, std::ostream & error_output);
		void DeInit();
		void Begin(std::ostream & error_output, float viewscale = 1.0);
		void End(std::ostream & error_output);
		void Activate() const;
};

#endif
