#ifndef _TEXTURE_INTERFACE_H
#define _TEXTURE_INTERFACE_H

#ifdef __APPLE__
#include <GLEW/glew.h>
#else
#include <GL/glew.h>
#endif

class GLSTATEMANAGER;
class GRAPHICS_FALLBACK;

/// an abstract base class for a simple texture interface
class TEXTURE_INTERFACE
{
	friend class GLSTATEMANAGER;
	friend class GRAPHICS_FALLBACK;
	public:
		virtual bool Loaded() const = 0;
		virtual void Activate() const = 0;
		virtual void Deactivate() const = 0;
		virtual bool IsRect() const {return false;}
		virtual unsigned int GetW() const = 0;
		virtual unsigned int GetH() const = 0;

	protected:
		virtual GLuint GetID() const = 0;
};

#endif // _TEXTURE_INTERFACE_H
