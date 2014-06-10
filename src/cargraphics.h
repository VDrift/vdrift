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

#ifndef _CARGRAPHICS_H
#define _CARGRAPHICS_H

#include "graphics/scenenode.h"
#include "mathvector.h"
#include "quaternion.h"
#include "memory.h"

#include <iosfwd>
#include <list>
#include <set>

class Camera;
class Texture;
class Model;
class PTree;
class CarDynamics;
class ContentManager;

class CarGraphics
{
public:
	CarGraphics();

	~CarGraphics();

	bool Load(
		const PTree & cfg,
		const std::string & carpath,
		const std::string & carname,
		const std::string & carwheel,
		const std::string & carpaint,
		const Vec3 & carcolor,
		const int anisotropy,
		const float camerabounce,
		ContentManager & content,
		std::ostream & error_output);

	/// update graphics from car input vector
	void Update(const std::vector<float> & inputs);

	/// update graphics from car dynamics state
	void Update(const CarDynamics & dynamics);

	void SetColor(float r, float g, float b);

	void EnableInteriorView(bool value);

	SceneNode & GetNode();

	const std::vector<Camera*> & GetCameras() const;

private:
	SceneNode topnode;
	SceneNode::Handle bodynode;
	SceneNode::Handle steernode;
	keyed_container<Drawable>::handle brakelights;
	keyed_container<Drawable>::handle reverselights;

	// car cameras
	std::vector<Camera*> cameras;

	// car lights
	struct Light
	{
		SceneNode::Handle node;
		keyed_container<Drawable>::handle draw;
	};
	std::list<Light> lights;

	// models and textures used by this car instance
	std::set<std::tr1::shared_ptr<Model> > models;
	std::set<std::tr1::shared_ptr<Texture> > textures;

	// steering wheel
	Quat steer_orientation;
	Quat steer_rotation;
	float steer_angle_max;

	// cached so we can update the brake light
	float applied_brakes;

	bool interior_view;

	bool LoadLight(
		const PTree & cfg,
		ContentManager & content,
		std::ostream & error_output);

	bool LoadCameras(
		const PTree & cfg,
		const float cambounce,
		std::ostream & error_output);
};

#endif // _CARGRAPHICS_H
