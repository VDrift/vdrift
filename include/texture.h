#ifndef _TEXTURE_H
#define _TEXTURE_H

#include "texture_interface.h"
#include "textureinfo.h"

class TEXTURE : public TEXTURE_INTERFACE
{
public:
	TEXTURE();
	
	~TEXTURE();
	
	virtual bool Load(const TextureInfo & info, std::ostream & error);
	
	virtual GLuint GetID() const {return tex_id;}
	
	virtual void Activate() const;
	
	virtual void Deactivate() const;

	virtual bool Loaded() const {return loaded;}
	
	virtual unsigned int GetW() const {return w;}
	
	virtual unsigned int GetH() const {return h;}
	
	unsigned short int GetOriginalW() const {return origw;}
	
	unsigned short int GetOriginalH() const {return origh;}

	/// allows the user to determine what the texture size scaling did to the texture dimensions
	float GetScale() const {return scale;}

private:
	GLuint tex_id;
	bool loaded;
	unsigned int w, h; ///< w and h are post-texture-size transform
	unsigned int origw, origh; ///< w and h are pre-texture-size transform
	float scale; ///< gets the amount of scaling applied by the texture-size transform, so the original w and h can be backed out
	bool alphachannel;

	void Unload();
};

#endif //_TEXTURE_H

