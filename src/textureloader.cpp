#include "textureloader.h"
#include "texture.h"

TEXTURE * TextureLoader::load(std::ostream & error) const
{
	TEXTURE * texture = new TEXTURE();
	if (texture->Load(*this, error))
	{
		return texture;
	}
	delete(texture);
	return NULL;
}

const std::string & TextureLoader::id() const
{
	return name;
}
