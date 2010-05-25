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
		
		FBTEXTURE() : 
			fbtexture(0),
			renderbuffer_multisample(0),
			inited(false),
			attached(false),
			sizew(0),
			sizeh(0),
			texture_target(NORMAL),
			depth(false),
			alpha(false),
			mipmap(false),
			multisample(0),
			texture_attachment(GL_COLOR_ATTACHMENT0),
			texture_format1(0),
			cur_side(POSX)
			{}
		~FBTEXTURE() {DeInit();}
		void Init(GLSTATEMANAGER & glstate, int sizex, int sizey, TARGET target, bool newdepth, bool filternearest, bool newalpha, bool usemipmap, std::ostream & error_output, int newmultisample = 0);
		void DeInit();
		virtual void Activate() const;
		virtual void Deactivate() const;
		virtual bool Loaded() const {return inited;}
		//void Screenshot(GLSTATEMANAGER & glstate, const std::string & filename, std::ostream & error_output);
		int GetWidth() const {return sizew;}
		int GetHeight() const {return sizeh;}
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
		bool depth;
		bool alpha;
		bool mipmap;
		int multisample;
		int texture_attachment;
		int texture_format1;
		CUBE_SIDE cur_side;
};

#endif
