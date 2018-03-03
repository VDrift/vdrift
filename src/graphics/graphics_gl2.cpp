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

#include "graphics_gl2.h"
#include "graphics_camera.h"
#include "scenenode.h"
#include "glutil.h"
#include "shader.h"
#include "uniforms.h"
#include "vertexattrib.h"
#include "frustumcull.h"
#include "model.h"
#include "sky.h"
#include "tokenize.h"

/// array end ptr
template <typename T, size_t N>
static T * End(T (&ar)[N])
{
	return ar + N;
}

static FrameBufferTexture::Format TextureFormatFromString(const std::string & format)
{
	if (format == "depth" || format == "depthshadow")
		return FrameBufferTexture::DEPTH24;
	else if (format == "R8")
		return FrameBufferTexture::R8;
	else if (format == "RGBA8")
		return FrameBufferTexture::RGBA8;
	else if (format == "RGB8")
		return FrameBufferTexture::RGB8;
	else if (format == "RGBA16")
		return FrameBufferTexture::RGBA16;
	else if (format == "RGB16")
		return FrameBufferTexture::RGB16;
	else
		assert(0);

	return FrameBufferTexture::RGB8;
}

static FrameBufferTexture::Target TextureTargetFromString(const std::string & type)
{
	if (type == "rectangle")
		return FrameBufferTexture::RECTANGLE;
	else if (type == "cube")
		return FrameBufferTexture::CUBEMAP;
	else
		return FrameBufferTexture::NORMAL;
}

static GLint DepthModeFromString(const std::string & mode)
{
	if (mode == "lequal")
		return GL_LEQUAL;
	else if (mode == "equal")
		return GL_EQUAL;
	else if (mode == "gequal")
		return GL_GEQUAL;
	else if (mode == "disabled")
		return GL_ALWAYS;
	else
		assert(0);

	return GL_LEQUAL;
}

static BlendMode::Enum BlendModeFromString(const std::string & mode)
{
	if (mode == "disabled")
		return BlendMode::DISABLED;
	else if (mode == "add")
		return BlendMode::ADD;
	else if (mode == "alphablend")
		return BlendMode::ALPHABLEND;
	else if (mode == "alphablend_premultiplied")
		return BlendMode::PREMULTIPLIED_ALPHA;
	else
		assert(0);

	return BlendMode::DISABLED;
}

static bool SortDraworder(Drawable * d1, Drawable * d2)
{
	assert(d1 && d2);
	return (d1->GetDrawOrder() < d2->GetDrawOrder());
}

static std::string BuildKey(const std::string & camera, const std::string & draw)
{
	return camera + ";" + draw;
}

static Quat GetCubeSideOrientation(int i, const Quat & origorient, std::ostream & error_output)
{
	Quat orient = origorient;

	switch (i)
	{
		case 0:
		orient.Rotate(M_PI*0.5, 0,1,0);
		break;

		case 1:
		orient.Rotate(-M_PI*0.5, 0,1,0);
		break;

		case 2:
		orient.Rotate(M_PI*0.5, 1,0,0);
		break;

		case 3:
		orient.Rotate(-M_PI*0.5, 1,0,0);
		break;

		case 4:
		// orient is already set up for us!
		break;

		case 5:
		orient.Rotate(M_PI, 0,1,0);
		break;

		default:
		error_output << "Reached odd spot while building cubemap orientation. How many sides are in a cube, anyway? " << i << "?" << std::endl;
		assert(0);
		break;
	};

	return orient;
}

static void AttachCubeSide(int i, FrameBufferObject & reflection_fbo, std::ostream & error_output)
{
	switch (i)
	{
		case 0:
		reflection_fbo.SetCubeSide(FrameBufferTexture::POSX);
		break;

		case 1:
		reflection_fbo.SetCubeSide(FrameBufferTexture::NEGX);
		break;

		case 2:
		reflection_fbo.SetCubeSide(FrameBufferTexture::POSY);
		break;

		case 3:
		reflection_fbo.SetCubeSide(FrameBufferTexture::NEGY);
		break;

		case 4:
		reflection_fbo.SetCubeSide(FrameBufferTexture::POSZ);
		break;

		case 5:
		reflection_fbo.SetCubeSide(FrameBufferTexture::NEGZ);
		break;

		default:
		error_output << "Reached odd spot while attaching cubemap side. How many sides are in a cube, anyway? " << i << "?" << std::endl;
		assert(0);
		break;
	};

	CheckForOpenGLErrors("cubemap generation: FBO cube side attachment", error_output);
}

GraphicsGL2::GraphicsGL2() :
	initialized(false),
	max_anisotropy(0),
	closeshadow(5.0),
	shadows(false),
	fsaa(1),
	lighting(0),
	bloom(false),
	normalmaps(false),
	glsl_330(false),
	contrast(1.0),
	reflection_status(REFLECTION_DISABLED),
	renderconfigfile("basic.conf"),
	renderscene(vertex_buffer),
	postprocess(vertex_buffer, screen_quad),
	light_direction(1,1,1),
	sky_dynamic(false),
	fixed_skybox(true)
{
	const unsigned int faces[2 * 3] = {
		0, 1, 2,
		2, 3, 0,
	};
	const float pos[4 * 3] = {
		0.0f,  0.0f, 0.0f,
		1.0f,  0.0f, 0.0f,
		1.0f,  1.0f, 0.0f,
		0.0f,  1.0f, 0.0f,
	};
	const float tco[4 * 2] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
	};
	screen_quad_verts.Add(faces, 6, pos, 12, tco, 8);
	screen_quad.SetVertArray(&screen_quad_verts);
}

GraphicsGL2::~GraphicsGL2()
{
	render_outputs.clear();
	texture_outputs.clear();
	texture_inputs.clear();
}

bool GraphicsGL2::Init(
	const std::string & newshaderpath,
	unsigned resx, unsigned resy,
	unsigned antialiasing,
	bool enableshadows, int new_shadow_distance,
	int new_shadow_quality, int reflection_type,
	const std::string & static_reflectionmap_file,
	const std::string & static_ambientmap_file,
	int anisotropy, int texturesize,
	int lighting_quality, bool newbloom,
	bool newnormalmaps, bool dynamicsky,
	const std::string & renderconfig,
	std::ostream & info_output,
	std::ostream & error_output)
{
	assert(!renderconfig.empty() && "Render configuration name string empty.");

	const GLubyte * version = glGetString(GL_VERSION);
	if (version[0] < '2')
	{
		error_output << "Graphics card or driver does not support required GL Version: 2.0" << std::endl;
		return false;
	}
	if (version[0] > '2')
	{
		int major_version = 0;
		int minor_version = 0;
		glGetIntegerv(GL_MAJOR_VERSION, &major_version);
		glGetIntegerv(GL_MINOR_VERSION, &minor_version);
		glsl_330 = (major_version > 3 || (major_version == 3 && minor_version >= 3));
	}

	#ifdef _WIN32
	// workaround for broken vao implementation Intel/Windows
	{
		const std::string vendor = (const char*)glGetString(GL_VENDOR);
		if (vendor == "Intel")
			vertex_buffer.BindElementBufferExplicitly();
	}
	#endif

	shadows = enableshadows;
	shadow_distance = new_shadow_distance;
	shadow_quality = new_shadow_quality;
	lighting = lighting_quality;
	bloom = newbloom;
	normalmaps = newnormalmaps;
	renderconfigfile = renderconfig;
	shaderpath = newshaderpath;
	sky_dynamic = dynamicsky;

	if (reflection_type == 1)
		reflection_status = REFLECTION_STATIC;
	else if (reflection_type == 2)
		reflection_status = REFLECTION_DYNAMIC;

	fsaa = (antialiasing > 1) ? antialiasing : 1;

	ChangeDisplay(resx, resy, error_output);

	if (GLC_EXT_texture_filter_anisotropic)
		glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anisotropy);
	info_output << "Maximum anisotropy: " << max_anisotropy << std::endl;

	GLint maxattach;
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxattach);
	info_output << "Maximum color attachments: " << maxattach << std::endl;

	const GLint mrtreq = 1;
	GLint mrt = 0;
	glGetIntegerv(GL_MAX_DRAW_BUFFERS, &mrt);
	info_output << "Maximum draw buffers (" << mrtreq << " required): " << mrt << std::endl;

	bool use_fbos = GLC_ARB_framebuffer_object && mrt >= mrtreq && maxattach >= mrtreq;
	if (renderconfigfile != "basic.conf" && !use_fbos)
	{
		info_output << "Graphics card doesn't support framebuffer objects." << std::endl;

		shadows = false;
		sky_dynamic = false;
		renderconfigfile = "basic.conf";
		info_output << "Fall back to: " << renderconfigfile << std::endl;
	}

	if (!static_reflectionmap_file.empty() && reflection_status != REFLECTION_DISABLED)
	{
		TextureInfo t;
		t.cube = true;
		t.verticalcross = true;
		t.mipmap = true;
		t.anisotropy = anisotropy;
		t.maxsize = TextureInfo::Size(texturesize);
		static_reflection.Load(static_reflectionmap_file, t, error_output);
	}

	if (!static_ambientmap_file.empty())
	{
		TextureInfo t;
		t.cube = true;
		t.verticalcross = true;
		t.mipmap = false;
		t.anisotropy = anisotropy;
		t.maxsize = TextureInfo::Size(texturesize);
		static_ambient.Load(static_ambientmap_file, t, error_output);
	}

	if (!EnableShaders(info_output, error_output))
	{
		// try to fall back to basic.conf
		if (renderconfigfile != "basic.conf")
		{
			shadows = false;
			sky_dynamic = false;
			renderconfigfile = "basic.conf";
			info_output << "Fall back to: " << renderconfigfile << std::endl;

			if (!EnableShaders(info_output, error_output))
				return false;
		}
	}

	if (sky_dynamic)
	{
		sky.reset(new Sky(*this, error_output));
		texture_inputs["sky"] = sky.get();
	}

	// gl state setup
	glPolygonOffset(-1, -1);

	info_output << "Renderer: " << shaderpath << "/" << renderconfigfile << std::endl;
	initialized = true;

	return true;
}

void GraphicsGL2::Deinit()
{
	if (!shaders.empty())
	{
		glUseProgram(0);
		shaders.clear();
	}
}

void GraphicsGL2::BindDynamicVertexData(std::vector<SceneNode*> nodes)
{
	// TODO: This doesn't look very efficient...
	SceneNode quad_node;
	SceneNode::DrawableHandle d = quad_node.GetDrawList().twodim.insert(screen_quad);
	nodes.push_back(&quad_node);

	vertex_buffer.SetDynamicVertexData(&nodes[0], nodes.size());

	screen_quad = quad_node.GetDrawList().twodim.get(d);
}

void GraphicsGL2::BindStaticVertexData(std::vector<SceneNode*> nodes)
{
	vertex_buffer.SetStaticVertexData(&nodes[0], nodes.size());
}

void GraphicsGL2::AddDynamicNode(SceneNode & node)
{
	Mat4 identity;
	node.Traverse(dynamic_draw_lists, identity);
}

void GraphicsGL2::AddStaticNode(SceneNode & node)
{
	Mat4 identity;
	node.Traverse(static_draw_lists, identity);
	static_draw_lists.ForEach(OptimizeFunctor());
}

void GraphicsGL2::ClearDynamicDrawables()
{
	dynamic_draw_lists.clear();
}

void GraphicsGL2::ClearStaticDrawables()
{
	static_draw_lists.clear();
}

void GraphicsGL2::SetupScene(
	float fov, float new_view_distance,
	const Vec3 cam_position,
	const Quat & cam_rotation,
	const Vec3 & dynamic_reflection_sample_pos,
	std::ostream & error_output)
{
	SetupCameras(fov, new_view_distance, cam_position, cam_rotation, dynamic_reflection_sample_pos);

	// sort the two dimentional drawlist so we get correct ordering
	std::sort(dynamic_draw_lists.twodim.begin(), dynamic_draw_lists.twodim.end(), &SortDraworder);

	// do fast culling queries for static geometry per pass
	ClearCulledDrawLists();
	for (const auto & pass : passes)
	{
		CullScenePass(pass, error_output);
	}

	renderscene.SetFSAA(fsaa);
	renderscene.SetContrast(contrast);
	renderscene.SetSunDirection(light_direction);

	postprocess.SetContrast(contrast);
	postprocess.SetSunDirection(light_direction);
}

void GraphicsGL2::UpdateScene(float dt)
{
	if (sky.get())
	{
		sky->Update(dt);
		SetSunDirection(sky->GetSunDirection());
	}
}

void GraphicsGL2::DrawScene(std::ostream & error_output)
{
	// vertex object might have been modified ouside, reset it
	glstate.ResetVertexObject();

	// draw the passes
	for (const auto & pass : passes)
	{
		if (!pass.postprocess)
			DrawScenePass(pass, error_output);
		else
			DrawScenePassPost(pass, error_output);
	}

	// reset texture and draw buffer
	glstate.BindTexture(0, GL_TEXTURE_2D, 0);
	glstate.BindFramebuffer(GL_FRAMEBUFFER, 0);
}

int GraphicsGL2::GetMaxAnisotropy() const
{
	return max_anisotropy;
}

bool GraphicsGL2::AntialiasingSupported() const
{
	return GLC_ARB_multisample;
}

bool GraphicsGL2::ReloadShaders(std::ostream & info_output, std::ostream & error_output)
{
	return EnableShaders(info_output, error_output);
}

void GraphicsGL2::SetCloseShadow(float value)
{
	closeshadow = value;
}

bool GraphicsGL2::GetShadows() const
{
	return shadows;
}

void GraphicsGL2::SetFixedSkybox(bool enable)
{
	fixed_skybox = enable;
}

void GraphicsGL2::SetSunDirection(const Vec3 & value)
{
	light_direction = value;
}

void GraphicsGL2::SetContrast(float value)
{
	contrast = value;
}

void GraphicsGL2::SetLocalTime(float hours)
{
	if (sky.get())
		sky->SetTime(hours);
}

void GraphicsGL2::SetLocalTimeSpeed(float value)
{
	if (sky.get())
		sky->SetTimeSpeed(value);
}

GraphicsState & GraphicsGL2::GetState()
{
	return glstate;
}

Shader * GraphicsGL2::GetShader(const std::string & name)
{
	auto it = shaders.find(name);
	if (it != shaders.end())
		return &it->second;
	else
		return 0;
}

void GraphicsGL2::AddInputTexture(const std::string & name, TextureInterface * texture)
{
	texture_inputs[name] = texture;
}

void GraphicsGL2::ChangeDisplay(
	const int width, const int height,
	std::ostream & error_output)
{
	glstate.SetViewport(width, height);

	CheckForOpenGLErrors("ChangeDisplay", error_output);

	w = width;
	h = height;
}

void GraphicsGL2::GetShaderDefines(std::vector <std::string> & defines) const
{
	defines.clear();

	{
		std::ostringstream s;
		s << "SCREENRESX " << w;
		defines.push_back(s.str());
	}
	{
		std::ostringstream s;
		s << "SCREENRESY " << h;
		defines.push_back(s.str());
	}

	if (reflection_status == REFLECTION_DISABLED)
		defines.push_back("_REFLECTIONDISABLED_");
	else if (reflection_status == REFLECTION_STATIC)
		defines.push_back("_REFLECTIONSTATIC_");
	else if (reflection_status == REFLECTION_DYNAMIC)
		defines.push_back("_REFLECTIONDYNAMIC_");

	if (shadows)
	{
		defines.push_back("_SHADOWS_");
		if (shadow_distance > 0)
			defines.push_back("_CSM2_");
		if (shadow_distance > 1)
			defines.push_back("_CSM3_");
		if (shadow_quality == 0)
			defines.push_back("_SHADOWSLOW_");
		if (shadow_quality == 1)
			defines.push_back("_SHADOWSMEDIUM_");
		if (shadow_quality == 2)
			defines.push_back("_SHADOWSHIGH_");
		if (shadow_quality == 3)
			defines.push_back("_SHADOWSVHIGH_");
		if (shadow_quality == 4)
			defines.push_back("_SHADOWSULTRA_");
	}

	if (normalmaps)
		defines.push_back("_NORMALMAPS_");

	if (lighting == 1)
		defines.push_back("_SSAO_LOW_");

	if (lighting == 2)
		defines.push_back("_SSAO_HIGH_");
}

bool GraphicsGL2::EnableShaders(std::ostream & info_output, std::ostream & error_output)
{
	CheckForOpenGLErrors("EnableShaders: start", error_output);

	// unload shaders
	glUseProgram(0);
	shaders.clear();
	CheckForOpenGLErrors("EnableShaders: shader unload", error_output);

	// unload inputs/outputs
	render_outputs.clear();
	texture_outputs.clear();
	texture_inputs.clear();
	CheckForOpenGLErrors("EnableShaders: FBO deinit", error_output);

	dynamic_draw_lists.clear();
	static_draw_lists.clear();
	culled_draw_lists.clear();
	passes.clear();

	// reload configuration
	config = GraphicsConfig();
	std::string rcpath = shaderpath + "/" + renderconfigfile;
	if (!config.Load(rcpath, error_output))
	{
		error_output << "EnableShaders: Error loading render configuration file: " << rcpath << std::endl;
		return false;
	}

	bool ssao = (lighting > 0);
	bool ssao_low = (lighting == 1);
	bool ssao_high = (lighting == 2);
	bool reflection_disabled = (reflection_status == REFLECTION_DISABLED);
	bool reflection_dynamic = (reflection_status == REFLECTION_DYNAMIC);
	bool shadows_near = shadows;
	bool shadows_medium = shadows && shadow_distance > 0;
	bool shadows_far = shadows && shadow_distance > 1;
	bool shadow_quality_low = shadows && (shadow_quality == 0);
	bool shadow_quality_medium = shadows && (shadow_quality == 1);
	bool shadow_quality_high = shadows && (shadow_quality == 2);
	bool shadow_quality_vhigh = shadows && (shadow_quality == 3);
	bool shadow_quality_ultra = shadows && (shadow_quality == 4);

	// for now, map vhigh and ultra to high
	shadow_quality_high = shadow_quality_high || shadow_quality_vhigh || shadow_quality_ultra;
	shadow_quality_vhigh = false;
	shadow_quality_ultra = true;

	conditions.clear();
	if (fsaa > 1) conditions.insert("fsaa");
	#define ADDCONDITION(x) if (x) conditions.insert(#x)
	ADDCONDITION(bloom);
	ADDCONDITION(normalmaps);
	ADDCONDITION(ssao);
	ADDCONDITION(ssao_low);
	ADDCONDITION(ssao_high);
	ADDCONDITION(reflection_disabled);
	ADDCONDITION(reflection_dynamic);
	ADDCONDITION(shadows_near);
	ADDCONDITION(shadows_medium);
	ADDCONDITION(shadows_far);
	ADDCONDITION(shadow_quality_low);
	ADDCONDITION(shadow_quality_medium);
	ADDCONDITION(shadow_quality_high);
	ADDCONDITION(shadow_quality_vhigh);
// 		ADDCONDITION(shadow_quality_ultra);
	ADDCONDITION(sky_dynamic);
	#undef ADDCONDITION

	// add some common textures
	if (reflection_status == REFLECTION_STATIC)
		texture_inputs["reflection_cube"] = static_reflection;
	texture_inputs["ambient_cube"] = static_ambient;

	if (!SetupFrameBufferTextures(info_output, error_output))
		return false;

	if (!SetupFrameBufferObjectsAndShaders(info_output, error_output))
		return false;

	// create cameras
	SetupCameras(90, 1000, Vec3(0), Quat(), Vec3(0));

	// init scene passes
	passes.reserve(config.passes.size());
	for (const auto & pass_config : config.passes)
	{
		if (pass_config.conditions.Satisfied(conditions))
		{
			passes.push_back(GraphicsPass());
			if (!InitScenePass(pass_config, passes.back(), error_output))
				return false;
		}
	}

	return true;
}

bool GraphicsGL2::SetupFrameBufferTextures(std::ostream & info_output, std::ostream & error_output)
{
	const bool has_texture_float = GLC_ARB_texture_float && GLC_ARB_half_float_pixel;
	for (const auto & output : config.outputs)
	{
		if (!output.conditions.Satisfied(conditions))
			continue;

		if (texture_outputs.find(output.name) != texture_outputs.end())
		{
			error_output << "Ignore duplicate definition of output: " << output.name << std::endl;
			continue;
		}

		if (output.type == "framebuffer")
		{
			render_outputs[output.name].RenderToFramebuffer(w, h);
		}
		else
		{
			FrameBufferTexture::Target target = TextureTargetFromString(output.type);
			FrameBufferTexture::Format format = TextureFormatFromString(output.format);
			if (!has_texture_float && (format == FrameBufferTexture::RGBA16 || format == FrameBufferTexture::RGB16))
			{
				error_output << "Your video card doesn't support floating point textures." << std::endl;
				error_output << "Failed to load render output: " << output.name << " " << output.type << std::endl;
				return false;
			}

			// initialize fbtexture
			int multisampling = (output.multisample < 0) ? fsaa : 0;
			FrameBufferTexture & fbtex = texture_outputs[output.name];
			fbtex.Init(
				output.width.GetSize(w), output.height.GetSize(h),
				target, format, (output.filter == "nearest"), output.mipmap,
				error_output, multisampling, (output.format == "depthshadow"));

			// register texture as input
			texture_inputs[output.name] = fbtex;
		}

		info_output << "Initialized render output: " << output.name;
		info_output << (output.type != "framebuffer" ? " (FBO)" : " (framebuffer alias)") << std::endl;
	}
	render_outputs["framebuffer"].RenderToFramebuffer(w, h);
	return true;
}

bool GraphicsGL2::SetupFrameBufferObjectsAndShaders(std::ostream & info_output, std::ostream & error_output)
{
	// gen graphics config shaders map
	std::map <std::string, const GraphicsConfigShader *> config_shaders;
	for (const auto & shader : config.shaders)
	{
		auto result = config_shaders.insert(std::make_pair(shader.name, &shader));
		if (!result.second)
			error_output << "Ignore duplicate definition of shader: " << shader.name << std::endl;
	}

	// setup frame buffer objects and shaders
	const std::vector <std::string> attributes(VertexAttrib::str, End(VertexAttrib::str));
	const std::vector <std::string> uniforms(Uniforms::str, End(Uniforms::str));
	std::vector <std::string> global_defines;
	GetShaderDefines(global_defines);
	for (const auto & pass : config.passes)
	{
		// check conditions
		if (!pass.conditions.Satisfied(conditions))
			continue;

		// load pass output
		const std::string & outname = pass.output;
		const std::vector <std::string> outputs = Tokenize(outname, " ");
		if (render_outputs.find(outname) == render_outputs.end())
		{
			// collect a list of textures for the outputs
			std::vector <FrameBufferTexture*> fbotex;
			for (const auto & output : outputs)
			{
				auto to = texture_outputs.find(output);
				if (to != texture_outputs.end())
				{
					fbotex.push_back(&to->second);
				}
			}
			if (fbotex.empty())
			{
				error_output << "None of these outputs are active: " << outname << std::endl;
				return false;
			}

			// initialize fbo
			FrameBufferObject & fbo = render_outputs[outname].RenderToFBO();
			fbo.Init(glstate, fbotex, error_output);
		}

		// load pass shader
		const std::string & shadername = pass.shader;
		if (shaders.find(shadername) == shaders.end())
		{
			auto csi = config_shaders.find(shadername);
			if (csi == config_shaders.end())
			{
				error_output << "Shader not defined: " << shadername << std::endl;
				return false;
			}
			const GraphicsConfigShader * cs = csi->second;

			std::vector <std::string> defines = Tokenize(cs->defines, " ");
			defines.reserve(defines.size() + global_defines.size());
			defines.insert(defines.end(), global_defines.begin(), global_defines.end());

			Shader & shader = shaders[cs->name];
			if (!shader.Load(
				glsl_330, outputs.size(),
				shaderpath + "/" + cs->vertex,
				shaderpath + "/" + cs->fragment,
				defines, uniforms, attributes,
				info_output, error_output))
			{
				return false;
			}
		}
	}
	return true;
}

void GraphicsGL2::SetupCameras(
	float fov,
	float view_distance,
	const Vec3 cam_position,
	const Quat & cam_rotation,
	const Vec3 & dynamic_reflection_sample_pos)
{
	{
		GraphicsCamera & cam = cameras["default"];
		cam.fov = fov;
		cam.pos = cam_position;
		cam.rot = cam_rotation;
		cam.view_distance = view_distance;
		cam.w = w;
		cam.h = h;
	}

	// create a camera for the skybox with a long view distance
	{
		GraphicsCamera & cam = cameras["skybox"];
		cam = cameras["default"];
		cam.view_distance = 10000;
		cam.pos = Vec3(0);
		if (fixed_skybox)
			cam.pos[2] = cam_position[2];
	}

	// create a camera for the dynamic reflections
	{
		GraphicsCamera & cam = cameras["dynamic_reflection"];
		cam.pos = dynamic_reflection_sample_pos;
		cam.fov = 90; // this gets automatically overridden with the correct fov (which is 90 anyway)
		cam.rot.LoadIdentity(); // this gets automatically rotated for each cube side
		cam.view_distance = 100;
		cam.w = 1; // this gets automatically overridden with the cubemap dimensions
		cam.h = 1; // this gets automatically overridden with the cubemap dimensions
	}

	// create a camera for the dynamic reflection skybox
	{
		GraphicsCamera & cam = cameras["dynamic_reflection_skybox"];
		cam = cameras["dynamic_reflection"];
		cam.view_distance = 10000;
		cam.pos = Vec3(0);
	}

	// create an ortho camera for 2d drawing
	{
		GraphicsCamera & cam = cameras["2d"];

		// this is the glOrtho call we want: glOrtho( 0, 1, 1, 0, -1, 1 );
		cam.fov = 0;
		cam.orthomin = Vec3(0, 1, -1);
		cam.orthomax = Vec3(1, 0, 1);
	}

	// create cameras for shadow passes
	if (shadows)
	{
		Mat4 view_matrix;
		cam_rotation.GetMatrix4(view_matrix);
		float translate[4] = {-cam_position[0], -cam_position[1], -cam_position[2], 0};
		view_matrix.MultiplyVector4(translate);
		view_matrix.Translate(translate[0], translate[1], translate[2]);

		Mat4 view_matrix_inv = view_matrix.Inverse();

		// derive light rotation quaternion from light direction vector
		Quat light_rotation;
		Vec3 up(0, 0, 1);
		float cosa = up.dot(light_direction);
		if (cosa * cosa < 1)
		{
			float a = -std::acos(cosa);
			Vec3 x = up.cross(light_direction).Normalize();
			light_rotation.SetAxisAngle(a, x[0], x[1], x[2]);
		}

		const std::string shadow_names[] = {"near", "medium", "far"};
		for (int i = 0; i < 3; i++)
		{
			float shadow_radius = (1<<i)*closeshadow+(i)*20; //5,30,60

			Vec3 shadowbox(1,1,1);
			shadowbox = shadowbox * (shadow_radius * std::sqrt(2.0f));
			Vec3 shadowoffset(0,0,-1);
			shadowoffset = shadowoffset * shadow_radius;
			(-cam_rotation).RotateVector(shadowoffset);
			shadowbox[2] += 60;

			GraphicsCamera & cam = cameras["shadows_"+shadow_names[i]];
			cam = cameras["default"];
			cam.fov = 0;
			cam.orthomin = -shadowbox;
			cam.orthomax = shadowbox;
			cam.pos = cam.pos + shadowoffset;
			cam.rot = light_rotation;

			// get camera clip matrix
			const Mat4 cam_proj_mat = GetProjMatrix(cam);
			const Mat4 cam_view_mat = GetViewMatrix(cam);

			Mat4 clip_matrix;
			clip_matrix.Scale(0.5f);
			clip_matrix.Translate(0.5f, 0.5f, 0.5f);
			clip_matrix = cam_proj_mat.Multiply(clip_matrix);
			clip_matrix = cam_view_mat.Multiply(clip_matrix);

			// premultiply the clip matrix with default camera view inverse matrix
			clip_matrix = view_matrix_inv.Multiply(clip_matrix);

			shadow_matrix[i] = clip_matrix;
		}

		assert(shadow_distance < 3);
		renderscene.SetShadowMatrix(shadow_matrix, shadow_distance + 1);
		postprocess.SetShadowMatrix(shadow_matrix, shadow_distance + 1);
	}
	else
	{
		renderscene.SetShadowMatrix(NULL, 0);
		postprocess.SetShadowMatrix(NULL, 0);
	}
}

void GraphicsGL2::ClearCulledDrawLists()
{
	for (auto & drawlist : culled_draw_lists)
	{
		drawlist.second.drawables.clear();
		drawlist.second.valid = false;
	}
}

bool GraphicsGL2::InitScenePass(
	const GraphicsConfigPass & pass_config,
	GraphicsPass & pass,
	std::ostream & error_output)
{
	// set state
	pass.blend_mode = BlendModeFromString(pass_config.blendmode);
	pass.depth_test = DepthModeFromString(pass_config.depthtest);
	pass.write_depth = pass_config.write_depth;
	pass.write_color = pass_config.write_color;
	pass.write_alpha = pass_config.write_alpha;
	pass.clear_depth = pass_config.clear_depth;
	pass.clear_color = pass_config.clear_color;
	pass.postprocess = (pass_config.draw.back() == "postprocess");
	pass.cull = pass_config.cull;

	// set textures
	GetScenePassInputTextures(pass_config.inputs, pass.textures);

	// set output
	auto oi = render_outputs.find(pass_config.output);
	if (oi == render_outputs.end())
	{
		error_output << "Render output " << pass_config.output << " couldn't be found";
		return false;
	}
	pass.output = &oi->second;

	// set shader
	auto si = shaders.find(pass_config.shader);
	if (si == shaders.end())
	{
		error_output << "Shader " << pass_config.shader << " couldn't be found";
		return false;
	}
	pass.shader = &si->second;

	// set camera
	std::string camera_name = pass_config.camera;
	auto bci = cameras.find(camera_name);
	if (bci == cameras.end())
	{
		error_output << "Camera " << camera_name << " couldn't be found";
		return false;
	}
	pass.camera = &bci->second;

	if (pass.postprocess)
		return true;

	// set sub-cameras and per camera draw lists
	auto & output = *pass.output;
	int cubesides = (output.IsFBO() && output.RenderToFBO().IsCubemap()) ? 6 : 1;
	for (int cubeside = 0; cubeside < cubesides; cubeside++)
	{
		if (cubesides > 1)
		{
			std::ostringstream s;
			s << pass_config.camera << "_cubeside" << cubeside;
			camera_name = s.str();

			GraphicsCamera & camera = cameras[camera_name];
			camera = *pass.camera;
			camera.rot = GetCubeSideOrientation(cubeside, camera.rot, error_output);
			camera.fov = 90;
			const auto & fbo = output.RenderToFBO();
			camera.w = fbo.GetWidth();
			camera.h = fbo.GetHeight();

			pass.sub_cameras[cubeside] = &camera;
		}

		for (const auto & draw_layer : pass_config.draw)
		{
			const std::string & draw_list_name = BuildKey(camera_name, draw_layer);
			pass.draw_lists.push_back(&culled_draw_lists[draw_list_name]);
		}
	}

	// set draw lists
	pass.static_draw_lists.reserve(pass_config.draw.size());
	pass.dynamic_draw_lists.reserve(pass_config.draw.size());
	for (const auto & draw_layer : pass_config.draw)
	{
		auto static_draw_list = static_draw_lists.GetByName(draw_layer);
		pass.static_draw_lists.push_back(&(static_draw_list.get()));
		if (!pass.static_draw_lists.back())
		{
			error_output << "Static drawables container " << draw_layer << " couldn't be found";
			return false;
		}

		auto dynamic_draw_list = dynamic_draw_lists.GetByName(draw_layer);
		pass.dynamic_draw_lists.push_back(&(dynamic_draw_list.get()));
		if (!pass.dynamic_draw_lists.back())
		{
			error_output << "Dynamic drawables container " << draw_layer << " couldn't be found";
			return false;
		}
	}

	return true;
}

void GraphicsGL2::CullScenePass(
	const GraphicsPass & pass,
	std::ostream & error_output)
{
	if (pass.postprocess)
		return;

	// for each pass, we have which camera and which draw layer to use
	// we want to do culling for each unique camera and draw layer combination
	auto & output = *pass.output;
	int cubesides = (output.IsFBO() && output.RenderToFBO().IsCubemap()) ? 6 : 1;
	for (int cubeside = 0; cubeside < cubesides; cubeside++)
	{
		auto cam = pass.camera;
		if (cubesides > 1)
		{
			// update sub-cameras
			auto sub_cam = pass.sub_cameras[cubeside];
			sub_cam->rot = GetCubeSideOrientation(cubeside, cam->rot, error_output);
			sub_cam->pos = cam->pos;
			cam = sub_cam;
		}

		Frustum frustum;
		frustum.Extract(GetProjMatrix(*cam).GetArray(), GetViewMatrix(*cam).GetArray());

		for (unsigned i = 0; i < pass.static_draw_lists.size(); i++)
		{
			auto & draw_list = *pass.draw_lists[i * cubesides + cubeside];
			if (draw_list.valid)
				continue;

			draw_list.valid = true;
			if (pass.cull)
			{
				if (cam->fov > 0)
				{
					float height = output.GetHeight();
					float fov = cam->fov * float(M_PI/180);
					float ct = ContributionCullThreshold(height, fov);
					auto cull = MakeFrustumCullerPersp(frustum.frustum, cam->pos, ct);

					// cull static drawlist
					pass.static_draw_lists[i]->Query(cull, draw_list.drawables);

					// cull dynamic drawlist
					for (const auto & drawable : *pass.dynamic_draw_lists[i])
					{
						if (!cull(drawable->GetCenter(), drawable->GetRadius()))
							draw_list.drawables.push_back(drawable);
					}
				}
				else
				{
					auto cull = MakeFrustumCuller(frustum.frustum);

					// cull static drawlist
					pass.static_draw_lists[i]->Query(cull, draw_list.drawables);

					// cull dynamic drawlist
					for (const auto & drawable : *pass.dynamic_draw_lists[i])
					{
						if (!cull(drawable->GetCenter(), drawable->GetRadius()))
							draw_list.drawables.push_back(drawable);
					}
				}
			}
			else
			{
				// copy static drawlist
				pass.static_draw_lists[i]->Query(
					Aabb<float>::IntersectAlways(),
					draw_list.drawables);

				// copy dynamic drawlist
				draw_list.drawables.insert(
					draw_list.drawables.end(),
					pass.dynamic_draw_lists[i]->begin(),
					pass.dynamic_draw_lists[i]->end());
			}
		}
	}
}

void GraphicsGL2::DrawScenePass(
	const GraphicsPass & pass,
	std::ostream & error_output)
{
	renderscene.SetShader(*pass.shader);
	renderscene.SetTextures(glstate, pass.textures, error_output);
	renderscene.SetColorMask(glstate, pass.write_color, pass.write_alpha);
	renderscene.SetDepthMode(glstate, pass.depth_test, pass.write_depth);
	renderscene.SetBlendMode(glstate, pass.blend_mode);
	renderscene.SetCamera(*pass.camera);

	auto & output = *pass.output;
	int cubesides = (output.IsFBO() && output.RenderToFBO().IsCubemap()) ? 6 : 1;
	for (int cubeside = 0; cubeside < cubesides; cubeside++)
	{
		if (cubesides > 1)
		{
			AttachCubeSide(cubeside, output.RenderToFBO(), error_output);
			renderscene.SetCamera(*pass.sub_cameras[cubeside]);
		}

		output.Begin(glstate, error_output);
		CheckForOpenGLErrors("render output begin", error_output);

		renderscene.ClearOutput(glstate, pass.clear_color, pass.clear_depth);

		for (unsigned i = 0; i < pass.static_draw_lists.size(); i++)
		{
			const auto & draw_list = *pass.draw_lists[i * cubesides + cubeside];
			if (!draw_list.drawables.empty())
			{
				renderscene.SetDrawList(draw_list.drawables);
				renderscene.Render(glstate, error_output);
			}
		}

		output.End(glstate, error_output);
		CheckForOpenGLErrors("render output end", error_output);
	}
}

void GraphicsGL2::DrawScenePassPost(
	const GraphicsPass & pass,
	std::ostream & error_output)
{
	assert(pass.postprocess);

	postprocess.SetShader(pass.shader);
	postprocess.SetTextures(glstate, pass.textures, error_output);
	postprocess.SetColorMask(glstate, pass.write_color, pass.write_alpha);
	postprocess.SetDepthMode(glstate, pass.depth_test, pass.write_depth);
	postprocess.SetBlendMode(glstate, pass.blend_mode);
	postprocess.SetCamera(*pass.camera);

	auto & output = *pass.output;

	output.Begin(glstate, error_output);

	CheckForOpenGLErrors("render output begin", error_output);

	postprocess.ClearOutput(glstate, pass.clear_color, pass.clear_depth);

	postprocess.Render(glstate, error_output);

	CheckForOpenGLErrors("render finish", error_output);

	output.End(glstate, error_output);

	CheckForOpenGLErrors("render output end", error_output);
}

void GraphicsGL2::GetScenePassInputTextures(
	const GraphicsConfigInputs & inputs,
	std::vector <TextureInterface*> & input_textures)
{
	for (const auto & t : inputs.tu)
	{
		unsigned int tuid = t.first;

		unsigned int cursize = input_textures.size();
		for (unsigned int extra = cursize; extra < tuid; extra++)
			input_textures.push_back(NULL);

		const std::string & texname = t.second;

		// quietly ignore invalid names
		// this allows us to specify outputs that are only present for certain conditions
		// and then always specify those outputs as inputs to later stages, and have
		// them be ignored if the conditions aren't met
		if (texture_inputs.find(texname) != texture_inputs.end())
		{
			input_textures.push_back(&*texture_inputs[texname]);
		}
		else
		{
			//TODO: decide if i want to do fancier error detection here to catch typos in render.conf
			//std::cout << "warning: " << texname << " not found" << std::endl;
			input_textures.push_back(NULL);
		}
	}
}
