#ifndef _TEXTURELOADER_H
#define _TEXTURELOADER_H

#include "objectloader.h"
#include "textureinfo.h"

class TEXTURE;

class TextureLoader : public ObjectLoader<TEXTURE>, public TextureInfo
{
public:
	virtual ~TextureLoader() {};
	virtual TEXTURE * load(std::ostream & error) const;
	virtual const std::string & id() const;
};

#endif // _TEXTURELOADER_H
