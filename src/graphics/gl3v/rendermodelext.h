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

#ifndef _RENDERMODELEXT
#define _RENDERMODELEXT

#include "rendertextureentry.h"
#include "renderuniformentry.h"
#include "glwrapper.h"
#include "rendermodelentry.h"
#include "rendercachevector.h"
#include "rendertexture.h"
#include "renderuniform.h"

class RenderPass;

/// This class is similar to RenderModelEntry, but contains textures and uniforms.
/// The idea is that these objects are stored externally from the renderer, and then arrays of pointers to them get passed into the renderer function at render time.
/// If a different geometry draw method is desired, the class can be derived from.
class RenderModelExt
{
	friend class RenderPass;
public:
	RenderModelExt();
	RenderModelExt(const RenderModelEntry & m);

	virtual ~RenderModelExt();
	virtual void draw(GLWrapper & gl) const;
	bool drawEnabled() const;
	void setVertexArrayObject(GLuint newVao, unsigned int newElementCount);

protected:
	GLuint vao;
	int elementCount;
	bool enabled;

	std::vector <RenderTextureEntry> textures;
	std::vector <RenderUniformEntry> uniforms;

	// These need to be called whenever the vectors above are changed.
	void clearTextureCache();
	void clearUniformCache();

private:
	RenderCacheVector <std::vector <RenderTexture> > perPassTextureCache;	// Indexed by pass ID.
	RenderCacheVector <std::vector <RenderUniform> > perPassUniformCache;	// Indexed by pass ID.
};

#endif
