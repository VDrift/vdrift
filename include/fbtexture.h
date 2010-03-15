#ifndef _FBTEXTURE_H
#define _FBTEXTURE_H

#ifdef __APPLE__
#include <GLExtensionWrangler/glew.h>
#include <OpenGL/gl.h>
#else
#include <GL/glew.h>
#include <GL/gl.h>
#endif

#include "texture.h"

#include <iostream>

class FBTEXTURE_GL : public TEXTURE_INTERFACE
{
	public:
		enum TARGET
		{
			NORMAL = GL_TEXTURE_2D,
			RECTANGLE = GL_TEXTURE_RECTANGLE_ARB,
			CUBEMAP = GL_TEXTURE_CUBE_MAP
		};
	
	private:
		GLuint fbtexture;
		GLuint renderbuffer_depth;
		GLuint renderbuffer_multisample;
		GLuint framebuffer_object;
		FBTEXTURE_GL * single_sample_FBO_for_multisampling;
		bool loaded;
		bool inited;
		int sizew, sizeh;
		
		TARGET texture_target;
		bool depth;
		bool alpha;
		int multisample;

	public:
		FBTEXTURE_GL() : single_sample_FBO_for_multisampling(NULL),loaded(false),inited(false),sizew(0),sizeh(0),texture_target(NORMAL),depth(false),alpha(false),multisample(0) {}
		~FBTEXTURE_GL() {DeInit();}
		void Init(int sizex, int sizey, TARGET target, bool newdepth, bool filternearest, bool newalpha, std::ostream & error_output, int newmultisample = 0);
		void DeInit();
		void Begin(std::ostream & error_output, float viewscale = 1.0);
		void End(std::ostream & error_output);
		virtual void Activate() const;
		virtual void Deactivate() const;
		virtual bool Loaded() const {return loaded;}
		void Screenshot(const std::string & filename, std::ostream & error_output);
};

#endif
