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
#include "glstatemanager.h"

#include <iostream>

class FBTEXTURE : public TEXTURE_INTERFACE
{
	public:
		enum TARGET
		{
			NORMAL = GL_TEXTURE_2D,
			RECTANGLE = GL_TEXTURE_RECTANGLE_ARB,
			CUBEMAP = GL_TEXTURE_CUBE_MAP_ARB
		};
		
		enum CUBE_SIDE
		{
			POSX = GL_TEXTURE_CUBE_MAP_POSITIVE_X,
			NEGX = GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
			POSY = GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
			NEGY = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
			POSZ = GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
			NEGZ = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
		};
		
	protected:
		virtual GLuint GetID() const {return single_sample_FBO_for_multisampling ? single_sample_FBO_for_multisampling->fbtexture : fbtexture;}
		
	private:
		GLuint fbtexture;
		GLuint renderbuffer_depth;
		GLuint renderbuffer_multisample;
		GLuint framebuffer_object;
		FBTEXTURE * single_sample_FBO_for_multisampling;
		bool loaded;
		bool inited;
		int sizew, sizeh;
		
		TARGET texture_target;
		bool depth;
		bool alpha;
		bool mipmap;
		int multisample;
		int texture_attachment;
		CUBE_SIDE cur_side;
		
		/// returns true if statis is good
		bool CheckStatus(std::ostream & error_output);

	public:
		FBTEXTURE() : single_sample_FBO_for_multisampling(NULL),loaded(false),inited(false),sizew(0),sizeh(0),
			texture_target(NORMAL),depth(false),alpha(false),mipmap(false),multisample(0),
			texture_attachment(GL_COLOR_ATTACHMENT0_EXT),cur_side(POSX) {}
		~FBTEXTURE() {DeInit();}
		void Init(int sizex, int sizey, TARGET target, bool newdepth, bool filternearest, bool newalpha, bool usemipmap, std::ostream & error_output, int newmultisample = 0);
		void DeInit();
		void Begin(GLSTATEMANAGER & glstate, std::ostream & error_output, float viewscale = 1.0);
		void End(std::ostream & error_output);
		virtual void Activate() const;
		virtual void Deactivate() const;
		virtual bool Loaded() const {return inited;}
		void Screenshot(GLSTATEMANAGER & glstate, const std::string & filename, std::ostream & error_output);
		void SetCubeSide(CUBE_SIDE side); ///< attach a specified cube side to the texture_attachment. for cube map FBOs only.
		int GetWidth() const {return sizew;}
		int GetHeight() const {return sizeh;}
		bool IsCubemap() const {return (texture_target == CUBEMAP);}
};

#endif
