#ifndef _RENDERMODELEXTDRAWABLE
#define _RENDERMODELEXTDRAWABLE

#include "gl3v/rendermodelext.h"

class VERTEXARRAY;

class RenderModelExternalDrawable : public RenderModelExternal
{
	public:
		virtual bool drawEnabled() const {return vert_array;}
		void SetVertArray(const VERTEXARRAY* value) {vert_array = value;}
		virtual void draw(GLWrapper & gl) const;
		
		RenderModelExternalDrawable() : vert_array(NULL) {}
		~RenderModelExternalDrawable() {}
		
	private:
		const VERTEXARRAY * vert_array;
};

#endif
