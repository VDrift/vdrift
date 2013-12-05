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

#include "glew.h"
#include <cassert>

// allow to run with fbo ext on older gpus (experimental compile time option)
#ifdef FBOEXT

#undef glGenFramebuffers
#undef glBindFramebuffer
#undef glGenRenderbuffers
#undef glBindRenderbuffer
#undef glRenderbufferStorage
#undef glFramebufferRenderbuffer
#undef glFramebufferTexture2D
#undef glCheckFramebufferStatus
#undef glDeleteFramebuffers
#undef glDeleteRenderbuffers
#undef glBlitFramebuffer

#define glGenFramebuffers GLEW_GET_FUN(__glewGenFramebuffersEXT)
#define glBindFramebuffer GLEW_GET_FUN(__glewBindFramebufferEXT)
#define glGenRenderbuffers GLEW_GET_FUN(__glewGenRenderbuffersEXT)
#define glBindRenderbuffer GLEW_GET_FUN(__glewBindRenderbufferEXT)
#define glRenderbufferStorage GLEW_GET_FUN(__glewRenderbufferStorageEXT)
#define glFramebufferRenderbuffer GLEW_GET_FUN(__glewFramebufferRenderbufferEXT)
#define glFramebufferTexture2D GLEW_GET_FUN(__glewFramebufferTexture2DEXT)
#define glCheckFramebufferStatus GLEW_GET_FUN(__glewCheckFramebufferStatusEXT)
#define glDeleteFramebuffers GLEW_GET_FUN(__glewDeleteFramebuffersEXT)
#define glDeleteRenderbuffers GLEW_GET_FUN(__glewDeleteRenderbuffersEXT)
#define glBlitFramebuffer GLEW_GET_FUN(__glewBlitFramebufferEXT)

#endif // FBOEXT

class GraphicsState
{
public:
	GraphicsState() :
		tutgt(),
		tutex(),
		tuactive(0),
		fbread(0),
		fbdraw(0),
		vpwidth(0),
		vpheight(0),
		r(1),g(1),b(1),a(1),
		alphavalue(0),
		alphamode(GL_NEVER),
		blendsource(GL_ZERO),
		blenddest(GL_ZERO),
		cullmode(GL_BACK),
		depthmode(GL_LESS),
		colormask(GL_TRUE),
		alphamask(GL_TRUE),
		depthmask(GL_TRUE),
		depthoffset(false),
		depthtest(false),
		alphatest(false),
		samplea2c(false),
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

	void SetColor(float nr, float ng, float nb, float na)
	{
		if (r != nr || g != ng || b != nb || a != na)
		{
			r=nr;g=ng;b=nb;a=na;
			glColor4f(r,g,b,a);
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

	void AlphaTest(bool enable, bool alpha_to_coverage = false)
	{
		if (enable != alphatest)
		{
			alphatest = enable;
			alphatest ? glEnable(GL_ALPHA_TEST) : glDisable(GL_ALPHA_TEST);
		}

		if (alpha_to_coverage != samplea2c)
		{
			samplea2c = alpha_to_coverage;
			samplea2c ? glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE) :
				glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
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

	void AlphaFunc(GLenum mode, float value)
	{
		if (mode != alphamode || value != alphavalue)
		{
			alphamode = mode;
			alphavalue = value;
			glAlphaFunc(mode, value);
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

	void CullFaceMode(GLenum mode)
	{
		if (mode != cullmode)
		{
			cullmode = mode;
			glCullFace(cullmode);
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

			if (!tutex[texunit])
				glEnable(target);

			glBindTexture(target, texture);

			if (!texture)
				glDisable(target);

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

private:
	//struct TexUnit {GLenum target; GLuint texture; bool enable};
	GLenum tutgt[16];   // texture unit target
	GLuint tutex[16];   // texture unit texture
	GLuint tuactive;
	GLuint fbread;
	GLuint fbdraw;
	int vpwidth;
	int vpheight;
	float r, g, b, a;
	float alphavalue;
	GLenum alphamode;
	GLenum blendsource;
	GLenum blenddest;
	GLenum cullmode;
	GLenum depthmode;
	GLboolean colormask;
	GLboolean alphamask;
	GLboolean depthmask;
	bool depthoffset;
	bool depthtest;
	bool alphatest;
	bool samplea2c;
	bool blend;
	bool cull;
};

#endif // _GRAPHICS_STATE_H

