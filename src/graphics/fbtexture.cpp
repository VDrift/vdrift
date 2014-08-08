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

#include "fbtexture.h"
#include "glutil.h"

#include <cassert>
#include <sstream>
#include <string>

FrameBufferTexture::FrameBufferTexture() :
	inited(false),
	mipmap(false),
	multisample(0),
	attachment(0),
	format(RGB8),
	cur_side(POSX),
	depthcomparisonenabled(true)
{
	// ctor
}

FrameBufferTexture::~FrameBufferTexture()
{
	DeInit();
}

void FrameBufferTexture::Init(
	int sizex, int sizey,
	Target newtarget, Format newformat,
	bool filternearest, bool usemipmap,
	std::ostream & error_output,
	int newmultisample,
	bool newdepthcomparisonenabled)
{
	assert(!attachment);

	CheckForOpenGLErrors("FBTEX init start", error_output);

	if (inited)
	{
		DeInit();

		CheckForOpenGLErrors("FBTEX deinit", error_output);
	}

	depthcomparisonenabled = newdepthcomparisonenabled;

	inited = true;

	target = newtarget;
	width = sizex;
	height = sizey;

	format = newformat;
	mipmap = usemipmap;

	multisample = newmultisample;

	//set texture info
	if (target == GL_TEXTURE_RECTANGLE)
	{
		assert(GLC_ARB_texture_rectangle);
	}

	int texture_format = GL_RGB;
	int data_format = GL_RGB;
	int data_type = GL_UNSIGNED_BYTE;

	switch (format)
	{
		case R8:
		texture_format = GL_R8;
		data_format = GL_RED;
		data_type = GL_UNSIGNED_BYTE;
		break;

		case RGB8:
		texture_format = GL_RGB;
		data_format = GL_RGB;
		data_type = GL_UNSIGNED_BYTE;
		break;

		case RGBA8:
		texture_format = GL_RGBA8;
		data_format = GL_RGBA;
		data_type = GL_UNSIGNED_BYTE;
		break;

		case RGB16:
		texture_format = GL_RGB16F;
		data_format = GL_RGB;
		data_type = GL_HALF_FLOAT;
		break;

		case RGBA16:
		texture_format = GL_RGBA16F;
		data_format = GL_RGBA;
		data_type = GL_HALF_FLOAT;
		break;

		case DEPTH24:
		texture_format = GL_DEPTH_COMPONENT24;
		data_format = GL_DEPTH_COMPONENT;
		data_type = GL_UNSIGNED_INT;
		break;

		default:
		assert(0);
		break;
	}

	//initialize the texture
	glGenTextures(1, &texid);
	glBindTexture(target, texid);

	CheckForOpenGLErrors("FBTEX texture generation and initial bind", error_output);

	if (target == CUBEMAP)
	{
		// generate storage for each of the six sides
		for (int i = 0; i < 6; i++)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, 0, texture_format, sizex, sizey, 0, data_format, data_type, NULL);
		}
	}
	else
	{
		glTexImage2D(target, 0, texture_format, sizex, sizey, 0, data_format, data_type, NULL);
	}

	CheckForOpenGLErrors("FBTEX texture storage initialization", error_output);

	glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	if (filternearest)
	{
		if (mipmap)
			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		else
			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	else
	{
		if (mipmap)
			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		else
			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	if (data_format == GL_DEPTH_COMPONENT)
	{
		glTexParameteri(target, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

		if (depthcomparisonenabled)
			glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
		else
			glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	}

	CheckForOpenGLErrors("FBTEX texture setup", error_output);

	if (mipmap)
	{
		glGenerateMipmap(target);

		CheckForOpenGLErrors("FBTEX initial mipmap generation", error_output);
	}

	glBindTexture(target, 0); // don't leave the texture bound

	CheckForOpenGLErrors("FBTEX texture unbinding", error_output);
}

void FrameBufferTexture::DeInit()
{
	if (texid > 0)
		glDeleteTextures(1, &texid);

	inited = false;
}
