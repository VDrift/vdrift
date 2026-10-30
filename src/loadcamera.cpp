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

std::unique_ptr<Camera> LoadCamera(
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

	if (type == "mount")
	{
		auto cam = std::make_unique<CameraMount>(name);
		cam->SetEffectStrength(camerabounce);
		cam->SetStiffness(stiffness);
		cam->SetOffset(position, lookat);
		cam->SetFOV(fov);
		return cam;
	}

	if (type == "chase")
	{
		auto cam = std::make_unique<CameraChase>(name);
		cam->SetOffset(position);
		cam->SetFOV(fov);
		return cam;
	}

	if (type == "orbit")
	{
		auto cam = std::make_unique<CameraOrbit>(name);
		cam->SetOffset(position);
		cam->SetFOV(fov);
		return cam;
	}

	if (type == "free")
	{
		auto cam = std::make_unique<CameraFree>(name);
		cam->SetOffset(position);
		cam->SetFOV(fov);
		return cam;
	}

	error_output << "Unknown camera type " << type << std::endl;
	return std::unique_ptr<Camera>();
}
