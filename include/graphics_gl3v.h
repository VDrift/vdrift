#ifndef _GRAPHICS_GL3V_H
#define _GRAPHICS_GL3V_H

#include "mathvector.h"
#include "scenenode.h"
#include "staticdrawables.h"
#include "matrix4.h"
#include "texture.h"
#include "aabb_space_partitioning.h"
#include "graphics_interface.h"

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
	
	GRAPHICS_GL3V(StringIdMap & map) : stringMap(map), renderer(gl) {}
	~GRAPHICS_GL3V() {};
	
private:
	
	StringIdMap & stringMap;
	GLWrapper gl;
	Renderer renderer;
	int w, h;
	
	// scenegraph output
	DRAWABLE_CONTAINER <PTRVECTOR> dynamic_drawlist; //used for objects that move or change
	STATICDRAWABLES static_drawlist; //used for objects that will never change
};

#endif

