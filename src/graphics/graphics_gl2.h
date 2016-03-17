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
#include "graphicsstate.h"
#include "texture.h"
#include "aabb_tree_adapter.h"
#include "drawable_container.h"
#include "render_input_postprocess.h"
#include "render_input_scene.h"
#include "render_output.h"
#include "vertexarray.h"
#include "vertexbuffer.h"

#include <memory>

struct GraphicsCamera;
class Shader;
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
		std::ostream & error_output);

	virtual void Deinit();

	virtual void BindDynamicVertexData(std::vector<SceneNode*> nodes);

	virtual void BindStaticVertexData(std::vector<SceneNode*> nodes);

	virtual void AddDynamicNode(SceneNode & node);

	virtual void AddStaticNode(SceneNode & node);

	virtual void ClearDynamicDrawables();

	virtual void ClearStaticDrawables();

	virtual void SetupScene(
		float fov, float new_view_distance,
		const Vec3 cam_position,
		const Quat & cam_rotation,
		const Vec3 & dynamic_reflection_sample_pos,
		std::ostream & error_output);

	virtual void UpdateScene(float dt);

	virtual void DrawScene(std::ostream & error_output);

	virtual int GetMaxAnisotropy() const;

	virtual bool AntialiasingSupported() const;

	virtual bool ReloadShaders(std::ostream & info_output, std::ostream & error_output);

	virtual void SetCloseShadow(float value);

	virtual bool GetShadows() const;

	virtual void SetFixedSkybox(bool enable);

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
	GLint max_anisotropy;

	// shadow state
	Mat4 shadow_matrix[3];
	int shadow_distance;
	int shadow_quality;
	float closeshadow;
	bool shadows;

	unsigned int fsaa;
	int lighting; ///<lighting quality; see data/settings/options.config for definition of values
	bool bloom;
	bool normalmaps;
	bool glsl_330;
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
	typedef std::map <std::string, Shader> ShaderMap;
	ShaderMap shaders;

	// vertex data buffer
	VertexBuffer vertex_buffer;

	// a special drawable that's used for fullscreen draw passes
	Drawable screen_quad;
	VertexArray screen_quad_verts;

	// scenegraph output
	template <typename T> class PtrVector : public std::vector<T*> {};
	typedef DrawableContainer <PtrVector> DynamicDrawables;
	DynamicDrawables dynamic_draw_lists; //used for objects that move or change

	typedef DrawableContainer<AabbTreeNodeAdapter> StaticDrawables;
	StaticDrawables static_draw_lists; //used for objects that will never change

	struct CulledDrawList
	{
		CulledDrawList() : valid(false) {};
		PtrVector <Drawable> drawables;
		bool valid;
	};
	typedef std::map <std::string, CulledDrawList> CulledDrawListMap;
	CulledDrawListMap culled_draw_lists;

	// render outputs
	typedef std::map <std::string, RenderOutput> RenderOutputMap;
	RenderOutputMap render_outputs;

	typedef std::map <std::string, FrameBufferTexture> TextureOutputMap;
	TextureOutputMap texture_outputs;

	// outputs and other textures used as inputs
	std::map <std::string, reseatable_reference <TextureInterface> > texture_inputs;

	// render input objects
	RenderInputScene renderscene;
	RenderInputPostprocess postprocess;

	// camera data
	typedef std::map <std::string, GraphicsCamera> CameraMap;
	CameraMap cameras;

	// scene passes
	struct GraphicsPass
	{
		std::vector<AabbTreeNodeAdapter<Drawable>*> static_draw_lists;
		std::vector<PtrVector<Drawable>*> dynamic_draw_lists;
		std::vector<CulledDrawList*> draw_lists;
		std::vector<TextureInterface*> textures;
		GraphicsCamera * camera;
		GraphicsCamera * sub_cameras[6];
		RenderOutput * output;
		Shader * shader;
		BlendMode::Enum blend_mode;
		GLenum depth_test;
		bool write_depth;
		bool write_color;
		bool write_alpha;
		bool clear_depth;
		bool clear_color;
		bool postprocess;
		bool cull;
	};
	std::vector<GraphicsPass> passes;

	Vec3 light_direction;
	std::shared_ptr<Sky> sky;
	bool sky_dynamic;
	bool fixed_skybox;


	void ChangeDisplay(
		const int width, const int height,
		std::ostream & error_output);

	/// common graphics config shader defines
	void GetShaderDefines(std::vector <std::string> & defines) const;

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

	/// load render configuration
	bool EnableShaders(std::ostream & info_output, std::ostream & error_output);

	bool SetupFrameBufferTextures(std::ostream & info_output, std::ostream & error_output);

	bool SetupFrameBufferObjectsAndShaders(std::ostream & info_output, std::ostream & error_output);

	void SetupCameras(
		float fov,
		float view_distance,
		const Vec3 cam_position,
		const Quat & cam_rotation,
		const Vec3 & dynamic_reflection_sample_pos);

	void ClearCulledDrawLists();

	bool InitScenePass(
		const GraphicsConfigPass & pass_config,
		GraphicsPass & pass,
		std::ostream & error_output);

	void CullScenePass(
		const GraphicsPass & pass,
		std::ostream & error_output);

	void DrawScenePass(
		const GraphicsPass & pass,
		std::ostream & error_output);

	/// draw postprocess scene pass
	void DrawScenePassPost(
		const GraphicsPass & pass,
		std::ostream & error_output);

	/// get input textures vector from config inputs
	void GetScenePassInputTextures(
		const GraphicsConfigInputs & inputs,
		std::vector <TextureInterface*> & input_textures);
};

#endif
