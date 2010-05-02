#ifndef _TEXTURE_INTERFACE_H
#define _TEXTURE_INTERFACE_H

class GLSTATEMANAGER;

/// an abstract base class for a simple texture interface
class TEXTURE_INTERFACE
{
	friend class GLSTATEMANAGER;
	public:
		virtual bool Loaded() const = 0;
		virtual void Activate() const = 0;
		virtual void Deactivate() const = 0;
	
	protected:
		virtual GLuint GetID() const = 0;
};

#endif // _TEXTURE_INTERFACE_H
