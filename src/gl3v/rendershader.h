#ifndef _RENDERSHADER
#define _RENDERSHADER

#include <set>
#include <string>

/// The bare minimum required to attach a shader to a shader program
struct RenderShader
{
	GLuint handle;

    // for debug only
    std::set <std::string> defines;
};

#endif
