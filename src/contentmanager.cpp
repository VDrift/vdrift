#include "contentmanager.h"
#include "texture.h"
#include "model_joe03.h"
#include "soundbuffer.h"

#include <fstream>

template <class key, class value>
static void printLeak(const std::map<key, value>& cache, std::ostream& out)
{
	typename std::map<key, value>::const_iterator i;
	for (i = cache.begin(); i != cache.end(); ++i)
	{
		out << "Leaked: " << i->first << std::endl;
	}
}

ContentManager::ContentManager(std::ostream & error) :
	sound_info(0, 0, 0, 0),
	texture_size(TEXTUREINFO::LARGE),
	texture_srgb(false),
	model_vbo(false),
	error(error)
{
	//ctor
}

ContentManager::~ContentManager()
{
	sweep();
	printLeak(textures, error);
	printLeak(models, error);
	printLeak(sounds, error);
}

void ContentManager::addSharedPath(const std::string & path)
{
	sharedpaths.push_back(path);
}

void ContentManager::addPath(const std::string & path)
{
	basepaths.push_back(path);
}

void ContentManager::setSound(const SOUNDINFO& info)
{
	sound_info = info;
}

void ContentManager::setTexSize(int value)
{
	texture_size = TEXTUREINFO::Size(value);
}

void ContentManager::setSRGB(bool value)
{
	texture_srgb = value;
}

void ContentManager::setVBO(bool value)
{
	model_vbo = value;
}

void ContentManager::sweep(std::ostream & info)
{
	sweep();
	info << "Textures: " << textures.size() << "\n";
	info << "Models: " << models.size() << "\n";
	info << "Sounds: " << sounds.size() << std::endl;
}

void ContentManager::sweep()
{
	textures.sweep();
	models.sweep();
	sounds.sweep();
}

template <>
bool ContentManager::load(
	std::tr1::shared_ptr<TEXTURE>& sptr,
	const std::string& abspath,
	const TEXTUREINFO& info)
{
	if (info.data || std::ifstream(abspath.c_str()))
	{
		TEXTUREINFO info_temp = info;
		info_temp.srgb = texture_srgb;
		info_temp.maxsize = texture_size;
		std::tr1::shared_ptr<TEXTURE> temp(new TEXTURE());
		if (temp->Load(abspath, info_temp, error))
		{
			sptr = temp;
			return true;
		}
	}
	return false;
}

template <>
bool ContentManager::load(
	std::tr1::shared_ptr<SOUNDBUFFER>& sptr,
	const std::string& abspath,
	const empty&)
{
	std::string filepath = abspath + ".ogg";
	if (!std::ifstream(filepath.c_str()))
	{
		filepath = abspath + ".wav";
	}
	if (std::ifstream(filepath.c_str()))
	{
		std::tr1::shared_ptr<SOUNDBUFFER> temp(new SOUNDBUFFER());
		if (temp->Load(filepath, sound_info, error))
		{
			sptr = temp;
			return true;
		}
	}
	return false;
}

template <>
bool ContentManager::load(
	std::tr1::shared_ptr<MODEL>& sptr,
	const std::string& abspath,
	const empty&)
{
	if (std::ifstream(abspath.c_str()))
	{
		std::tr1::shared_ptr<MODEL_JOE03> temp(new MODEL_JOE03());
		if (temp->Load(abspath, error, !model_vbo))
		{
			sptr = temp;
			return true;
		}
	}
	return false;
}

template <>
bool ContentManager::load(
	std::tr1::shared_ptr<MODEL>& sptr,
	const std::string& abspath,
	const JOEPACK& pack)
{
	std::tr1::shared_ptr<MODEL_JOE03> temp(new MODEL_JOE03());
	std::string name = abspath.substr(abspath.rfind('/')+1); // doesn't look very efficient
	if (temp->Load(name, error, !model_vbo, &pack))
	{
		sptr = temp;
		return true;
	}
	return false;
}

template <>
bool ContentManager::load(
	std::tr1::shared_ptr<MODEL>& sptr,
	const std::string& abspath,
	const VERTEXARRAY& varray)
{
	std::tr1::shared_ptr<MODEL> temp(new MODEL());
	if (temp->Load(varray, error, !model_vbo))
	{
		sptr = temp;
		return true;
	}
	return false;
}
