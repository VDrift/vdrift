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

#ifndef _GRAPHICS_GL2_H
#define _GRAPHICS_GL2_H

#include "graphics.h"
#include "graphics_config.h"
#include "glstatemanager.h"
#include "texture.h"
#include "staticdrawables.h"
#include "render_input_postprocess.h"
#include "render_input_scene.h"
#include "render_output.h"
#include "memory.h"

struct GraphicsCamera;
class Shader;
class SceneNode;
class Sky;

class GraphicsGL2 : public Graphics
{
public:
	GraphicsGL2();

	~GraphicsGL2();

	/// reflection_type is 0 (low=OFF), 1 (medium=static), 2 (high=dynamic)
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

	/// Setup scene cameras
	virtual void SetupScene(
		float fov, float new_view_distance,
		const Vec3 cam_position,
		const Quat & cam_rotation,
		const Vec3 & dynamic_reflection_sample_pos);

	virtual void UpdateScene(float dt);

	virtual void DrawScene(std::ostream & error_output);

	virtual void EndScene(std::ostream & error_output);

	virtual int GetMaxAnisotropy() const;

	virtual bool AntialiasingSupported() const;

	virtual bool GetUsingShaders() const;

	virtual bool ReloadShaders(std::ostream & info_output, std::ostream & error_output);

	virtual void SetCloseShadow(float value);

	virtual bool GetShadows() const;

	virtual void SetSunDirection(const Vec3 & value);

	virtual void SetContrast(float value);

	virtual void SetLocalTime(float hours);

	virtual void SetLocalTimeSpeed(float value);

	// Allow external code to use gl state manager.
	GraphicsState & GetState();

	// Get pointer to shader called name.
	// Returns null on failure.
	Shader * GetShader(const std::string & name);

	// Add an render input texture (texture used by render passes).
	// Warning: Existing texture (texture with the same name) will be overriden.
	// Warning: The texture is expected to stay valid until graphics is destroyed.
	// todo: RemoveInputTexture ?
	void AddInputTexture(const std::string & name, TextureInterface * texture);

private:
	// avoids sending excessive state changes to OpenGL
	GraphicsState glstate;

	// configuration variables, internal data
	int w, h;
	bool initialized;
	bool using_shaders;
	GLint max_anisotropy;
	bool shadows;
	int shadow_distance;
	int shadow_quality;
	float closeshadow;
	unsigned int fsaa;
	int lighting; ///<lighting quality; see data/settings/options.config for definition of values
	bool bloom;
	bool normalmaps;
	float contrast;
	enum {REFLECTION_DISABLED, REFLECTION_STATIC, REFLECTION_DYNAMIC} reflection_status;
	Texture static_reflection;
	Texture static_ambient;
	std::string renderconfigfile;
	std::string shaderpath;

	// configuration variables in a data-driven friendly format
	std::set <std::string> conditions;
	GraphicsConfig config;

	// shaders
	typedef std::map <std::string, Shader> shader_map_type;
	shader_map_type shadermap;

	// scenegraph output
	DrawableContainer <PtrVector> dynamic_drawlist; //used for objects that move or change
	StaticDrawables static_drawlist; //used for objects that will never change

	// render outputs
	typedef std::map <std::string, RenderOutput> render_output_map_type;
	render_output_map_type render_outputs;
	typedef std::map <std::string, FrameBufferTexture> texture_output_map_type;
	texture_output_map_type texture_outputs;

	// outputs and other textures used as inputs
	std::map <std::string, reseatable_reference <TextureInterface> > texture_inputs;

	// render input objects
	RenderInputScene renderscene;
	RenderInputPostprocess postprocess;

	// camera data
	typedef std::map <std::string, GraphicsCamera> camera_map_type;
	camera_map_type cameras;

	Vec3 light_direction;
	std::tr1::shared_ptr<Sky> sky;
	bool sky_dynamic;

	void ChangeDisplay(
		const int width, const int height,
		std::ostream & error_output);

	/// shader name is used to distinquish between shaders with different defines set
	/// shader_defines is a space delimited list of defines
	bool LoadShader(
		const std::string & shader_name,
		const std::string & shader_defines,
		const std::string & shader_path,
		const std::string & vert_shader_name,
		const std::string & frag_shader_name,
		std::ostream & info_output,
		std::ostream & error_output);

	void EnableShaders(std::ostream & info_output, std::ostream & error_output);

	void DisableShaders(std::ostream & error_output);

	void CullScenePass(
		const GraphicsConfigPass & pass,
		std::map <std::string, PtrVector <Drawable> > & culled_static_drawlist,
		std::ostream & error_output);

	void DrawScenePass(
		const GraphicsConfigPass & pass,
		std::map <std::string, PtrVector <Drawable> > & culled_static_drawlist,
		std::ostream & error_output);

	/// draw postprocess scene pass
	void DrawScenePassPost(
		const GraphicsConfigPass & pass,
		std::ostream & error_output);

	/// get input textures vector from config inputs
	void GetScenePassInputTextures(
		const GraphicsConfigInputs & inputs,
		std::vector <TextureInterface*> & input_textures);

	void BindInputTextures(
		const std::vector <TextureInterface*> & textures,
		std::ostream & error_output);

	void UnbindInputTextures(
		const std::vector <TextureInterface*> & textures,
		std::ostream & error_output);

	void DrawScenePassLayer(
		const std::string & layer,
		const GraphicsConfigPass & pass,
		const std::vector <TextureInterface*> & input_textures,
		const std::map <std::string, PtrVector <Drawable> > & culled_static_drawlist,
		RenderOutput & render_output,
		std::ostream & error_output);

	void RenderDrawlist(
		const std::vector <Drawable*> & drawlist,
		RenderInputScene & render_scene,
		RenderOutput & render_output,
		std::ostream & error_output);

	void RenderDrawlists(
		const std::vector <Drawable*> & dynamic_drawlist,
		const std::vector <Drawable*> & static_drawlist,
		const std::vector <TextureInterface*> & extra_textures,
		RenderInputScene & render_scene,
		RenderOutput & render_output,
		std::ostream & error_output);

	void RenderPostProcess(
		const std::string & shadername,
		const std::vector <TextureInterface*> & textures,
		RenderOutput & render_output,
		bool write_color,
		bool write_alpha,
		std::ostream & error_output);

	void Render(
		RenderInput * input,
		RenderOutput & output,
		std::ostream & error_output);
};

#endif
