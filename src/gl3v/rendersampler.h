#ifndef _RENDERSAMPLER
#define _RENDERSAMPLER

#include "glwrapper.h"
#include "renderstate.h"

class RenderSampler
{
	public:
		void apply(GLWrapper & gl) const;
		void addState(const RenderState & newState) {state.push_back(newState);}
		GLuint getHandle() const {return handle;}
		const std::vector <RenderState> & getStateVector() const {return state;} ///< used for debug printing

		RenderSampler(GLuint newTu, GLuint newHandle) : tu(newTu), handle(newHandle) {}

	private:
		std::vector <RenderState> state;
		GLuint tu;
		GLuint handle;
};

#endif