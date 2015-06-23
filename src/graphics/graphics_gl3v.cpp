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

#include "graphics_gl3v.h"
#include "scenenode.h"
#include "joeserialize.h"
#include "utils.h"

#include <unordered_map>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <cctype>

#define enableContributionCull true

GraphicsGL3::GraphicsGL3(StringIdMap & map) :
	stringMap(map),
	gl(vertex_buffer),
	renderer(gl),
	logNextGlFrame(false),
	initialized(false),
	fixed_skybox(true),
	closeshadow(5.f)
{
	// initialize the full screen quad
	fullscreenquadVertices.SetTo2DQuad(0,0,1,1, 0,1,1,0, 0);
	fullscreenquad.SetVertArray(&fullscreenquadVertices);
}

GraphicsGL3::~GraphicsGL3()
{
	// dtor
}

bool GraphicsGL3::Init(
	const std::string & shader_path,
	unsigned resx,
	unsigned resy,
	unsigned antialiasing,
	bool shadows,
	int /*shadow_distance*/,
	int /*shadow_quality*/,
	int reflection_type,
	const std::string & static_reflectionmap_file,
	const std::string & /*static_ambientmap_file*/,
	int anisotropy,
	int texturesize,
	int /*lighting_quality*/,
	bool bloom,
	bool normalmaps,
	bool /*dynamicsky*/,
	const std::string & render_config,
	std::ostream & info_output,
	std::ostream & error_output)
{
	rendercfg = render_config;
	shaderpath = shader_path;

	// first, see if we support the required gl version by attempting to initialize the GL wrapper
	gl.setInfoOutput(info_output);
	gl.setErrorOutput(error_output);
	if (!gl.initialize())
	{
		error_output << "Initialization of GL3 failed." << std::endl;
		return false;
	}

	#ifdef _WIN32
	// workaround for broken vao implementation Intel/Windows
	{
		const std::string vendor = (const char*)glGetString(GL_VENDOR);
		if (vendor == "Intel")
			vertex_buffer.BindElementBufferExplicitly();
	}
	#endif

	// set up our graphical configuration option conditions
	bool fsaa = (antialiasing > 1);

	// add the conditions to the set
	#define ADDCONDITION(x) if (x) conditions.insert(#x)
	ADDCONDITION(bloom);
	ADDCONDITION(normalmaps);
	ADDCONDITION(fsaa);
	ADDCONDITION(shadows);
	#undef ADDCONDITION

	if (reflection_type >= 1)
		conditions.insert("reflections_low");
	if (reflection_type >= 2)
		conditions.insert("reflections_high");

	// load the reflection cubemap
	if (!static_reflectionmap_file.empty())
	{
		TextureInfo t;
		t.cube = true;
		t.verticalcross = true;
		t.mipmap = true;
		t.anisotropy = anisotropy;
		t.maxsize = TextureInfo::Size(texturesize);
		static_reflection.Load(static_reflectionmap_file, t, error_output);
	}

	// this information is needed to initialize the renderer in ReloadShaders
	w = resx;
	h = resy;

	// initialize the renderer
	bool success = ReloadShaders(info_output, error_output);
	initialized = success;
	return success;
}

void GraphicsGL3::Deinit()
{
	renderer.clear();
}

void GraphicsGL3::BindDynamicVertexData(std::vector<SceneNode*> nodes)
{
	// TODO: This doesn't look very efficient...
	SceneNode quad_node;
	SceneNode::DrawableHandle d = quad_node.GetDrawList().twodim.insert(fullscreenquad);
	nodes.push_back(&quad_node);

	vertex_buffer.SetDynamicVertexData(&nodes[0], nodes.size());

	fullscreenquad = quad_node.GetDrawList().twodim.get(d);
}

void GraphicsGL3::BindStaticVertexData(std::vector<SceneNode*> nodes)
{
	vertex_buffer.SetStaticVertexData(&nodes[0], nodes.size());
}

void GraphicsGL3::AddDynamicNode(SceneNode & node)
{
	Mat4 identity;
	node.Traverse(dynamic_drawlist, identity);
}

void GraphicsGL3::AddStaticNode(SceneNode & node)
{
	Mat4 identity;
	node.Traverse(static_drawlist, identity);
	static_drawlist.ForEach(OptimizeFunctor());
}

void GraphicsGL3::ClearDynamicDrawables()
{
	dynamic_drawlist.clear();
}

void GraphicsGL3::ClearStaticDrawables()
{
	static_drawlist.clear();
}

GraphicsGL3::CameraMatrices & GraphicsGL3::setCameraPerspective(const std::string & name,
	const Vec3 & position,
	const Quat & rotation,
	float fov,
	float nearDistance,
	float farDistance,
	float w,
	float h)
{
	CameraMatrices & matrices = cameras[name];

	// generate view matrix
	rotation.GetMatrix4(matrices.viewMatrix);
	Vec3 rotated_cam_position = position;
	rotation.RotateVector(rotated_cam_position);
	matrices.viewMatrix.Translate(-rotated_cam_position[0],-rotated_cam_position[1],-rotated_cam_position[2]);

	// generate projection matrix
	matrices.projectionMatrix.Perspective(fov, w/(float)h, nearDistance, farDistance);

	// generate inverse projection matrix
	matrices.inverseProjectionMatrix.InvPerspective(fov, w/(float)h, nearDistance, farDistance);

	// generate inverse view matrix
	matrices.inverseViewMatrix = matrices.viewMatrix.Inverse();

	return matrices;
}

GraphicsGL3::CameraMatrices & GraphicsGL3::setCameraOrthographic(const std::string & name,
	const Vec3 & position,
	const Quat & rotation,
	const Vec3 & orthoMin,
	const Vec3 & orthoMax)
{
	CameraMatrices & matrices = cameras[name];

	// generate view matrix
	rotation.GetMatrix4(matrices.viewMatrix);
	Vec3 rotated_cam_position = position;
	rotation.RotateVector(rotated_cam_position);
	matrices.viewMatrix.Translate(-rotated_cam_position[0],-rotated_cam_position[1],-rotated_cam_position[2]);

	// generate inverse view matrix
	matrices.inverseViewMatrix = matrices.viewMatrix.Inverse();

	// generate projection matrix
	matrices.projectionMatrix.SetOrthographic(orthoMin[0], orthoMax[0], orthoMin[1], orthoMax[1], orthoMin[2], orthoMax[2]);

	// generate inverse projection matrix
	matrices.inverseProjectionMatrix = matrices.projectionMatrix.Inverse();

	return matrices;
}

void GraphicsGL3::SetupScene(
	float fov, float new_view_distance,
	const Vec3 cam_position,
	const Quat & cam_rotation,
	const Vec3 & /*dynamic_reflection_sample_pos*/,
	std::ostream & error_output)
{
	lastCameraPosition = cam_position;

	const float nearDistance = 0.1;

	setCameraPerspective("default",
		cam_position,
		cam_rotation,
		fov,
		nearDistance,
		new_view_distance,
		w,
		h);

	Vec3 skyboxCamPosition(0,0,0);
	if (fixed_skybox)
		skyboxCamPosition[2] = cam_position[2];

	setCameraPerspective("skybox",
		skyboxCamPosition,
		cam_rotation,
		fov,
		nearDistance,
		10000.f,
		w,
		h);

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

	// shadow cameras
	for (int i = 0; i < 3; i++)
	{
		//float shadow_radius = (1<<i)*closeshadow+(i)*20.0; //5,30,60
		float shadow_radius = (1<<(2-i))*closeshadow+(2-i)*20.0;

		Vec3 shadowbox(1,1,1);
		//shadowbox = shadowbox * (shadow_radius*sqrt(2.0));
		shadowbox = shadowbox * (shadow_radius*1.5);
		Vec3 shadowoffset(0,0,-1);
		shadowoffset = shadowoffset * shadow_radius;
		(-cam_rotation).RotateVector(shadowoffset);
		if (i == 2)
			shadowbox[2] += 25.0;
		Vec3 shadowPosition = cam_position+shadowoffset;

		// snap the shadow camera's location to shadow map texels
		// this can be commented out to minimize car aliasing at the expense of scenery aliasing
		const float shadowMapResolution = 512;
		float snapToGridSize = 2.f*shadowbox[0]/shadowMapResolution;
		Vec3 cameraSpaceShadowPosition = shadowPosition;
		light_rotation.RotateVector(cameraSpaceShadowPosition);
		for (int n = 0; n < 3; n++)
		{
			float pos = cameraSpaceShadowPosition[n];
			float gridpos = pos / snapToGridSize;
			gridpos = floor(gridpos);
			cameraSpaceShadowPosition[n] = gridpos*snapToGridSize;
		}
		(-light_rotation).RotateVector(cameraSpaceShadowPosition);
		shadowPosition = cameraSpaceShadowPosition;

		std::string suffix = Utils::tostr(i+1);

		CameraMatrices & shadowcam = setCameraOrthographic("shadow"+suffix,
			shadowPosition,
			light_rotation,
			-shadowbox,
			shadowbox);

		std::string matrixName = "shadowMatrix";

		// create and send shadow reconstruction matrices
		// the reconstruction matrix should transform from view to world, then from world to shadow view, then from shadow view to shadow clip space
		const CameraMatrices & defaultcam = cameras.find("default")->second;
		Mat4 shadowReconstruction = defaultcam.inverseViewMatrix.Multiply(shadowcam.viewMatrix).Multiply(shadowcam.projectionMatrix);
		/*//Mat4 shadowReconstruction = shadowcam.projectionMatrix.Multiply(shadowcam.viewMatrix.Multiply(defaultcam.inverseViewMatrix));
		std::cout << "shadowcam.projectionMatrix: " << std::endl;
		shadowcam.projectionMatrix.DebugPrint(std::cout);
		std::cout << "defaultcam.inverseViewMatrix: " << std::endl;
		defaultcam.inverseViewMatrix.DebugPrint(std::cout);
		std::cout << "shadowcam.viewMatrix: " << std::endl;
		shadowcam.viewMatrix.DebugPrint(std::cout);
		std::cout << "defaultcam.inverseViewMatrix.Multiply(shadowcam.viewMatrix): " << std::endl;
		defaultcam.inverseViewMatrix.Multiply(shadowcam.viewMatrix).DebugPrint(std::cout);
		std::cout << matrixName << ":" << std::endl;
		shadowReconstruction.DebugPrint(std::cout);*/

		//renderer.setGlobalUniform(RenderUniformEntry(stringMap.addStringId(matrixName), shadowReconstruction.GetArray(),16));

		// examine the user-defined fields to find out which shadow matrix to send to a pass
		std::vector <StringId> passes = renderer.getPassNames();
		for (std::vector <StringId>::const_iterator i = passes.begin(); i != passes.end(); i++)
		{
			std::map <std::string, std::string> fields = renderer.getUserDefinedFields(*i);
			std::map <std::string, std::string>::const_iterator field = fields.find(matrixName);
			if (field != fields.end() && field->second == suffix)
			{
				renderer.setPassUniform(*i, RenderUniformEntry(stringMap.addStringId(matrixName), shadowReconstruction.GetArray(),16));
			}
		}
	}

	// send cameras to passes
	for (std::map <std::string, std::string>::const_iterator i = passNameToCameraName.begin(); i != passNameToCameraName.end(); i++)
	{
		renderer.setPassUniform(stringMap.addStringId(i->first), RenderUniformEntry(stringMap.addStringId("viewMatrix"), cameras[i->second].viewMatrix.GetArray(),16));
		renderer.setPassUniform(stringMap.addStringId(i->first), RenderUniformEntry(stringMap.addStringId("projectionMatrix"), cameras[i->second].projectionMatrix.GetArray(),16));
	}

	// send matrices for the default camera
	const CameraMatrices & defaultCamera = cameras.find("default")->second;
	renderer.setGlobalUniform(RenderUniformEntry(stringMap.addStringId("invProjectionMatrix"), defaultCamera.inverseProjectionMatrix.GetArray(),16));
	renderer.setGlobalUniform(RenderUniformEntry(stringMap.addStringId("invViewMatrix"), defaultCamera.inverseViewMatrix.GetArray(),16));
	renderer.setGlobalUniform(RenderUniformEntry(stringMap.addStringId("defaultViewMatrix"), defaultCamera.viewMatrix.GetArray(),16));
	renderer.setGlobalUniform(RenderUniformEntry(stringMap.addStringId("defaultProjectionMatrix"), defaultCamera.projectionMatrix.GetArray(),16));

	// send sun light direction for the default camera

	// transform to eyespace (view space)
	MathVector <float, 4> lightDirection4;
	for (int i = 0; i < 3; i++)
		lightDirection4[i] = light_direction[i];
	lightDirection4[3] = 0;
	defaultCamera.viewMatrix.MultiplyVector4(&lightDirection4[0]);

	// upload to the shaders
	RenderUniformEntry lightDirectionUniform(stringMap.addStringId("eyespaceLightDirection"), &lightDirection4[0], 3);
	renderer.setGlobalUniform(lightDirectionUniform);

	// set the reflection strength
	// TODO: read this from the track definition
	float reflectedLightColor[4];
	for (int i = 0; i < 3; i++)
		reflectedLightColor[i] = 0.5;
	reflectedLightColor[3] = 1.;
	renderer.setGlobalUniform(RenderUniformEntry(stringMap.addStringId("reflectedLightColor"), reflectedLightColor, 4));

	// set the ambient strength
	// TODO: read this from the track definition
	float ambientLightColor[4];
	for (int i = 0; i < 3; i++)
		ambientLightColor[i] = 1.56;
	ambientLightColor[3] = 1.;
	renderer.setGlobalUniform(RenderUniformEntry(stringMap.addStringId("ambientLightColor"), ambientLightColor, 4));

	// set the sun strength
	// TODO: read this from the track definition
	float directionalLightColor[4];
	for (int i = 0; i < 3; i++)
		directionalLightColor[i] = 8.3;
	directionalLightColor[3] = 1.;
	renderer.setGlobalUniform(RenderUniformEntry(stringMap.addStringId("directionalLightColor"), directionalLightColor, 4));

	AssembleDrawMap(error_output);
}

// returns empty string if no camera
std::string GraphicsGL3::getCameraForPass(StringId pass) const
{
	std::string passString = stringMap.getString(pass);
	std::string cameraString;
	std::map <std::string, std::string>::const_iterator camIter = passNameToCameraName.find(passString);
	if (camIter != passNameToCameraName.end())
		cameraString = camIter->second;
	return cameraString;
}

std::string GraphicsGL3::getCameraDrawGroupKey(StringId pass, StringId group) const
{
	return getCameraForPass(pass)+"/"+stringMap.getString(group);
}

static bool SortDraworder(Drawable * d1, Drawable * d2)
{
	assert(d1 && d2);
	return (d1->GetDrawOrder() < d2->GetDrawOrder());
}

// returns true for cull, false for don't-cull
static bool contributionCull(const Drawable * d, const Vec3 & cam)
{
	const Vec3 & obj = d->GetObjectCenter();
	float radius = d->GetRadius();
	float dist2 = (obj - cam).MagnitudeSquared();
	const float fov = 90; // rough field-of-view estimation
	float numerator = 2*radius*fov;
	const float pixelThreshold = 1;
	//float pixels = numerator*numerator/dist2; // perspective divide (we square the numerator because we're using squared distance)
	//if (pixels < pixelThreshold)
	if (numerator*numerator < dist2*pixelThreshold)
		return true;
	else
		return false;
}

// returns true for cull, false for don't-cull
static bool frustumCull(const Drawable * d, const Frustum & frustum)
{
	float rd;
	const float bound = d->GetRadius();
	const Vec3 & center = d->GetObjectCenter();

	for (int i=0; i<6; i++)
	{
		rd=frustum.frustum[i][0]*center[0]+
				frustum.frustum[i][1]*center[1]+
				frustum.frustum[i][2]*center[2]+
				frustum.frustum[i][3];
		if (rd < -bound)
		{
			return true;
		}
	}

	return false;
}

// if frustum is NULL, don't do frustum or contribution culling
void GraphicsGL3::AssembleDrawList(const std::vector <Drawable*> & drawables, std::vector <RenderModelExt*> & out, Frustum * frustum, const Vec3 & camPos)
{
	if (frustum && enableContributionCull)
	{
		for (std::vector <Drawable*>::const_iterator i = drawables.begin(); i != drawables.end(); i++)
		{
			if (!frustumCull(*i, *frustum) && !contributionCull(*i, camPos))
				out.push_back(&(*i)->GenRenderModelData(stringMap));
		}
	}
	else if (frustum)
	{
		for (std::vector <Drawable*>::const_iterator i = drawables.begin(); i != drawables.end(); i++)
		{
			if (!frustumCull(*i, *frustum))
				out.push_back(&(*i)->GenRenderModelData(stringMap));
		}
	}
	else
	{
		for (std::vector <Drawable*>::const_iterator i = drawables.begin(); i != drawables.end(); i++)
		{
			out.push_back(&(*i)->GenRenderModelData(stringMap));
		}
	}
}

// if frustum is NULL, don't do frustum or contribution culling
void GraphicsGL3::AssembleDrawList(const AabbTreeNodeAdapter <Drawable> & adapter, std::vector <RenderModelExt*> & out, Frustum * frustum, const Vec3 & camPos)
{
	static std::vector <Drawable*> queryResults;
	queryResults.clear();
	if (frustum)
		adapter.Query(*frustum, queryResults);
	else
		adapter.Query(Aabb<float>::IntersectAlways(), queryResults);

	const std::vector <Drawable*> & drawables = queryResults;

	if (frustum && enableContributionCull)
	{
		for (std::vector <Drawable*>::const_iterator i = drawables.begin(); i != drawables.end(); i++)
		{
			if (!contributionCull(*i, camPos))
				out.push_back(&(*i)->GenRenderModelData(stringMap));
		}
	}
	else
	{
		for (std::vector <Drawable*>::const_iterator i = drawables.begin(); i != drawables.end(); i++)
		{
			out.push_back(&(*i)->GenRenderModelData(stringMap));
		}
	}
}

void GraphicsGL3::AssembleDrawMap(std::ostream & /*error_output*/)
{
	//sort the two dimentional drawlist so we get correct ordering
	std::sort(dynamic_drawlist.twodim.begin(),dynamic_drawlist.twodim.end(),&SortDraworder);

	drawMap.clear();

	// for each pass, we have which camera and which draw groups to use
	// we want to do culling for each unique camera and draw group combination
	// use "camera/group" as a unique key string
	// this is cached to avoid extra memory allocations each frame, so we need to clear old data
	for (std::map <std::string, std::vector <RenderModelExt*> >::iterator i = cameraDrawGroupDrawLists.begin(); i != cameraDrawGroupDrawLists.end(); i++)
	{
		i->second.clear();
	}

	// because the cameraDrawGroupDrawLists are cached, this is how we keep track of which combinations
	// we have already generated
	std::set <std::string> cameraDrawGroupCombinationsGenerated;

	// for each pass, do culling of the dynamic and static drawlists and put the results into the cameraDrawGroupDrawLists
	std::vector <StringId> passes = renderer.getPassNames();
	for (std::vector <StringId>::const_iterator i = passes.begin(); i != passes.end(); i++)
	{
		if (renderer.getPassEnabled(*i))
		{
			const std::set <StringId> & passDrawGroups = renderer.getDrawGroups(*i);
			for (std::set <StringId>::const_iterator g = passDrawGroups.begin(); g != passDrawGroups.end(); g++)
			{
				StringId passName = *i;
				StringId drawGroupName = *g;
				std::string drawGroupString = stringMap.getString(drawGroupName);
				std::string cameraDrawGroupKey = getCameraDrawGroupKey(passName, drawGroupName);

				std::vector <RenderModelExt*> & outDrawList = cameraDrawGroupDrawLists[cameraDrawGroupKey];

				// see if we have already generated this combination
				if (cameraDrawGroupCombinationsGenerated.find(cameraDrawGroupKey) == cameraDrawGroupCombinationsGenerated.end())
				{
					// we need to generate this combination

					// extract frustum information
					RenderUniform proj, view;
					bool doCull = true;
					/*if (getCameraForPass(passName).empty())
					{
						doCull = false;
					}
					else*/
					{
						doCull = !(!doCull || !renderer.getPassUniform(passName, stringMap.addStringId("viewMatrix"), view));
						doCull = !(!doCull || !renderer.getPassUniform(passName, stringMap.addStringId("projectionMatrix"), proj));
					}
					Frustum frustum;
					Frustum * frustumPtr = NULL;
					if (doCull)
					{
						frustum.Extract(&proj.data[0], &view.data[0]);
						frustumPtr = &frustum;
					}

					// assemble dynamic entries
					reseatable_reference <PtrVector <Drawable> > dynamicDrawablesPtr = dynamic_drawlist.GetByName(drawGroupString);
					if (dynamicDrawablesPtr)
					{
						const std::vector <Drawable*> & dynamicDrawables = *dynamicDrawablesPtr;
						//AssembleDrawList(dynamicDrawables, outDrawList, frustumPtr, lastCameraPosition);
						AssembleDrawList(dynamicDrawables, outDrawList, NULL, lastCameraPosition); // TODO: the above line is commented out because frustum culling dynamic drawables doesen't work at the moment; is the object center in the drawable for the car in the correct space??
					}

					// assemble static entries
					reseatable_reference <AabbTreeNodeAdapter <Drawable> > staticDrawablesPtr = static_drawlist.GetByName(drawGroupString);
					if (staticDrawablesPtr)
					{
						const AabbTreeNodeAdapter <Drawable> & staticDrawables = *staticDrawablesPtr;
						AssembleDrawList(staticDrawables, outDrawList, frustumPtr, lastCameraPosition);
					}

					// if it's requesting the full screen rect draw group, feed it our special drawable
					if (drawGroupString == "full screen rect")
					{
						std::vector <Drawable*> rect;
						rect.push_back(&fullscreenquad);
						AssembleDrawList(rect, outDrawList, NULL, lastCameraPosition);
					}
				}

				// use the generated combination in our drawMap
				drawMap[passName][drawGroupName] = &outDrawList;

				cameraDrawGroupCombinationsGenerated.insert(cameraDrawGroupKey);
			}
		}
	}

	/*for (std::map <std::string, std::vector <RenderModelExternal*> >::iterator i = cameraDrawGroupDrawLists.begin(); i != cameraDrawGroupDrawLists.end(); i++)
	{
		std::cout << i->first << ": " << i->second.size() << std::endl;
	}
	std::cout << "----------" << std::endl;*/
	//if (enableContributionCull) std::cout << "Contribution cull count: " << assembler.contributionCullCount << std::endl;
	//std::cout << "normal_noblend: " << drawGroups[stringMap.addStringId("normal_noblend")].size() << "/" << static_drawlist.GetDrawList().GetByName("normal_noblend")->size() << std::endl;
}

void GraphicsGL3::DrawScene(std::ostream & error_output)
{
	// reset active vertex array in case it has been modified outside
	gl.unbindVertexArray();

	gl.logging(logNextGlFrame);
	renderer.render(w, h, stringMap, drawMap, error_output);
	gl.logging(false);

	logNextGlFrame = false;
}

int GraphicsGL3::GetMaxAnisotropy() const
{
	int max_anisotropy = 1;
	if (GLC_EXT_texture_filter_anisotropic)
		gl.GetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anisotropy);
	return max_anisotropy;
}

bool GraphicsGL3::AntialiasingSupported() const
{
	return true;
}

int upper(int c)
{
  return std::toupper((unsigned char)c);
}

bool GraphicsGL3::ReloadShaders(std::ostream & info_output, std::ostream & error_output)
{
	// reinitialize the entire renderer
	std::vector <RealtimeExportPassInfo> passInfos;
	bool passInfosLoaded = joeserialize::LoadObjectFromFile("passList", shaderpath+"/"+rendercfg, passInfos, false, true, info_output, error_output);
	if (passInfosLoaded)
	{
		// strip pass infos from the list that we pass to the renderer if they are disabled
		int origPassInfoSize = passInfos.size();
		for (int i = origPassInfoSize - 1; i >= 0; i--)
		{
			std::map <std::string, std::string> & fields = passInfos[i].userDefinedFields;
			std::map <std::string, std::string>::const_iterator field = fields.find("conditions");
			if (field != fields.end())
			{
				GraphicsConfigCondition condition;
				condition.Parse(field->second);
				if (!condition.Satisfied(conditions))
				{
					passInfos.erase(passInfos.begin()+i);
				}
			}
		}

		std::set <std::string> allcapsConditions;
		for (std::set <std::string>::const_iterator i = conditions.begin(); i != conditions.end(); i++)
		{
			std::string s = *i;
			std::transform(s.begin(), s.end(), s.begin(), upper);
			allcapsConditions.insert(s);
		}

		bool initSuccess = renderer.initialize(passInfos, stringMap, shaderpath, w, h, allcapsConditions, error_output);
		if (initSuccess)
		{
			// assign cameras to each pass
			std::vector <StringId> passes = renderer.getPassNames();
			for (std::vector <StringId>::const_iterator i = passes.begin(); i != passes.end(); i++)
			{
				std::map <std::string, std::string> fields = renderer.getUserDefinedFields(*i);
				std::map <std::string, std::string>::const_iterator field = fields.find("camera");
				if (field != fields.end())
					passNameToCameraName[stringMap.getString(*i)] = field->second;
			}

			// set viewport size
			float viewportSize[2] = {float(w), float(h)};
			RenderUniformEntry viewportSizeUniform(stringMap.addStringId("viewportSize"), viewportSize, 2);
			renderer.setGlobalUniform(viewportSizeUniform);

			// set static reflection texture
			if (static_reflection.GetId())
			{
				renderer.setGlobalTexture(stringMap.addStringId("reflectionCube"), RenderTextureEntry(stringMap.addStringId("reflectionCube"), static_reflection.GetId(), GL_TEXTURE_CUBE_MAP));
			}

			if (initialized)
				renderer.printRendererStatus(VERBOSITY_MAXIMUM, stringMap, std::cout);
		}
		else
		{
			error_output << "Initialization of GL3 renderer failed; that's OK, falling back to GL 1 or 2" << std::endl;
			return false;
		}
	}
	else
	{
		error_output << "Unable to load GL3 pass information; that's OK, falling back to GL 1 or 2" << std::endl;
		return false;
	}

	info_output << "GL3 initialization successful" << std::endl;

	if (initialized)
		logNextGlFrame = true;

	return true;
}

void GraphicsGL3::SetCloseShadow ( float value )
{
	closeshadow = value;
}

bool GraphicsGL3::GetShadows() const
{
	return true;
}

void GraphicsGL3::SetFixedSkybox(bool enable)
{
	fixed_skybox = enable;
}

void GraphicsGL3::SetSunDirection(const Vec3 & value)
{
	light_direction = value;
}

void GraphicsGL3::SetContrast(float /*value*/)
{

}
