#ifndef _RENDERMODELEXT
#define _RENDERMODELEXT

#include "keyed_container.h"
#include "stringidmap.h"
#include "rendertextureentry.h"
#include "renderuniformentry.h"
#include "glwrapper.h"
#include "rendermodelentry.h"

/// This class is similar to RenderModelEntry, but contains textures and uniforms.
/// The idea is that these objects are stored externally from the renderer, and then
/// arrays of pointers to them get passed into the renderer function at render time.
/// If a different geometry draw method is desired, the class can be derived from.
class RenderModelExternal
{
	public:
		std::vector <RenderTextureEntry> textures;
		std::vector <RenderUniformEntry> uniforms;
		
		GLuint vao;
		int elementCount;
		
		RenderModelExternal() : vao(0), elementCount(0) {}
		RenderModelExternal(const RenderModelEntry & m) : vao(m.vao), elementCount(m.elementCount) {}
		
		virtual void draw(GLWrapper & gl) const {gl.drawGeometry(vao, elementCount);}
		virtual bool drawEnabled() const {return elementCount > 0;}
};

#endif
