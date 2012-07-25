#ifndef _RENDER_INPUT_H
#define _RENDER_INPUT_H

#include <ostream>

class GLSTATEMANAGER;
struct FRUSTUM;

/// supported blend modes
namespace BLENDMODE
{
enum BLENDMODE
{
	DISABLED,
	ADD,
	ALPHABLEND,
	PREMULTIPLIED_ALPHA,
	ALPHATEST
};
}

/// purely abstract base class
class RENDER_INPUT
{
public:
	virtual void Render(GLSTATEMANAGER & glstate, std::ostream & error_output) = 0;

	/// get frustum from current mjp matrix
	static void ExtractFrustum(FRUSTUM & frustum);

	/// transform the shadow matrices in texture4-6 by the inverse of the camera transform in texture3
	static void PushShadowMatrices();

	static void PopShadowMatrices();
};

#endif //_RENDER_INPUT_H
