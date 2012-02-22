#include "loadcamera.h"
#include "camera_chase.h"
#include "camera_free.h"
#include "camera_mount.h"
#include "camera_orbit.h"
#include "cfg/ptree.h"

CAMERA * LoadCamera(
	const PTree & cfg,
	const float camerabounce,
	std::ostream & error_output)
{
	std::string type, name;
	if (!cfg.get("type", type, error_output)) return 0;
	if (!cfg.get("name", name, error_output)) return 0;

	MATHVECTOR<float, 3> position;
	MATHVECTOR<float, 3> lookat;
	float fov = 0.0;
	float stiffness = 0.0;
	cfg.get("fov", fov);
	cfg.get("stiffness", stiffness);
	cfg.get("position", position);
	if (!cfg.get("lookat", lookat))
	{
		lookat = position + direction::Forward;
	}

	CAMERA * cam;
	if (type == "mount")
	{
		CAMERA_MOUNT * c = new CAMERA_MOUNT(name);
		c->SetEffectStrength(camerabounce);
		c->SetStiffness(stiffness);
		c->SetOffset(position, lookat);
		cam = c;
	}
	else if (type == "chase")
	{
		CAMERA_CHASE * c = new CAMERA_CHASE(name);
		c->SetOffset(position);
		cam = c;
	}
	else if (type == "orbit")
	{
		CAMERA_ORBIT * c = new CAMERA_ORBIT(name);
		c->SetOffset(position);
		cam = c;
	}
	else if (type == "free")
	{
		CAMERA_FREE * c = new CAMERA_FREE(name);
		c->SetOffset(position);
		cam = c;
	}
	else
	{
		error_output << "Unknown camera type " << type << std::endl;
		return 0;
	}
	cam->SetFOV(fov);
	return cam;
}
