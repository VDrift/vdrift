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
#include "glcore.h"

#include <iosfwd>
#include <string>
#include <vector>

class GraphicsState;

class FrameBufferObject
{
public:
	FrameBufferObject();

	~FrameBufferObject();

	void Init(
		GraphicsState & glstate,
		const std::vector <FrameBufferTexture*> & newtextures,
		std::ostream & error_output,
		bool force_multisample_off = false);

	void DeInit();

	void Begin(GraphicsState & glstate, std::ostream & error_output, float viewscale = 1.0);

	void End(GraphicsState & glstate, std::ostream & error_output);

	void Screenshot(GraphicsState & glstate, const std::string & filename, std::ostream & error_output);

	///< attach a specified cube side, for cube map FBOs only.
	void SetCubeSide(FrameBufferTexture::CubeSide side);

	int GetWidth() const {return width;}

	int GetHeight() const {return height;}

	bool Loaded() const {return inited;}

	bool IsCubemap() const;

private:
	GLuint framebuffer_object;
	bool inited;
	int width;
	int height;

	std::vector <FrameBufferTexture*> textures;
	std::vector <GLuint> multisample_renderbuffers;
	GLuint depth_renderbuffer;

	// if we're multisampling, the result is blit to a single sampled framebuffer
	// which is attached to the single sample fbtextures (color textures)
	FrameBufferObject * singlesample_framebuffer_object;

	/// returns true if status is OK
	bool CheckStatus(std::ostream & error_output);
};

#endif
