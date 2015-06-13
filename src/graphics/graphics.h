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

#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include "mathvector.h"
#include "quaternion.h"

#include <iosfwd>
#include <string>

class SceneNode;

/// an abstract base class that defines the graphics interface
/// expects a valid OpenGL context with initialized extension entry points (glewInit)
class Graphics
{
public:
	/// reflection_type is 0 (low=OFF), 1 (medium=static), 2 (high=dynamic)
	/// returns true on success
	virtual bool Init(
		const std::string & shaderpath,
		unsigned resx, unsigned resy,
		unsigned antialiasing,
		bool enableshadows, int shadow_distance,
		int shadow_quality, int reflection_type,
		const std::string & static_reflectionmap_file,
		const std::string & static_ambientmap_file,
		int anisotropy, int texturesize,
		int lighting_quality, bool newbloom,
		bool newnormalmaps, bool dynamicsky,
		const std::string & renderconfig,
		std::ostream & info_output,
		std::ostream & error_output) = 0;

	virtual void Deinit() = 0;

	virtual void BindDynamicVertexData(std::vector<SceneNode*> nodes) = 0;

	virtual void BindStaticVertexData(std::vector<SceneNode*> nodes) = 0;

	virtual void AddDynamicNode(SceneNode & node) = 0;

	virtual void AddStaticNode(SceneNode & node) = 0;

	virtual void ClearDynamicDrawables() = 0;

	virtual void ClearStaticDrawables() = 0;

	/// Prepare scene for drawing. All non gpu setup happens here: culling, sorting etc
	/// Vertex data and node binds should have happened before.
	virtual void SetupScene(
		float fov, float new_view_distance,
		const Vec3 cam_position, const Quat & cam_rotation,
		const Vec3 & dynamic_reflection_sample_pos,
		std::ostream & error_output) = 0;

	/// optional (atm) scene animation update function
	/// to be called after SetupScene and before DrawScene
	virtual void UpdateScene(float /*dt*/) {};

	virtual void DrawScene(std::ostream & error_output) = 0;

	virtual int GetMaxAnisotropy() const = 0;

	virtual bool AntialiasingSupported() const = 0;

	virtual bool ReloadShaders(std::ostream & info_output, std::ostream & error_output) = 0;

	virtual void SetCloseShadow(float value) = 0;

	virtual bool GetShadows() const = 0;

	/// move skybox geometry with the camera
	virtual void SetFixedSkybox(bool enable) = 0;

	virtual void SetSunDirection(const Vec3 & value) = 0;

	virtual void SetContrast(float value) = 0;

	/// optional advanced lighting simulation interface, should be factored out eventually
	/// set scene local time in hours 0 - 23
	virtual void SetLocalTime(float /*hours*/) {};

	/// set scene local time speedup relative to real time: 0, 1, ..., 32
	virtual void SetLocalTimeSpeed(float /*value*/) {};

	virtual void printProfilingInfo(std::ostream & /*out*/) const { }

	virtual ~Graphics() {}
};

#endif

