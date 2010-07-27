#ifndef _TEXTURECUBE_H
#define _TEXTURECUBE_H

#include "texture_interface.h"

#include <string>
#include <ostream>

struct TextureCubeInfo
{
	TextureCubeInfo() : mipmap(true), verticalcross(false) { }

	std::string name;
	bool mipmap;
	bool verticalcross;
};

class TextureCube : public TEXTURE_INTERFACE
{
public:
	TextureCube();

	~TextureCube();

	bool Load(const TextureCubeInfo & info, std::ostream & error);

	virtual GLuint GetID() const {return tex_id;}

	virtual void Activate() const;

	virtual void Deactivate() const;

	virtual bool Loaded() const {return loaded;}

	virtual unsigned int GetW() const {return w;}

	virtual unsigned int GetH() const {return h;}

private:
	GLuint tex_id;
	bool loaded;
	unsigned int w, h;

	bool LoadVerticalCross(const TextureCubeInfo & info, std::ostream & error);
};

#endif // _TEXTURECUBE_H
