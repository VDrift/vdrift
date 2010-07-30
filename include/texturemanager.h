#ifndef _TEXTUREMANAGER_H
#define _TEXTUREMANAGER_H

#include "texture.h"
#include "objectmanager.h"

struct TextureInfoLess
{
	bool operator()(const TEXTUREINFO & x, const TEXTUREINFO & y) const
	{
		return x.GetName() < y.GetName();
	}
};

typedef std::tr1::shared_ptr <TEXTURE> TEXTUREPTR;

class TEXTUREMANAGER : public OBJECTMANAGER <TEXTUREINFO, TEXTURE, TextureInfoLess>
{
public:
	TEXTUREMANAGER(std::ostream & error)
	: OBJECTMANAGER <TEXTUREINFO, TEXTURE, TextureInfoLess> (error)
	{
	}
};

#endif // _TEXTUREMANAGER_H
