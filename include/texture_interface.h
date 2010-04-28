#ifndef _TEXTURE_INTERFACE_H
#define _TEXTURE_INTERFACE_H

/// an abstract base class for a simple texture interface
class TEXTURE_INTERFACE
{
	public:
		virtual bool Loaded() const = 0;
		virtual void Activate() const = 0;
		virtual void Deactivate() const = 0;
};

#endif // _TEXTURE_INTERFACE_H
