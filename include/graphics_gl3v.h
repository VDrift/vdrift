#ifndef _GRAPHICS_GL3V_H
#define _GRAPHICS_GL3V_H

#include "mathvector.h"
#include "scenenode.h"
#include "staticdrawables.h"
#include "matrix4.h"
#include "texture.h"
#include "graphics_interface.h"
#include "frustum.h"

#include "gl3v/glwrapper.h"
#include "gl3v/renderer.h"
#include "gl3v/stringidmap.h"

#include <string>
#include <ostream>
#include <map>
#include <list>
#include <vector>

class SCENENODE;

/// a wrapper around the gl3v renderer
class GRAPHICS_GL3V : public GRAPHICS_INTERFACE
{
public:
	///reflection_type is 0 (low=OFF), 1 (medium=static), 2 (high=dynamic)
	/// returns true on success
	virtual bool Init(const std::string & shaderpath,
				unsigned int resx, unsigned int resy, unsigned int bpp,
				unsigned int depthbpp, bool fullscreen, bool shaders,
				unsigned int antialiasing, bool enableshadows,
				int shadow_distance, int shadow_quality,
				int reflection_type,
				const std::string & static_reflectionmap_file,
				const std::string & static_ambientmap_file,
				int anisotropy, const std::string & texturesize,
				int lighting_quality, bool newbloom, bool newnormalmaps,
				const std::string & renderconfig,
				std::ostream & info_output, std::ostream & error_output);
	virtual void Deinit();
	virtual void BeginScene(std::ostream & error_output);
	virtual DRAWABLE_CONTAINER <PTRVECTOR> & GetDynamicDrawlist();
	virtual void AddStaticNode(SCENENODE & node, bool clearcurrent = true);
	virtual void SetupScene(float fov, float new_view_distance, const MATHVECTOR <float, 3> cam_position, const QUATERNION <float> & cam_rotation,
					const MATHVECTOR <float, 3> & dynamic_reflection_sample_pos);
	virtual void DrawScene(std::ostream & error_output);
	virtual void EndScene(std::ostream & error_output);
	virtual int GetMaxAnisotropy() const;
	virtual bool AntialiasingSupported() const;
	virtual bool GetUsingShaders() const;
	virtual bool ReloadShaders(const std::string & shaderpath, std::ostream & info_output, std::ostream & error_output);
	virtual void SetCloseShadow ( float value );
	virtual bool GetShadows() const;
	virtual void SetSunDirection ( const QUATERNION< float >& value );
	virtual void SetContrast ( float value );
	
	GRAPHICS_GL3V(StringIdMap & map);
	~GRAPHICS_GL3V() {};
	
private:
	
	StringIdMap & stringMap;
	GLWrapper gl;
	Renderer renderer;
	int w, h;
	bool logNextGlFrame; // used to take a gl log capture after reloading shaders if gl logging is enabled
	bool initialized;
	MATHVECTOR <float, 3> lastCameraPosition;
	
	struct CameraMatrices
	{
		MATRIX4 <float> projectionMatrix;
		MATRIX4 <float> viewMatrix;
	};
	std::map <std::string, CameraMatrices> cameras;
	void setCameraPerspective(const std::string & name, 
							  const MATHVECTOR <float, 3> & position,
							  const QUATERNION <float> & rotation,
							  float fov,
							  float nearDistance,
							  float farDistance,
							  float w,
							  float h);
	void setCameraOrthographic(const std::string & name,
							   const MATHVECTOR <float, 3> & position,
							   const QUATERNION <float> & rotation,
							   const MATHVECTOR <float, 3> & orthoMin,
							   const MATHVECTOR <float, 3> & orthoMax);
	
	std::string getCameraDrawGroupKey(StringId pass, StringId group) const;
	std::string getCameraForPass(StringId pass) const;
	
	// scenegraph output
	DRAWABLE_CONTAINER <PTRVECTOR> dynamic_drawlist; //used for objects that move or change
	STATICDRAWABLES static_drawlist; //used for objects that will never change
	
	// drawlist cache
	std::map <std::string, std::vector <RenderModelExternal*> > cameraDrawGroupDrawLists;
	
	// drawlist assembly functions
	void assembleDrawList(const std::vector <DRAWABLE*> & drawables, std::vector <RenderModelExternal*> & out, FRUSTUM * frustum, const MATHVECTOR <float, 3> & camPos);
	void assembleDrawList(const AABB_SPACE_PARTITIONING_NODE_ADAPTER <DRAWABLE> & adapter, std::vector <RenderModelExternal*> & out, FRUSTUM * frustum, const MATHVECTOR <float, 3> & camPos);
	
	// a map that stores which camera each pass uses
	std::map <std::string, std::string> passNameToCameraName;
};

#endif

