#ifndef _TEXTURE_H
#define _TEXTURE_H

#include <string>
#include <iostream>

#ifdef __APPLE__
#include <GLExtensionWrangler/glew.h>
#else
#include <GL/glew.h>
#endif

#include "textureinfo.h"
#include "texture_interface.h"

class TEXTURE_GL : public TEXTURE_INTERFACE
{
public:
	TEXTURE_GL()
	{
		Init();
	}
	
	~TEXTURE_GL()
	{
		Unload();
	}
	
	virtual bool Loaded() const
	{
		return loaded;
	}

	virtual void Activate() const;
	
	virtual void Deactivate() const;

	void SetInfo(const TEXTUREINFO & texinfo)
	{
		texture_info.CopyFrom(texinfo);
	}
	
	bool Load(const TEXTUREINFO & texinfo, std::ostream & error_output, const std::string & texsize)
	{
		SetInfo(texinfo);
		return Load(error_output, texsize);
	}
	
	void Unload();
	
	unsigned short int GetW() const
	{
		return w;
	}
	
	unsigned short int GetH() const
	{
		return h;
	}
	
	unsigned short int GetOriginalW() const
	{
		return origw;
	}
	
	unsigned short int GetOriginalH() const
	{
		return origh;
	}
	
	bool IsEqualTo(const TEXTURE_GL & othertex) const
	{
		return IsEqualTo(othertex.GetTextureInfo());
	}
	
	bool IsEqualTo(const TEXTUREINFO & texinfo) const;
	
	const TEXTUREINFO & GetTextureInfo() const
	{
		return texture_info;
	}

	///scale factor from original size.  allows the user to determine
	///what the texture size scaling did to the texture dimensions
	float GetScale() const
	{
		return scale;
	}
	
private:
	TEXTUREINFO texture_info;
	GLuint tex_id;
	bool loaded;
	unsigned int w, h; ///< w and h are post-texture-size transform
	unsigned int origw, origh; ///< w and h are pre-texture-size transform
	float scale; ///< gets the amount of scaling applied by the texture-size transform, so the original w and h can be backed out
	bool alphachannel;
	
	void Init()
	{
		loaded = false;
		w = 0;
		h = 0;
		origw = 0;
		origh = 0;
		scale = 1.0;
		alphachannel = false;
	}
	
	bool IsPowerOfTwo(int x)
	{
	    return ((x != 0) && !(x & (x - 1)));
	}
	
	bool LoadCube(std::ostream & error_output);
	
	bool LoadCubeVerticalCross(std::ostream & error_output);
	
	bool Load(std::ostream & error_output, const std::string & texsize);
};

#endif //_TEXTURE_H

