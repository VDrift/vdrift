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

#ifndef _GRAPHICS_GL3V_H
#define _GRAPHICS_GL3V_H

#include "graphics.h"
#include "mathvector.h"
#include "scenenode.h"
#include "staticdrawables.h"
#include "matrix4.h"
#include "texture.h"
#include "vertexarray.h"
#include "frustum.h"
#include "graphics_config_condition.h"
#include "gl3v/glwrapper.h"
#include "gl3v/renderer.h"
#include "gl3v/stringidmap.h"

#include <iosfwd>
#include <string>
#include <map>
#include <list>
#include <vector>

class SceneNode;

/// a wrapper around the gl3v renderer
class GraphicsGL3 : public Graphics
{
public:
	///reflection_type is 0 (low=OFF), 1 (medium=static), 2 (high=dynamic)
	/// returns true on success
	virtual bool Init(
		const std::string & shaderpath,
		unsigned resx, unsigned resy,
		unsigned bpp, unsigned depthbpp,
		bool fullscreen, unsigned antialiasing,
		bool enableshadows, int shadow_distance,
		int shadow_quality, int reflection_type,
		const std::string & static_reflectionmap_file,
		const std::string & static_ambientmap_file,
		int anisotropy, int texturesize,
		int lighting_quality, bool newbloom,
		bool newnormalmaps, bool dynamicsky,
		const std::string & renderconfig,
		std::ostream & info_output,
		std::ostream & error_output);
	virtual void Deinit();
	virtual void BeginScene(std::ostream & error_output);
	virtual DrawableContainer <PtrVector> & GetDynamicDrawlist();
	virtual void AddStaticNode(SceneNode & node, bool clearcurrent = true);
	virtual void SetupScene(float fov, float new_view_distance, const Vec3 cam_position, const Quat & cam_rotation,
					const Vec3 & dynamic_reflection_sample_pos);
	virtual void DrawScene(std::ostream & error_output);
	virtual void EndScene(std::ostream & error_output);
	virtual int GetMaxAnisotropy() const;
	virtual bool AntialiasingSupported() const;
	virtual bool GetUsingShaders() const;
	virtual bool ReloadShaders(std::ostream & info_output, std::ostream & error_output);
	virtual void SetCloseShadow ( float value );
	virtual bool GetShadows() const;
	virtual void SetSunDirection(const Vec3 & value);
	virtual void SetContrast(float value);
	virtual void printProfilingInfo(std::ostream & out) const {renderer.printProfilingInfo(out);}

	GraphicsGL3(StringIdMap & map);
	~GraphicsGL3() {};

private:
	StringIdMap & stringMap;
	GLWrapper gl;
	Renderer renderer;
	std::string rendercfg;
	std::string shaderpath;
	int w, h;
	bool logNextGlFrame; // used to take a gl log capture after reloading shaders if gl logging is enabled
	bool initialized;
	Vec3 lastCameraPosition;
	Vec3 light_direction;

	struct CameraMatrices
	{
		Matrix4 <float> projectionMatrix;
		Matrix4 <float> inverseProjectionMatrix;
		Matrix4 <float> viewMatrix;
		Matrix4 <float> inverseViewMatrix;
	};
	std::map <std::string, CameraMatrices> cameras;
	CameraMatrices & setCameraPerspective(const std::string & name,
							  const Vec3 & position,
							  const Quat & rotation,
							  float fov,
							  float nearDistance,
							  float farDistance,
							  float w,
							  float h);
	CameraMatrices & setCameraOrthographic(const std::string & name,
							   const Vec3 & position,
							   const Quat & rotation,
							   const Vec3 & orthoMin,
							   const Vec3 & orthoMax);

	std::string getCameraDrawGroupKey(StringId pass, StringId group) const;
	std::string getCameraForPass(StringId pass) const;

	// scenegraph output
	DrawableContainer <PtrVector> dynamic_drawlist; //used for objects that move or change
	StaticDrawables static_drawlist; //used for objects that will never change

	// a special drawable that's used for fullscreen quad passes
	Drawable fullscreenquad;
	VertexArray fullscreenquadVertices;

	// drawlist cache
	std::map <std::string, std::vector <RenderModelExt*> > cameraDrawGroupDrawLists;

	// drawlist assembly functions
	void assembleDrawList(const std::vector <Drawable*> & drawables, std::vector <RenderModelExt*> & out, Frustum * frustum, const Vec3 & camPos);
	void assembleDrawList(const AabbTreeNodeAdapter <Drawable> & adapter, std::vector <RenderModelExt*> & out, Frustum * frustum, const Vec3 & camPos);

	// a map that stores which camera each pass uses
	std::map <std::string, std::string> passNameToCameraName;

	// a set storing all configuration option conditions (bloom enabled, etc)
	std::set <std::string> conditions;

	Texture static_reflection;

	float closeshadow;
};

#endif

