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

#ifndef _FBOBJECT_H
#define _FBOBJECT_H

#include "fbtexture.h"
#include "glstatemanager.h"
#include "glew.h"

#include <iostream>
#include <vector>

class FBOBJECT
{
	public:
		FBOBJECT() : framebuffer_object(0), renderbuffer_depth(0), inited(false), width(0), height(0) {}
		~FBOBJECT() {DeInit();}

		void Init(GLSTATEMANAGER & glstate, std::vector <FBTEXTURE*> newtextures, std::ostream & error_output, bool force_multisample_off = false);
		void DeInit();
		void Begin(GLSTATEMANAGER & glstate, std::ostream & error_output, float viewscale = 1.0);
		void End(GLSTATEMANAGER & glstate, std::ostream & error_output);
		bool Loaded() const {return inited;}
		void Screenshot(GLSTATEMANAGER & glstate, const std::string & filename, std::ostream & error_output);
		void SetCubeSide(FBTEXTURE::CUBE_SIDE side); ///< attach a specified cube side to the texture_attachment. for cube map FBOs only.
		int GetWidth() const {return width;}
		int GetHeight() const {return height;}
		bool IsCubemap() const;

	private:
		GLuint framebuffer_object;
		std::vector <FBOBJECT> multisample_dest_singlesample_framebuffer_object;
		GLuint renderbuffer_depth;
		bool inited;
		int width;
		int height;

		std::vector <FBTEXTURE*> textures;

		/// returns true if status is OK
		bool CheckStatus(std::ostream & error_output);

		FBTEXTURE & GetCubemapTexture();
};

#endif
