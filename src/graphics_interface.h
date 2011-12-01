#ifndef _GRAPHICS_INTERFACE_H
#define _GRAPHICS_INTERFACE_H

#include "mathvector.h"
#include "quaternion.h"
#include "drawable_container.h"

#include <string>
#include <ostream>

class SCENENODE;

/// an abstract base class that defines the graphics interface
class GRAPHICS_INTERFACE
{
public:
	typedef DRAWABLE_CONTAINER <PTRVECTOR> dynamicdrawlist_type;

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
				int anisotropy, int texturesize,
				int lighting_quality, bool newbloom, bool newnormalmaps,
				const std::string & renderconfig,
				std::ostream & info_output, std::ostream & error_output) = 0;
	virtual void Deinit() = 0;
	virtual void BeginScene(std::ostream & error_output) = 0;
	virtual DRAWABLE_CONTAINER <PTRVECTOR> & GetDynamicDrawlist() = 0;
	virtual void AddStaticNode(SCENENODE & node, bool clearcurrent = true) = 0;
	virtual void SetupScene(float fov, float new_view_distance, const MATHVECTOR <float, 3> cam_position, const QUATERNION <float> & cam_rotation,
					const MATHVECTOR <float, 3> & dynamic_reflection_sample_pos) = 0;
	virtual void DrawScene(std::ostream & error_output) = 0;
	virtual void EndScene(std::ostream & error_output) = 0;
	virtual int GetMaxAnisotropy() const = 0;
	virtual bool AntialiasingSupported() const = 0;
	virtual bool GetUsingShaders() const = 0;
	virtual bool ReloadShaders(const std::string & shaderpath, std::ostream & info_output, std::ostream & error_output) = 0;
	virtual void SetCloseShadow ( float value ) = 0;
	virtual bool GetShadows() const = 0;
	virtual void SetSunDirection ( const QUATERNION< float >& value ) = 0;
	virtual void SetContrast ( float value ) = 0;
	virtual void printProfilingInfo(std::ostream & out) const {}

	virtual ~GRAPHICS_INTERFACE() {}
};

#endif

