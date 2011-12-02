#ifndef _TEXTURE_H
#define _TEXTURE_H

#include "texture_interface.h"
#include "textureinfo.h"
#include <string>

class TEXTURE : public TEXTURE_INTERFACE
{
public:
	TEXTURE():
		id(0),
		w(0),
		h(0),
		origw(0),
		origh(0),
		scale(1.0),
		alpha(false),
		cube(false)
	{
		// ctor
	}

	virtual ~TEXTURE() {Unload();}

	virtual GLuint GetID() const {return id;}

	virtual void Activate() const;

	virtual void Deactivate() const;

	virtual bool Loaded() const {return id;}

	bool Load(const std::string & path, const TEXTUREINFO & info, std::ostream & error);

	void Unload();

	virtual unsigned int GetW() const {return w;}

	virtual unsigned int GetH() const {return h;}

	unsigned short int GetOriginalW() const {return origw;}

	unsigned short int GetOriginalH() const {return origh;}

	///scale factor from original size.  allows the user to determine
	///what the texture size scaling did to the texture dimensions
	float GetScale() const {return scale;}

	bool IsCube() const {return cube;}

private:
	GLuint id;
	unsigned int w, h; ///< w and h are post-texture-size transform
	unsigned int origw, origh; ///< w and h are pre-texture-size transform
	float scale; ///< gets the amount of scaling applied by the texture-size transform, so the original w and h can be backed out
	bool alpha;
	bool cube;

	bool LoadCube(const std::string & path, const TEXTUREINFO & info, std::ostream & error);

	bool LoadCubeVerticalCross(const std::string & path, const TEXTUREINFO & info, std::ostream & error);
};

#endif //_TEXTURE_H

