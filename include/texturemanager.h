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
	
	void DebugPrint(std::ostream & out)
	{
		int refcount = 0;
		out << "Texture manager debug print: " << std::endl;
		for(iterator it = objectmap.begin(); it != objectmap.end(); it++)
		{
			int references = it->second.use_count();
			refcount += references;
			out << "References: " << references;
			out << " Texture: " << it->first.GetName()  << std::endl;
		}
		out << "References count: " << refcount << std::endl;
		OBJECTMANAGER<TEXTUREINFO, TEXTURE, TextureInfoLess>::DebugPrint(out);
		out << "Texture manager debug print end." << std::endl;
	}
};

#endif // _TEXTUREMANAGER_H
