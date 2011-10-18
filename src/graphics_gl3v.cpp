#include "graphics_gl3v.h"
#include "joeserialize.h"
#include "unordered_map.h"
#include "utils.h"
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <cctype>

#define enableContributionCull true

GRAPHICS_GL3V::GRAPHICS_GL3V(StringIdMap & map) :
	stringMap(map), renderer(gl), logNextGlFrame(false), initialized(false),
	closeshadow(5.f)
{
	// initialize the full screen quad
	fullscreenquadVertices.SetTo2DQuad(0,0,1,1, 0,1,1,0, 0);
	fullscreenquad.SetVertArray(&fullscreenquadVertices);
}

bool GRAPHICS_GL3V::Init(const std::string & shaderpath,
				unsigned int resx, unsigned int resy, unsigned int bpp,
				unsigned int depthbpp, bool fullscreen, bool shaders,
				unsigned int antialiasing, bool shadows,
				int shadow_distance, int shadow_quality,
				int reflection_type,
				const std::string & static_reflectionmap_file,
				const std::string & static_ambientmap_file,
				int anisotropy, const std::string & texturesize,
				int lighting_quality, bool bloom, bool normalmaps,
				const std::string & renderconfig,
				std::ostream & info_output, std::ostream & error_output)
{
	// first, see if we support the required gl version by attempting to initialize the GL wrapper
	gl.setInfoOutput(info_output);
	gl.setErrorOutput(error_output);
	if (!gl.initialize())
	{
		error_output << "Initialization of GL3 failed; that's OK, falling back to GL 1 or 2" << std::endl;
		return false;
	}

	// set up our graphical configuration option conditions
	bool fsaa = (antialiasing > 1);

	// add the conditions to the set
	#define ADDCONDITION(x) if (x) conditions.insert(#x)
	ADDCONDITION(bloom);
	ADDCONDITION(normalmaps);
	ADDCONDITION(fsaa);
	ADDCONDITION(shadows);
	#undef ADDCONDITION

	// load the reflection cubemap
	if (!static_reflectionmap_file.empty())
	{
		TEXTUREINFO t;
		t.cube = true;
		t.verticalcross = true;
		t.mipmap = true;
		t.anisotropy = anisotropy;
		t.size = texturesize;
		static_reflection.Load(static_reflectionmap_file, t, error_output);
	}

	// this information is needed to initialize the renderer in ReloadShaders
	w = resx;
	h = resy;

	// initialize the renderer
	bool success = ReloadShaders(shaderpath, info_output, error_output);
	initialized = success;
	return success;
}

void GRAPHICS_GL3V::Deinit()
{
	renderer.clear();
}

void GRAPHICS_GL3V::BeginScene(std::ostream & error_output)
{

}

DRAWABLE_CONTAINER <PTRVECTOR> & GRAPHICS_GL3V::GetDynamicDrawlist()
{
	return dynamic_drawlist;
}

GRAPHICS_GL3V::CameraMatrices & GRAPHICS_GL3V::setCameraPerspective(const std::string & name,
	const MATHVECTOR <float, 3> & position,
	const QUATERNION <float> & rotation,
	float fov,
	float nearDistance,
	float farDistance,
	float w,
	float h)
{
	CameraMatrices & matrices = cameras[name];

	// generate view matrix
	rotation.GetMatrix4(matrices.viewMatrix);
	MATHVECTOR <float, 3> rotated_cam_position = position;
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

GRAPHICS_GL3V::CameraMatrices & GRAPHICS_GL3V::setCameraOrthographic(const std::string & name,
	const MATHVECTOR <float, 3> & position,
	const QUATERNION <float> & rotation,
	const MATHVECTOR <float, 3> & orthoMin,
	const MATHVECTOR <float, 3> & orthoMax)
{
	CameraMatrices & matrices = cameras[name];

	// generate view matrix
	rotation.GetMatrix4(matrices.viewMatrix);
	MATHVECTOR <float, 3> rotated_cam_position = position;
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

void GRAPHICS_GL3V::SetupScene(float fov, float new_view_distance, const MATHVECTOR <float, 3> cam_position, const QUATERNION <float> & cam_rotation,
				const MATHVECTOR <float, 3> & dynamic_reflection_sample_pos)
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

	MATHVECTOR <float,3> skyboxCamPosition(0,0,0);
	setCameraPerspective("skybox",
		skyboxCamPosition,
		cam_rotation,
		fov,
		0.0001f,
		10000.f,
		w,
		h);

	// create a camera for 3d ui elements that has a fixed FOV
	setCameraPerspective("ui3d",
		cam_position,
		cam_rotation,
		45,
		nearDistance,
		new_view_distance,
		w,
		h);

	// shadow cameras
	for (int i = 0; i < 3; i++)
	{
		//float shadow_radius = (1<<i)*closeshadow+(i)*20.0; //5,30,60
		float shadow_radius = (1<<(2-i))*closeshadow+(2-i)*20.0;

		MATHVECTOR <float, 3> shadowbox(1,1,1);
		//shadowbox = shadowbox * (shadow_radius*sqrt(2.0));
		shadowbox = shadowbox * (shadow_radius*1.5);
		MATHVECTOR <float, 3> shadowoffset(0,0,-1);
		shadowoffset = shadowoffset * shadow_radius;
		(-cam_rotation).RotateVector(shadowoffset);
		if (i == 2)
			shadowbox[2] += 25.0;
		MATHVECTOR <float, 3> shadowPosition = cam_position+shadowoffset;

		// snap the shadow camera's location to shadow map texels
		// this can be commented out to minimize car aliasing at the expense of scenery aliasing
		const float shadowMapResolution = 512;
		float snapToGridSize = 2.f*shadowbox[0]/shadowMapResolution;
		MATHVECTOR <float, 3> cameraSpaceShadowPosition = shadowPosition;
		sunDirection.RotateVector(cameraSpaceShadowPosition);
		for (int n = 0; n < 3; n++)
		{
			float pos = cameraSpaceShadowPosition[n];
			float gridpos = pos / snapToGridSize;
			gridpos = floor(gridpos);
			cameraSpaceShadowPosition[n] = gridpos*snapToGridSize;
		}
		(-sunDirection).RotateVector(cameraSpaceShadowPosition);
		shadowPosition = cameraSpaceShadowPosition;

		std::string suffix = UTILS::tostr(i+1);

		CameraMatrices & shadowcam = setCameraOrthographic("shadow"+suffix,
			shadowPosition,
			sunDirection,
			-shadowbox,
			shadowbox);

		std::string matrixName = "shadowMatrix";

		// create and send shadow reconstruction matrices
		// the reconstruction matrix should transform from view to world, then from world to shadow view, then from shadow view to shadow clip space
		const CameraMatrices & defaultcam = cameras.find("default")->second;
		MATRIX4 <float> shadowReconstruction = defaultcam.inverseViewMatrix.Multiply(shadowcam.viewMatrix).Multiply(shadowcam.projectionMatrix);
		/*//MATRIX4 <float> shadowReconstruction = shadowcam.projectionMatrix.Multiply(shadowcam.viewMatrix.Multiply(defaultcam.inverseViewMatrix));
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

	// send inverse matrices for the default camera
	const CameraMatrices & defaultCamera = cameras.find("default")->second;
	renderer.setGlobalUniform(RenderUniformEntry(stringMap.addStringId("invProjectionMatrix"), defaultCamera.inverseProjectionMatrix.GetArray(),16));
	renderer.setGlobalUniform(RenderUniformEntry(stringMap.addStringId("invViewMatrix"), defaultCamera.inverseViewMatrix.GetArray(),16));
	renderer.setGlobalUniform(RenderUniformEntry(stringMap.addStringId("defaultViewMatrix"), defaultCamera.viewMatrix.GetArray(),16));

	// send sun light direction for the default camera

	// this computes the worldspace light direction
	// it is a little funky but at least matches what's done in GRAPHICS_FALLBACK::DrawScene
	// TODO: use a 3D vector for sun direction instead of a quaternion and read it from the track.txt
	MATHVECTOR <float, 3> lightDirection(0,0,1);
	(-sunDirection).RotateVector(lightDirection);

	// transform to eyespace (view space)
	MATHVECTOR <float, 4> lightDirection4;
	for (int i = 0; i < 3; i++)
		lightDirection4[i] = lightDirection[i];
	lightDirection4[3] = 0;
	defaultCamera.viewMatrix.MultiplyVector4(&lightDirection4[0]);

	// upload to the shaders
	RenderUniformEntry lightDirectionUniform(stringMap.addStringId("eyespaceLightDirection"), &lightDirection4[0], 3);
	renderer.setGlobalUniform(lightDirectionUniform);

	// set the reflection strength
	// TODO: read this from the track definition
	float reflectedLightColor[4];
	for (int i = 0; i < 3; i++)
		reflectedLightColor[i] = 1.0;
	reflectedLightColor[3] = 1.;
	renderer.setGlobalUniform(RenderUniformEntry(stringMap.addStringId("reflectedLightColor"), reflectedLightColor, 4));

	// set the ambient strength
	// TODO: read this from the track definition
	float ambientLightColor[4];
	for (int i = 0; i < 3; i++)
		ambientLightColor[i] = 1.5;
	ambientLightColor[3] = 1.;
	renderer.setGlobalUniform(RenderUniformEntry(stringMap.addStringId("ambientLightColor"), ambientLightColor, 4));

	// set the sun strength
	// TODO: read this from the track definition
	float directionalLightColor[4];
	for (int i = 0; i < 3; i++)
		directionalLightColor[i] = 1.4;
	directionalLightColor[3] = 1.;
	renderer.setGlobalUniform(RenderUniformEntry(stringMap.addStringId("directionalLightColor"), directionalLightColor, 4));
}

// returns true for cull, false for don't-cull
static bool contributionCull(const DRAWABLE * d, const MATHVECTOR <float, 3> & cam)
{
	const MATHVECTOR <float, 3> & obj = d->GetObjectCenter();
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
static bool frustumCull(const DRAWABLE * d, const FRUSTUM & frustum)
{
	float rd;
	const float bound = d->GetRadius();
	const MATHVECTOR <float, 3> & center = d->GetObjectCenter();

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
void GRAPHICS_GL3V::assembleDrawList(const std::vector <DRAWABLE*> & drawables, std::vector <RenderModelExternal*> & out, FRUSTUM * frustum, const MATHVECTOR <float, 3> & camPos)
{
	if (frustum && enableContributionCull)
	{
		for (std::vector <DRAWABLE*>::const_iterator i = drawables.begin(); i != drawables.end(); i++)
		{
			if (!frustumCull(*i, *frustum) && !contributionCull(*i, camPos))
				out.push_back(&(*i)->generateRenderModelData(gl, stringMap));
		}
	}
	else if (frustum)
	{
		for (std::vector <DRAWABLE*>::const_iterator i = drawables.begin(); i != drawables.end(); i++)
		{
			if (!frustumCull(*i, *frustum))
				out.push_back(&(*i)->generateRenderModelData(gl, stringMap));
		}
	}
	else
	{
		for (std::vector <DRAWABLE*>::const_iterator i = drawables.begin(); i != drawables.end(); i++)
		{
			out.push_back(&(*i)->generateRenderModelData(gl, stringMap));
		}
	}
}

// if frustum is NULL, don't do frustum or contribution culling
void GRAPHICS_GL3V::assembleDrawList(const AABB_SPACE_PARTITIONING_NODE_ADAPTER <DRAWABLE> & adapter, std::vector <RenderModelExternal*> & out, FRUSTUM * frustum, const MATHVECTOR <float, 3> & camPos)
{
	static std::vector <DRAWABLE*> queryResults;
	queryResults.clear();
	if (frustum)
		adapter.Query(*frustum, queryResults);
	else
		adapter.Query(AABB<float>::INTERSECT_ALWAYS(), queryResults);

	const std::vector <DRAWABLE*> & drawables = queryResults;

	if (frustum && enableContributionCull)
	{
		for (std::vector <DRAWABLE*>::const_iterator i = drawables.begin(); i != drawables.end(); i++)
		{
			if (!contributionCull(*i, camPos))
				out.push_back(&(*i)->generateRenderModelData(gl, stringMap));
		}
	}
	else
	{
		for (std::vector <DRAWABLE*>::const_iterator i = drawables.begin(); i != drawables.end(); i++)
		{
			out.push_back(&(*i)->generateRenderModelData(gl, stringMap));
		}
	}
}

// returns empty string if no camera
std::string GRAPHICS_GL3V::getCameraForPass(StringId pass) const
{
	std::string passString = stringMap.getString(pass);
	std::string cameraString;
	std::map <std::string, std::string>::const_iterator camIter = passNameToCameraName.find(passString);
	if (camIter != passNameToCameraName.end())
		cameraString = camIter->second;
	return cameraString;
}

std::string GRAPHICS_GL3V::getCameraDrawGroupKey(StringId pass, StringId group) const
{
	return getCameraForPass(pass)+"/"+stringMap.getString(group);
}

static bool SortDraworder(DRAWABLE * d1, DRAWABLE * d2)
{
	assert(d1 && d2);
	return (d1->GetDrawOrder() < d2->GetDrawOrder());
}

void GRAPHICS_GL3V::DrawScene(std::ostream & error_output)
{
	//sort the two dimentional drawlist so we get correct ordering
	std::sort(dynamic_drawlist.twodim.begin(),dynamic_drawlist.twodim.end(),&SortDraworder);

	// for each pass, we have which camera and which draw groups to use
	// we want to do culling for each unique camera and draw group combination
	// use "camera/group" as a unique key string
	// this is cached to avoid extra memory allocations each frame, so we need to clear old data
	for (std::map <std::string, std::vector <RenderModelExternal*> >::iterator i = cameraDrawGroupDrawLists.begin(); i != cameraDrawGroupDrawLists.end(); i++)
	{
		i->second.clear();
	}

	// because the cameraDrawGroupDrawLists are cached, this is how we keep track of which combinations
	// we have already generated
	std::set <std::string> cameraDrawGroupCombinationsGenerated;

	// this maps passes to maps of draw groups and draw list vector pointers
	// so drawMap[passName][drawGroup] is a pointer to a vector of RenderModelExternal pointers
	// this is complicated but it lets us do culling per camera position and draw group combination
	std::map <StringId, std::map <StringId, std::vector <RenderModelExternal*> *> > drawMap;

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

				std::vector <RenderModelExternal*> & outDrawList = cameraDrawGroupDrawLists[cameraDrawGroupKey];

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
					FRUSTUM frustum;
					FRUSTUM * frustumPtr = NULL;
					if (doCull)
					{
						frustum.Extract(&proj.data[0], &view.data[0]);
						frustumPtr = &frustum;
					}

					// assemble dynamic entries
					reseatable_reference <PTRVECTOR <DRAWABLE> > dynamicDrawablesPtr = dynamic_drawlist.GetByName(drawGroupString);
					if (dynamicDrawablesPtr)
					{
						const std::vector <DRAWABLE*> & dynamicDrawables = *dynamicDrawablesPtr;
						//assembleDrawList(dynamicDrawables, outDrawList, frustumPtr, lastCameraPosition);
						assembleDrawList(dynamicDrawables, outDrawList, NULL, lastCameraPosition); // TODO: the above line is commented out because frustum culling dynamic drawables doesen't work at the moment; is the object center in the drawable for the car in the correct space??
					}

					// assemble static entries
					reseatable_reference <AABB_SPACE_PARTITIONING_NODE_ADAPTER <DRAWABLE> > staticDrawablesPtr = static_drawlist.GetDrawlist().GetByName(drawGroupString);
					if (staticDrawablesPtr)
					{
						const AABB_SPACE_PARTITIONING_NODE_ADAPTER <DRAWABLE> & staticDrawables = *staticDrawablesPtr;
						assembleDrawList(staticDrawables, outDrawList, frustumPtr, lastCameraPosition);
					}

					// if it's requesting the full screen rect draw group, feed it our special drawable
					if (drawGroupString == "full screen rect")
					{
						std::vector <DRAWABLE*> rect;
						rect.push_back(&fullscreenquad);
						assembleDrawList(rect, outDrawList, NULL, lastCameraPosition);
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
	//std::cout << "normal_noblend: " << drawGroups[stringMap.addStringId("normal_noblend")].size() << "/" << static_drawlist.GetDrawlist().GetByName("normal_noblend")->size() << std::endl;

	// render!
	gl.logging(logNextGlFrame);
	renderer.render(w, h, stringMap, drawMap, error_output);
	gl.logging(false);

	logNextGlFrame = false;
}

void GRAPHICS_GL3V::EndScene(std::ostream & error_output)
{

}

int GRAPHICS_GL3V::GetMaxAnisotropy() const
{
	int max_anisotropy = 1;
	if (GLEW_EXT_texture_filter_anisotropic)
		gl.GetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anisotropy);
	return max_anisotropy;
}

bool GRAPHICS_GL3V::AntialiasingSupported() const
{
	return true;
}

bool GRAPHICS_GL3V::GetUsingShaders() const
{
	return true;
}

int upper(int c)
{
  return std::toupper((unsigned char)c);
}

bool GRAPHICS_GL3V::ReloadShaders(const std::string & shaderpath, std::ostream & info_output, std::ostream & error_output)
{
	// reinitialize the entire renderer
	std::vector <RealtimeExportPassInfo> passInfos;
	bool passInfosLoaded = joeserialize::LoadObjectFromFile("passList", shaderpath+"/gl3/vdrift1.rhr", passInfos, false, true, info_output, error_output);
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
				GRAPHICS_CONFIG_CONDITION condition;
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

		bool initSuccess = renderer.initialize(passInfos, stringMap, shaderpath+"/gl3", w, h, allcapsConditions, error_output);
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
			float viewportSize[2] = {w,h};
			RenderUniformEntry viewportSizeUniform(stringMap.addStringId("viewportSize"), viewportSize, 2);
			renderer.setGlobalUniform(viewportSizeUniform);

			// set static reflection texture
			if (static_reflection.Loaded())
			{
				assert(static_reflection.IsCube());
				renderer.setGlobalTexture(stringMap.addStringId("reflectionCube"), RenderTextureEntry(stringMap.addStringId("reflectionCube"), static_reflection.GetID(), GL_TEXTURE_CUBE_MAP));
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

void GRAPHICS_GL3V::AddStaticNode(SCENENODE & node, bool clearcurrent)
{
	static_drawlist.Generate(node, clearcurrent);
}

void GRAPHICS_GL3V::SetCloseShadow ( float value )
{
	closeshadow = value;
}

bool GRAPHICS_GL3V::GetShadows() const
{
	return true;
}

void GRAPHICS_GL3V::SetSunDirection ( const QUATERNION< float >& value )
{
	sunDirection = value;
}

void GRAPHICS_GL3V::SetContrast ( float value )
{

}
