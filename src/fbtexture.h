#ifndef _FBTEXTURE_H
#define _FBTEXTURE_H

#include "texture_interface.h"
#include "glstatemanager.h"

#ifdef __APPLE__
#include <GLEW/glew.h>
#include <OpenGL/gl.h>
#else
#include <GL/glew.h>
#include <GL/gl.h>
#endif

#include <iostream>

class FBOBJECT;

class FBTEXTURE : public TEXTURE_INTERFACE
{
	friend class FBOBJECT;
	public:
		enum TARGET
		{
			NORMAL = GL_TEXTURE_2D,
			RECTANGLE = GL_TEXTURE_RECTANGLE,
			CUBEMAP = GL_TEXTURE_CUBE_MAP
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

		enum FORMAT
		{
			LUM8 = GL_LUMINANCE8,
			RGB8 = GL_RGB,
			RGBA8 = GL_RGBA,
			RGB16 = GL_RGB16,
			RGBA16 = GL_RGBA16,
			DEPTH24 = GL_DEPTH_COMPONENT24
		};

		FBTEXTURE() :
			fbtexture(0),
			renderbuffer_multisample(0),
			inited(false),
			attached(false),
			sizew(0),
			sizeh(0),
			texture_target(NORMAL),
			mipmap(false),
			multisample(0),
			texture_attachment(GL_COLOR_ATTACHMENT0),
			texture_format(RGB8),
			cur_side(POSX),
			depthcomparisonenabled(true)
			{}
		~FBTEXTURE() {DeInit();}
		void Init(int sizex, int sizey, TARGET target, FORMAT newformat, bool filternearest, bool usemipmap, std::ostream & error_output, int newmultisample = 0, bool newdepthcomparisonenabled = true);
		void DeInit();
		virtual void Activate() const;
		virtual void Deactivate() const;
		virtual bool Loaded() const {return inited;}
		//void Screenshot(GLSTATEMANAGER & glstate, const std::string & filename, std::ostream & error_output);
		virtual bool IsRect() const {return (texture_target == RECTANGLE);}
		virtual unsigned int GetW() const {return sizew;}
		virtual unsigned int GetH() const {return sizeh;}
		bool IsCubemap() const {return (texture_target == CUBEMAP);}

	protected:
		virtual GLuint GetID() const {return fbtexture;}

	private:
		GLuint fbtexture;
		GLuint renderbuffer_multisample;
		bool inited;
		bool attached;
		int sizew, sizeh;

		TARGET texture_target;
		bool mipmap;
		int multisample;
		int texture_attachment;
		FORMAT texture_format;
		CUBE_SIDE cur_side;
		bool depthcomparisonenabled;
};

#endif
