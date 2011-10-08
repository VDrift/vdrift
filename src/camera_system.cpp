#include "camera_system.h"
#include "camera.h"

CAMERA_SYSTEM::~CAMERA_SYSTEM()
{
	for (unsigned int i = 0; i < camera.size(); i++)
	{
		if (camera[i]) delete camera[i];
	}
}

CAMERA * CAMERA_SYSTEM::Active() const
{
	return camera[active];
}

CAMERA * CAMERA_SYSTEM::Select(const std::string & name)
{
	active = 0;
	std::map<std::string, int>::iterator it = camera_map.find(name);
	if (it != camera_map.end()) active = it->second;
	return camera[active];
}

CAMERA * CAMERA_SYSTEM::Next()
{
	active++;
	if (active == camera.size()) active = 0;
	return camera[active];
}

CAMERA * CAMERA_SYSTEM::Prev()
{
	if (active == 0) active = camera.size();
	active--;
	return camera[active];
}

void CAMERA_SYSTEM::Add(CAMERA * newcam)
{
	active = camera.size();
	camera_map[newcam->GetName()] = active;
	camera.push_back(newcam);
}
