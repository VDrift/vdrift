#ifndef _TEXTUREMANAGER_H
#define _TEXTUREMANAGER_H

#include "texture.h"
#include "objectmanager.h"

struct TextureInfoHash
{
	std::size_t operator() (const TEXTUREINFO & info) const
	{
		std::tr1::hash<std::string> hashString;
		return hashString(info.GetName());
	}
};

struct TextureInfoEqual
{
	bool operator()(const TEXTUREINFO & x, const TEXTUREINFO & y) const
	{
		return x.GetName() == y.GetName();
	}
};

typedef std::tr1::shared_ptr <TEXTURE> TEXTUREPTR;

//typedef OBJECTMANAGER<TEXTUREINFO, TEXTURE, TextureInfoHash, TextureInfoEqual> TEXTUREMANAGER;
class TEXTUREMANAGER : public OBJECTMANAGER <TEXTUREINFO, TEXTURE, TextureInfoHash, TextureInfoEqual>
{
public:
	TEXTUREMANAGER(std::ostream & error)
	: OBJECTMANAGER <TEXTUREINFO, TEXTURE, TextureInfoHash, TextureInfoEqual> (error)
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
		OBJECTMANAGER<TEXTUREINFO, TEXTURE, TextureInfoHash, TextureInfoEqual>::DebugPrint(out);
		out << "Texture manager debug print end." << std::endl;
	}
};

#endif // _TEXTUREMANAGER_H
