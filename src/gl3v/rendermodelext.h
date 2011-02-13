#ifndef _RENDERMODELEXT
#define _RENDERMODELEXT

#include "keyed_container.h"
#include "stringidmap.h"
#include "rendertextureentry.h"
#include "renderuniformentry.h"
#include "glwrapper.h"
#include "rendermodelentry.h"
#include "rendercachevector.h"
#include "rendertexture.h"
#include "renderuniform.h"

class RenderPass;

/// This class is similar to RenderModelEntry, but contains textures and uniforms.
/// The idea is that these objects are stored externally from the renderer, and then
/// arrays of pointers to them get passed into the renderer function at render time.
/// If a different geometry draw method is desired, the class can be derived from.
class RenderModelExternal
{
	friend class RenderPass;
	public:
		RenderModelExternal() : vao(0), elementCount(0), enabled(false) {}
		RenderModelExternal(const RenderModelEntry & m) : vao(m.vao), elementCount(m.elementCount)
		{
			if (elementCount > 0)
				enabled = true;
		}
		
		virtual ~RenderModelExternal() {}
		virtual void draw(GLWrapper & gl) const {gl.drawGeometry(vao, elementCount);}
		bool drawEnabled() const {return enabled;}
		void setVertexArrayObject(GLuint newVao, unsigned int newElementCount)
		{
			vao = newVao;
			elementCount = newElementCount;
			if (elementCount > 0)
				enabled = true;
		}
		
	protected:
		GLuint vao;
		int elementCount;
		bool enabled;
		
		std::vector <RenderTextureEntry> textures;
		std::vector <RenderUniformEntry> uniforms;
		
		// these need to be called whenever the vectors above are changed
		void clearTextureCache() {perPassTextureCache.clear();}
		void clearUniformCache() {perPassUniformCache.clear();}
		
	private:
		RenderCacheVector <std::vector <RenderTexture> > perPassTextureCache; // indexed by pass ID
		RenderCacheVector <std::vector <RenderUniform> > perPassUniformCache; // indexed by pass ID
};

#endif
