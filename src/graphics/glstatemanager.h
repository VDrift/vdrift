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

#ifndef _GLSTATEMANAGER_H
#define _GLSTATEMANAGER_H

#include "texture_interface.h"
#include "glew.h"
#include <vector>
#include <cassert>

class GLSTATEMANAGER
{
public:
	GLSTATEMANAGER() :
		used(65536, false),
		state(65536),
		curtu(255),
		r(1),g(1),b(1),a(1),
		depthmask(true),
		alphamode(GL_NEVER),
		alphavalue(0),
		blendsource(GL_ZERO),
		blenddest(GL_ZERO),
		cullmode(GL_BACK),
		colormask(true),
		alphamask(true),
		framebuffer(0)
	{
	}

	inline void Enable(int stateid)
	{
		Set(stateid, true);
	}

	inline void Disable(int stateid)
	{
		Set(stateid, false);
	}

	void SetColor(float nr, float ng, float nb, float na)
	{
		if (r != nr || g != ng || b != nb || a != na)
		{
			r=nr;g=ng;b=nb;a=na;
			glColor4f(r,g,b,a);
		}
	}

	void SetDepthMask(bool newdepthmask)
	{
		if (newdepthmask != depthmask)
		{
			depthmask = newdepthmask;
			glDepthMask(depthmask ? 1 : 0);
		}
	}

	void SetColorMask(bool newcolormask, bool newalphamask)
	{
		if (newcolormask != colormask || newalphamask != alphamask)
		{
			colormask = newcolormask;
			alphamask = newalphamask;
			GLboolean val = colormask ? GL_TRUE : GL_FALSE;
			glColorMask(val, val, val, alphamask ? GL_TRUE : GL_FALSE);
		}
	}

	void SetAlphaFunc(GLenum mode, float value)
	{
		if (mode != alphamode || value != alphavalue)
		{
			alphamode = mode;
			alphavalue = value;
			glAlphaFunc(mode, value);
		}
	}

	void SetBlendFunc(GLenum s, GLenum d)
	{
		if (blendsource != s || blenddest != d)
		{
			blendsource = s;
			blenddest = d;
			glBlendFunc(s, d);
		}
	}

	void SetCullFace(GLenum mode)
	{
		if (mode != cullmode)
		{
			cullmode = mode;
			glCullFace(cullmode);
		}
	}

	void BindFramebuffer(GLuint fbid)
	{
		if (fbid != framebuffer)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, fbid);
			framebuffer = fbid;
		}
	}

	void BindTexture2D(unsigned tu, const TEXTURE_INTERFACE * texture)
	{
		if (tu >= tex2d.size())
			tex2d.resize(tu + 1, 0);

		GLuint & curid = tex2d[tu];
		GLuint id = texture ? texture->GetID() : 0;
		if (curid != id)
		{
			if (curtu != tu)
			{
				glActiveTexture(GL_TEXTURE0 + tu);
				curtu = tu;
			}

			Enable(GL_TEXTURE_2D); // fixme: should be done per texture unit

			glBindTexture(GL_TEXTURE_2D, id);
			curid = id;
		}
	}

private:
	std::vector <bool> used; //on modern compilers this should result in a lower memory usage bit_vector-type arrangement
	std::vector <bool> state;
	std::vector <GLuint> tex2d; //active textures
	unsigned curtu; //selected texture unit

	float r, g, b, a;
	bool depthmask;
	GLenum alphamode;
	float alphavalue;
	GLenum blendsource;
	GLenum blenddest;
	GLenum cullmode;
	bool colormask;
	bool alphamask;
	GLuint framebuffer;

	void Set(int stateid, bool newval)
	{
		assert(stateid <= 65535);

		if (used[stateid])
		{
			if (state[stateid] != newval)
			{
				state[stateid] = newval;
				if (newval)
					glEnable(stateid);
				else
					glDisable(stateid);
			}
		}
		else
		{
			used[stateid] = true;
			state[stateid] = newval;
			if (newval)
				glEnable(stateid);
			else
				glDisable(stateid);
		}
	}
};

#endif

