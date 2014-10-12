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

#ifndef _GRAPHICS_STATE_H
#define _GRAPHICS_STATE_H

#include "glcore.h"
#include <cassert>

class GraphicsState
{
public:
	GraphicsState() :
		tutgt(),
		tutex(),
		tuactive(0),
		fbread(0),
		fbdraw(0),
		vobject(0),
		vpwidth(0),
		vpheight(0),
		blendsource(GL_ZERO),
		blenddest(GL_ZERO),
		depthmode(GL_LESS),
		colormask(GL_TRUE),
		alphamask(GL_TRUE),
		depthmask(GL_TRUE),
		depthoffset(false),
		depthtest(false),
		blend(false),
		cull(false)
	{
		// ctor
	}

	// clear independent of currently set writemasks
	void ClearDrawBuffer(bool color, bool depth)
	{
		if (color && depth)
		{
			if (!colormask || !alphamask)
				glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			if (!depthmask)
				glDepthMask(GL_TRUE);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			if (!colormask || !alphamask)
				glColorMask(colormask, colormask, colormask, alphamask);
			if (!depthmask)
				glDepthMask(GL_FALSE);
		}
		else if (color)
		{
			if (!colormask || !alphamask)
				glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

			glClear(GL_COLOR_BUFFER_BIT);

			if (!colormask || !alphamask)
				glColorMask(colormask, colormask, colormask, alphamask);
		}
		else if (depth)
		{
			if (!depthmask)
				glDepthMask(GL_TRUE);

			glClear(GL_DEPTH_BUFFER_BIT);

			if (!depthmask)
				glDepthMask(GL_FALSE);
		}
	}

	void ColorMask(GLboolean writecolor, GLboolean writealpha)
	{
		if (writecolor != colormask || writealpha != alphamask)
		{
			colormask = writecolor;
			alphamask = writealpha;
			glColorMask(colormask, colormask, colormask, alphamask);
		}
	}

	void DepthTest(GLenum testdepth, GLboolean writedepth)
	{
		bool test = writedepth || testdepth != GL_ALWAYS;
		if (writedepth != depthmask)
		{
			depthmask = writedepth;
			glDepthMask(depthmask);
		}
		if (testdepth != depthmode)
		{
			depthmode = testdepth;
			glDepthFunc(depthmode);
		}
		if (test != depthtest)
		{
			depthtest = test;
			depthtest ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
		}
	}

	void DepthOffset(bool enable)
	{
		if (enable != depthoffset)
		{
			depthoffset = enable;
			depthoffset ? glEnable(GL_POLYGON_OFFSET_FILL) :
				glDisable(GL_POLYGON_OFFSET_FILL);
		}
	}

	void Blend(bool enable)
	{
		if (enable != blend)
		{
			blend = enable;
			blend ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
		}
	}

	void CullFace(bool enable)
	{
		if (enable != cull)
		{
			cull = enable;
			cull ? glEnable(GL_CULL_FACE) :	glDisable(GL_CULL_FACE);
		}
	}

	void BlendFunc(GLenum s, GLenum d)
	{
		if (blendsource != s || blenddest != d)
		{
			blendsource = s;
			blenddest = d;
			glBlendFunc(s, d);
		}
	}

	void ActiveTexture(GLuint texunit)
	{
		assert(texunit < 16);
		if (tuactive != texunit)
		{
			tuactive = texunit;
			glActiveTexture(GL_TEXTURE0 + tuactive);
		}
	}

	void BindTexture(GLuint texunit, GLenum target, GLuint texture)
	{
		assert(texunit < 16);
		if (tutgt[texunit] != target || tutex[texunit] != texture)
		{
			ActiveTexture(texunit);
			glBindTexture(target, texture);
			tutgt[texunit] = target;
			tutex[texunit] = texture;
		}
	}

	void BindFramebuffer(GLenum target, GLuint framebuffer)
	{
		if (target == GL_READ_FRAMEBUFFER && fbread != framebuffer)
		{
			fbread = framebuffer;
			glBindFramebuffer(target, framebuffer);
		}
		else if (target == GL_DRAW_FRAMEBUFFER && fbdraw != framebuffer)
		{
			fbdraw = framebuffer;
			glBindFramebuffer(target, framebuffer);
		}
		else if (target == GL_FRAMEBUFFER && (fbread != framebuffer || fbdraw != framebuffer))
		{
			fbread = framebuffer;
			fbdraw = framebuffer;
			glBindFramebuffer(target, framebuffer);
		}
	}

	void SetViewport(int width, int height)
	{
		if (width != vpwidth || height != vpheight)
		{
			vpwidth = width;
			vpheight = height;
			glViewport(0, 0, vpwidth, vpheight);
		}
	}

	GLuint & VertexObject()
	{
		return vobject;
	}

	// reset vao/vbo/ibo state and clear vobject
	void ResetVertexObject()
	{
		if (GLC_ARB_vertex_array_object)
			glBindVertexArray(0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		vobject = 0;
	}

private:
	//struct TexUnit {GLenum target; GLuint texture; bool enable};
	GLenum tutgt[16];   // texture unit target
	GLuint tutex[16];   // texture unit texture
	GLuint tuactive;
	GLuint fbread;
	GLuint fbdraw;
	GLuint vobject;		// currently bound vertex buffer/array object
	int vpwidth;
	int vpheight;
	GLenum blendsource;
	GLenum blenddest;
	GLenum depthmode;
	GLboolean colormask;
	GLboolean alphamask;
	GLboolean depthmask;
	bool depthoffset;
	bool depthtest;
	bool blend;
	bool cull;
};

#endif // _GRAPHICS_STATE_H

