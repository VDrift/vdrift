/************************************************************************/
/*                                                                      */
/* This file is part of VDrift.                                         */
/*                                                                      */
/* VDrift is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* VDrift is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with VDrift.  If not, see <http://www.gnu.org/licenses/>.      */
/*                                                                      */
/************************************************************************/

#ifndef _FBTEXTURE_H
#define _FBTEXTURE_H

#include "texture_interface.h"
#include "glstatemanager.h"
#include "glew.h"

#include <iostream>

class FrameBufferObject;

class FrameBufferTexture : public TextureInterface
{
	friend class FrameBufferObject;
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

		FrameBufferTexture() :
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
		~FrameBufferTexture() {DeInit();}
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
