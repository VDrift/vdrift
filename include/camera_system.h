#ifndef _CAMERA_SYSTEM_H
#define _CAMERA_SYSTEM_H

#include <string>
#include <vector>
#include <map>

class CAMERA;

///camera manager class
class CAMERA_SYSTEM
{
public:
	~CAMERA_SYSTEM();
	
	CAMERA* Active() const;
	
	CAMERA* Select(const std::string & name);
	
	CAMERA* Next();
	
	CAMERA* Prev();
	
	void Add(CAMERA* newcam);

protected:
	std::vector<CAMERA*> camera;			//camera vector used to preserve order
	unsigned int active;					//active camera index
	std::map<std::string, int> camera_map;	//name => index
};

#endif // _CAMERA_SYSTEM_H
