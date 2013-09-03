/************************************************************************/
/*                                                                      */
/* This file is part of VDrift.                                         */
/*                                                                      */
/* VDrift is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* VDrift is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with VDrift.  If not, see <http://www.gnu.org/licenses/>.      */
/*                                                                      */
/************************************************************************/

#include "loadcamera.h"
#include "camera_chase.h"
#include "camera_free.h"
#include "camera_mount.h"
#include "camera_orbit.h"
#include "cfg/ptree.h"

Camera * LoadCamera(
	const PTree & cfg,
	const float camerabounce,
	std::ostream & error_output)
{
	std::string type, name;
	if (!cfg.get("type", type, error_output)) return 0;
	if (!cfg.get("name", name, error_output)) return 0;

	Vec3 position;
	Vec3 lookat;
	float fov = 0.0;
	float stiffness = 0.0;
	cfg.get("fov", fov);
	cfg.get("stiffness", stiffness);
	cfg.get("position", position);
	if (!cfg.get("lookat", lookat))
	{
		lookat = position + Direction::Forward;
	}

	Camera * cam;
	if (type == "mount")
	{
		CameraMount * c = new CameraMount(name);
		c->SetEffectStrength(camerabounce);
		c->SetStiffness(stiffness);
		c->SetOffset(position, lookat);
		cam = c;
	}
	else if (type == "chase")
	{
		CameraChase * c = new CameraChase(name);
		c->SetOffset(position);
		cam = c;
	}
	else if (type == "orbit")
	{
		CameraOrbit * c = new CameraOrbit(name);
		c->SetOffset(position);
		cam = c;
	}
	else if (type == "free")
	{
		CameraFree * c = new CameraFree(name);
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
