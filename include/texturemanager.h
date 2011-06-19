#ifndef _TEXTUREMANAGER_H
#define _TEXTUREMANAGER_H

#include "manager.h"

class TEXTURE;
class TEXTUREINFO;

class TEXTUREMANAGER : public MANAGER<TEXTURE>
{
public:
	TEXTUREMANAGER(std::ostream & error);

	bool Load(const std::string & path, const std::string & name, const TEXTUREINFO & info, std::tr1::shared_ptr<TEXTURE> & sptr);

	/// in general all textures on disk will be in the SRGB colorspace, so if the renderer wants to do
	/// gamma correct lighting, it will want all textures to be gamma corrected using the SRGB flag
	void SetSRGB(bool newsrgb) {srgb = newsrgb;}

private:
	bool srgb;
};

#endif // _TEXTUREMANAGER_H