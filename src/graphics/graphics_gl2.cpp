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
#include "sky.h"

/// array end ptr
template <typename T, size_t N>
static T * End(T (&ar)[N])
{
	return ar + N;
}

/// break up the input into a vector of strings using the token characters given
std::vector <std::string> Tokenize(const std::string & input, const std::string & tokens)
{
	std::vector <std::string> out;

	unsigned int pos = 0;
	unsigned int lastpos = 0;

	while (pos != (unsigned int) std::string::npos)
	{
		pos = input.find_first_of(tokens, pos);
		std::string thisstr = input.substr(lastpos,pos-lastpos);
		if (!thisstr.empty())
			out.push_back(thisstr);
		pos = input.find_first_not_of(tokens, pos);
		lastpos = pos;
	}

	return out;
}

static void ReportOnce(const void * id, const std::string & message, std::ostream & output)
{
	static std::map <const void*, std::string> prev_messages;

	std::map <const void*, std::string>::iterator i = prev_messages.find(id);
	if (i == prev_messages.end() || i->second != message)
	{
		prev_messages[id] = message;
		output << message << std::endl;
	}
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

static bool Cull(const Frustum & f, const Drawable & d)
{
	const float radius = d.GetRadius();
	Vec3 center = d.GetObjectCenter();
	d.GetTransform().TransformVectorOut(center[0], center[1], center[2]);
	for (int i = 0; i < 6; i++)
	{
		const float rd =
			f.frustum[i][0] * center[0] +
			f.frustum[i][1] * center[1] +
			f.frustum[i][2] * center[2] +
			f.frustum[i][3];
		if (rd <= -radius)
			return true;
	}
	return false;
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
	sky_dynamic(false)
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
	unsigned resx, unsigned resy, unsigned depthbpp,
	bool fullscreen, unsigned antialiasing,
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
	node.Traverse(dynamic_drawlist, identity);
}

void GraphicsGL2::AddStaticNode(SceneNode & node)
{
	Mat4 identity;
	node.Traverse(static_drawlist, identity);
	static_drawlist.ForEach(OptimizeFunctor());
}

void GraphicsGL2::ClearDynamicDrawables()
{
	dynamic_drawlist.clear();
}

void GraphicsGL2::ClearStaticDrawables()
{
	static_drawlist.clear();
}

void GraphicsGL2::SetupScene(
	float fov, float new_view_distance,
	const Vec3 cam_position,
	const Quat & cam_rotation,
	const Vec3 & dynamic_reflection_sample_pos,
	std::ostream & error_output)
{
	// setup the default camera from the passed-in parameters
	{
		GraphicsCamera & cam = cameras["default"];
		cam.fov = fov;
		cam.pos = cam_position;
		cam.rot = cam_rotation;
		cam.view_distance = new_view_distance;
		cam.w = w;
		cam.h = h;
	}

	// create a camera for the skybox with a long view distance
	{
		GraphicsCamera & cam = cameras["skybox"];
		cam = cameras["default"];
		cam.view_distance = 10000;
		cam.pos = Vec3(0);
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
		cam.orthomode = true;
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
		if (cosa * cosa < 1.0f)
		{
			float a = -acosf(cosa);
			Vec3 x = up.cross(light_direction).Normalize();
			light_rotation.SetAxisAngle(a, x[0], x[1], x[2]);
		}

		std::vector <std::string> shadow_names;
		shadow_names.push_back("near");
		shadow_names.push_back("medium");
		shadow_names.push_back("far");

		for (int i = 0; i < 3; i++)
		{
			float shadow_radius = (1<<i)*closeshadow+(i)*20.0; //5,30,60

			Vec3 shadowbox(1,1,1);
			shadowbox = shadowbox * (shadow_radius*sqrt(2.0));
			Vec3 shadowoffset(0,0,-1);
			shadowoffset = shadowoffset * shadow_radius;
			(-cam_rotation).RotateVector(shadowoffset);
			shadowbox[2] += 60.0;

			GraphicsCamera & cam = cameras["shadows_"+shadow_names[i]];
			cam = cameras["default"];
			cam.orthomode = true;
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

	// sort the two dimentional drawlist so we get correct ordering
	std::sort(dynamic_drawlist.twodim.begin(), dynamic_drawlist.twodim.end(), &SortDraworder);

	// do fast culling queries for static geometry per pass
	ClearCulledDrawLists();
	for (std::vector <GraphicsConfigPass>::const_iterator i = config.passes.begin(); i != config.passes.end(); i++)
	{
		CullScenePass(*i, error_output);
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
	for (std::vector <GraphicsConfigPass>::const_iterator i = config.passes.begin(); i != config.passes.end(); i++)
	{
		assert(!i->draw.empty());
		if (i->draw.back() != "postprocess")
			DrawScenePass(*i, error_output);
		else
			DrawScenePassPost(*i, error_output);
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
	ShaderMap::iterator it = shaders.find(name);
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

	// setup frame buffer textures
	const bool has_texture_float = GLC_ARB_texture_float && GLC_ARB_half_float_pixel;
	for (std::vector <GraphicsConfigOutput>::const_iterator i = config.outputs.begin(); i != config.outputs.end(); i++)
	{
		if (!i->conditions.Satisfied(conditions))
			continue;

		if (texture_outputs.find(i->name) != texture_outputs.end())
		{
			error_output << "Ignore duplicate definiion of output: " << i->name << std::endl;
			continue;
		}

		if (i->type == "framebuffer")
		{
			render_outputs[i->name].RenderToFramebuffer(w, h);
		}
		else
		{
			FrameBufferTexture::Target target = TextureTargetFromString(i->type);
			FrameBufferTexture::Format format = TextureFormatFromString(i->format);
			if (!has_texture_float && (format == FrameBufferTexture::RGBA16 || format == FrameBufferTexture::RGB16))
			{
				error_output << "Your video card doesn't support floating point textures." << std::endl;
				error_output << "Failed to load render output: " << i->name << " " << i->type << std::endl;
				return false;
			}

			// initialize fbtexture
			int multisampling = (i->multisample < 0) ? fsaa : 0;
			FrameBufferTexture & fbtex = texture_outputs[i->name];
			fbtex.Init(
				i->width.GetSize(w), i->height.GetSize(h),
				target, format, (i->filter == "nearest"), i->mipmap,
				error_output, multisampling, (i->format == "depthshadow"));

			// register texture as input
			texture_inputs[i->name] = fbtex;
		}

		info_output << "Initialized render output: " << i->name;
		info_output << (i->type != "framebuffer" ? " (FBO)" : " (framebuffer alias)") << std::endl;
	}
	render_outputs["framebuffer"].RenderToFramebuffer(w, h);

	// gen graphics config shaders map
	typedef std::map <std::string, const GraphicsConfigShader *> ConfigShaderMap;
	ConfigShaderMap config_shaders;
	for (std::vector <GraphicsConfigShader>::const_iterator s = config.shaders.begin(); s != config.shaders.end(); s++)
	{
		std::pair <ConfigShaderMap::iterator, bool> result = config_shaders.insert(std::make_pair(s->name, &*s));
		if (!result.second)
			error_output << "Ignore duplicate definition of shader: " << s->name << std::endl;
	}

	// setup frame buffer objects and shaders
	const std::vector <std::string> attributes(VertexAttrib::str, End(VertexAttrib::str));
	const std::vector <std::string> uniforms(Uniforms::str, End(Uniforms::str));
	std::vector <std::string> global_defines;
	GetShaderDefines(global_defines);
	for (std::vector <GraphicsConfigPass>::const_iterator i = config.passes.begin(); i != config.passes.end(); i++)
	{
		// check conditions
		if (!i->conditions.Satisfied(conditions))
			continue;

		// load pass output
		const std::string & outname = i->output;
		const std::vector <std::string> outputs = Tokenize(outname, " ");
		if (render_outputs.find(outname) == render_outputs.end())
		{
			// collect a list of textures for the outputs
			std::vector <FrameBufferTexture*> fbotex;
			for (std::vector <std::string>::const_iterator o = outputs.begin(); o != outputs.end(); o++)
			{
				TextureOutputMap::iterator to = texture_outputs.find(*o);
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
		const std::string & shadername = i->shader;
		if (shaders.find(shadername) == shaders.end())
		{
			ConfigShaderMap::const_iterator csi = config_shaders.find(shadername);
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

void GraphicsGL2::ClearCulledDrawLists()
{
	for(CulledDrawListMap::iterator i = culled_drawlists.begin(); i != culled_drawlists.end(); i++)
	{
		i->second.drawables.clear();
		i->second.valid = false;
	}
}

void GraphicsGL2::CullScenePass(
	const GraphicsConfigPass & pass,
	std::ostream & error_output)
{
	// for each pass, we have which camera and which draw layer to use
	// we want to do culling for each unique camera and draw layer combination
	// use camera/layer as the unique key
	assert(!pass.draw.empty());

	if (pass.draw.back() == "postprocess" || !pass.conditions.Satisfied(conditions))
		return;

	for (std::vector <std::string>::const_iterator d = pass.draw.begin(); d != pass.draw.end(); d++)
	{
		// determine if we're dealing with a cubemap
		RenderOutputMap::iterator oi = render_outputs.find(pass.output);
		if (oi == render_outputs.end())
		{
			ReportOnce(&pass, "Render output "+pass.output+" couldn't be found", error_output);
			return;
		}

		const bool cubemap = (oi->second.IsFBO() && oi->second.RenderToFBO().IsCubemap());
		const int cubesides = cubemap ? 6 : 1;
		std::string cameraname = pass.camera;
		for (int cubeside = 0; cubeside < cubesides; cubeside++)
		{
			if (cubemap)
			{
				// build a name for the sub camera
				{
					std::ostringstream s;
					s << pass.camera << "_cubeside" << cubeside;
					cameraname = s.str();
				}

				// get the base camera
				CameraMap::iterator bci = cameras.find(pass.camera);
				if (bci == cameras.end())
				{
					ReportOnce(&pass, "Camera " + pass.camera + " couldn't be found", error_output);
					return;
				}

				// create our sub-camera
				GraphicsCamera & cam = cameras[cameraname];
				cam = bci->second;

				// set the sub-camera's properties
				cam.rot = GetCubeSideOrientation(cubeside, cam.rot, error_output);
				cam.fov = 90;
				assert(oi->second.IsFBO());
				const FrameBufferObject & fbo = oi->second.RenderToFBO();
				cam.w = fbo.GetWidth();
				cam.h = fbo.GetHeight();
			}

			const std::string drawlist_name = BuildKey(cameraname, *d);
			CulledDrawList & drawlist = culled_drawlists[drawlist_name];
			if (drawlist.valid)
				break;

			drawlist.valid = true;
			if (pass.cull)
			{
				// cull static drawlist
				reseatable_reference <AabbTreeNodeAdapter <Drawable> > container = static_drawlist.GetByName(*d);
				if (!container)
				{
					ReportOnce(&pass, "Drawable container " + *d + " couldn't be found", error_output);
					return;
				}

				CameraMap::iterator ci = cameras.find(cameraname);
				if (ci == cameras.end())
				{
					ReportOnce(&pass, "Camera " + cameraname + " couldn't be found", error_output);
					return;
				}
				const GraphicsCamera & cam = ci->second;

				Frustum frustum;
				frustum.Extract(GetProjMatrix(cam).GetArray(), GetViewMatrix(cam).GetArray());

				container->Query(frustum, drawlist.drawables);

				// cull dynamic drawlist
				reseatable_reference <PtrVector <Drawable> > container_dynamic = dynamic_drawlist.GetByName(*d);
				if (!container_dynamic)
				{
					ReportOnce(&pass, "Drawable container " + *d + " couldn't be found", error_output);
					return;
				}
				for (size_t i = 0; i < container_dynamic->size(); ++i)
				{
					if (!Cull(frustum, *(*container_dynamic)[i]))
						drawlist.drawables.push_back((*container_dynamic)[i]);
				}
			}
			else
			{
				// copy static drawlist
				reseatable_reference <AabbTreeNodeAdapter <Drawable> > container = static_drawlist.GetByName(*d);
				if (!container)
				{
					ReportOnce(&pass, "Drawable container " + *d + " couldn't be found", error_output);
					return;
				}
				container->Query(Aabb<float>::IntersectAlways(), drawlist.drawables);

				// copy dynamic drawlist
				reseatable_reference <PtrVector <Drawable> > container_dynamic = dynamic_drawlist.GetByName(*d);
				if (!container_dynamic)
				{
					ReportOnce(&pass, "Drawable container " + *d + " couldn't be found", error_output);
					return;
				}
				drawlist.drawables.insert(
					drawlist.drawables.end(),
					container_dynamic->begin(),
					container_dynamic->end());
			}
		}
	}
}

void GraphicsGL2::DrawScenePass(
	const GraphicsConfigPass & pass,
	std::ostream & error_output)
{
	// log failure here?
	if (!pass.conditions.Satisfied(conditions))
		return;

	// setup shader
	ShaderMap::iterator si = shaders.find(pass.shader);
	if (si == shaders.end())
	{
		ReportOnce(&pass, "Shader " + pass.shader + " couldn't be found", error_output);
		return;
	}
	renderscene.SetShader(si->second);

	// setup textures
	std::vector <TextureInterface*> input_textures;
	GetScenePassInputTextures(pass.inputs, input_textures);
	renderscene.SetTextures(glstate, input_textures, error_output);

	// setup state
	renderscene.SetColorMask(glstate, pass.write_color, pass.write_alpha);
	renderscene.SetDepthMode(glstate, DepthModeFromString(pass.depthtest), pass.write_depth);
	renderscene.SetBlendMode(glstate, BlendModeFromString(pass.blendmode));

	// setup output
	RenderOutputMap::iterator oi = render_outputs.find(pass.output);
	if (oi == render_outputs.end())
	{
		ReportOnce(&pass, "Render output " + pass.output + " couldn't be found", error_output);
		return;
	}
	RenderOutput & output = oi->second;

	// handle the cubemap case
	const bool cubemap = (output.IsFBO() && output.RenderToFBO().IsCubemap());
	const int cubesides = cubemap ? 6 : 1;
	std::string cameraname = pass.camera;
	for (int cubeside = 0; cubeside < cubesides; cubeside++)
	{
		if (cubemap)
		{
			// build a name for the sub camera
			std::ostringstream s;
			s << pass.camera << "_cubeside" << cubeside;
			cameraname = s.str();

			// attach the correct cube side on the render output
			AttachCubeSide(cubeside, output.RenderToFBO(), error_output);
		}

		// setup camera
		CameraMap::iterator ci = cameras.find(cameraname);
		if (ci == cameras.end())
		{
			ReportOnce(&pass, "Camera " + pass.camera + " couldn't be found", error_output);
			return;
		}
		renderscene.SetCamera(ci->second);

		// render pass draw layers
		output.Begin(glstate, error_output);
		renderscene.ClearOutput(glstate, pass.clear_color, pass.clear_depth);
		for (std::vector <std::string>::const_iterator d = pass.draw.begin(); d != pass.draw.end(); d++)
		{
			const std::string drawlist_name = BuildKey(cameraname, *d);
			CulledDrawListMap::const_iterator drawlist_it = culled_drawlists.find(drawlist_name);
			if (drawlist_it == culled_drawlists.end())
			{
				ReportOnce(&pass, "Couldn't find culled static drawlist for camera/draw combination: " + drawlist_name, error_output);
				return;
			}

			if (!drawlist_it->second.drawables.empty())
			{
				renderscene.SetDrawList(drawlist_it->second.drawables);
				renderscene.Render(glstate, error_output);
			}
		}
		output.End(glstate, error_output);
		CheckForOpenGLErrors("render output end", error_output);
	}
}

void GraphicsGL2::DrawScenePassPost(
	const GraphicsConfigPass & pass,
	std::ostream & error_output)
{
	assert(pass.draw.back() == "postprocess");

	if (!pass.conditions.Satisfied(conditions))
		return;

	ShaderMap::iterator si = shaders.find(pass.shader);
	if (si == shaders.end())
	{
		ReportOnce(&pass, "Shader " + pass.shader + " couldn't be found", error_output);
		return;
	}
	postprocess.SetShader(&si->second);

	std::vector <TextureInterface*> input_textures;
	GetScenePassInputTextures(pass.inputs, input_textures);
	postprocess.SetTextures(glstate, input_textures, error_output);

	postprocess.SetColorMask(glstate, pass.write_color, pass.write_alpha);
	postprocess.SetDepthMode(glstate, DepthModeFromString(pass.depthtest), pass.write_depth);
	postprocess.SetBlendMode(glstate, BlendModeFromString(pass.blendmode));

	RenderOutputMap::iterator oi = render_outputs.find(pass.output);
	if (oi == render_outputs.end())
	{
		ReportOnce(&pass, "Render output " + pass.output + " couldn't be found", error_output);
		return;
	}
	RenderOutput & output = oi->second;

	// setup camera, even though we don't use it directly for the post process
	// we want to have some info available
	std::string cameraname = pass.camera;
	CameraMap::iterator ci = cameras.find(cameraname);
	if (ci == cameras.end())
	{
		ReportOnce(&pass, "Camera " + cameraname + " couldn't be found", error_output);
		return;
	}
	postprocess.SetCamera(ci->second);

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
	for (std::map <unsigned int, std::string>::const_iterator t = inputs.tu.begin(); t != inputs.tu.end(); t++)
	{
		unsigned int tuid = t->first;

		unsigned int cursize = input_textures.size();
		for (unsigned int extra = cursize; extra < tuid; extra++)
			input_textures.push_back(NULL);

		const std::string & texname = t->second;

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
