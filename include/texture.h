#ifndef _TEXTURE_H
#define _TEXTURE_H

#ifdef __APPLE__
#include <GLExtensionWrangler/glew.h>
#else
#include <GL/glew.h>
#endif

#include "textureinfo.h"
#include "texture_interface.h"

class TEXTURE : public TEXTURE_INTERFACE
{
public:
	TEXTURE() {Init();}
	
	TEXTURE(const TEXTUREINFO & info, std::ostream & error) {Init(); Load(info, error);}
	
	virtual ~TEXTURE() {Unload();}
	
	virtual GLuint GetID() const {return tex_id;}
	
	virtual void Activate() const;
	
	virtual void Deactivate() const;

	virtual bool Loaded() const {return loaded;}
	
	bool Load(const TEXTUREINFO & info, std::ostream & error);
	
	void Unload();
	
	virtual unsigned int GetW() const {return w;}
	
	virtual unsigned int GetH() const {return h;}
	
	unsigned short int GetOriginalW() const {return origw;}
	
	unsigned short int GetOriginalH() const {return origh;}

	///scale factor from original size.  allows the user to determine
	///what the texture size scaling did to the texture dimensions
	float GetScale() const {return scale;}
	
private:
	TEXTUREINFO texture_info; // can be removed eventually
	GLuint tex_id;
	bool loaded;
	unsigned int w, h; ///< w and h are post-texture-size transform
	unsigned int origw, origh; ///< w and h are pre-texture-size transform
	float scale; ///< gets the amount of scaling applied by the texture-size transform, so the original w and h can be backed out
	bool alphachannel;
	bool cube;
	
	void Init()
	{
		loaded = false;
		w = 0;
		h = 0;
		origw = 0;
		origh = 0;
		scale = 1.0;
		alphachannel = false;
		cube = false;
	}
	
	bool LoadCube(const TEXTUREINFO & info, std::ostream & error);
	
	bool LoadCubeVerticalCross(const TEXTUREINFO & info, std::ostream & error);
};

#endif //_TEXTURE_H

