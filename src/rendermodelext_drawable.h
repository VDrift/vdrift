#ifndef _RENDERMODELEXTDRAWABLE
#define _RENDERMODELEXTDRAWABLE

#include "gl3v/rendermodelext.h"

class VERTEXARRAY;
class DRAWABLE;

class RenderModelExternalDrawable : public RenderModelExternal
{
	friend class DRAWABLE;
	public:
		void SetVertArray(const VERTEXARRAY* value) {vert_array = value;if (vert_array) enabled = true;}
		virtual void draw(GLWrapper & gl) const;

		RenderModelExternalDrawable() : vert_array(NULL) {}
		~RenderModelExternalDrawable() {}

	private:
		const VERTEXARRAY * vert_array;
        void SetLineSize(float size) { linesize = size; }

        float linesize;
};

#endif
