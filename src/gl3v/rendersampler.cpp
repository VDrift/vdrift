#include "rendersampler.h"

void RenderSampler::apply(GLWrapper & gl) const
{
	for (std::vector <RenderState>::const_iterator s = state.begin(); s != state.end(); s++)
	{
		s->applySampler(gl, handle);
	}
	
	gl.BindSampler(tu, handle);
}
