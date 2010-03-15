#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include <string>
#include <ostream>
#include <map>
#include <list>
#include <vector>

#include "shader.h"
//#include "model_ase.h"
#include "mathvector.h"
#include "fbtexture.h"
#include "scenegraph.h"
#include "matrix4.h"
#include "texture.h"
#include "reseatable_reference.h"
#include "aabb_space_partitioning.h"

#include <SDL/SDL.h>

class SCENENODE;

class GRAPHICS_SDLGL
{
private:
	class GLSTATEMANAGER
	{
		private:
			std::vector <bool> used; //on modern compilers this should result in a lower memory usage bit_vector-type arrangement
			std::vector <bool> state;
			
			float r, g, b, a;
			bool depthmask;
			GLenum alphamode;
			float alphavalue;
			GLenum blendsource;
			GLenum blenddest;
			GLenum cullmode;
			
			void Set(int stateid, bool newval)
			{
				assert(stateid <= 65535);
				
				if (used[stateid])
				{
					if (state[stateid] != newval)
					{
						state[stateid] = newval;
						if (newval)
							glEnable(stateid);
						else
							glDisable(stateid);
					}
				}
				else
				{
					used[stateid] = true;
					state[stateid] = newval;
					if (newval)
						glEnable(stateid);
					else
						glDisable(stateid);
				}
			}
			
		public:
			GLSTATEMANAGER() : used(65536, false), state(65536), r(1),g(1),b(1),a(1), depthmask(true), alphamode(GL_NEVER), alphavalue(0), blendsource(GL_ZERO), blenddest(GL_ZERO), cullmode(GL_BACK)
			{
			}
			
			inline void Enable(int stateid)
			{
				Set(stateid, true);
			}
			
			inline void Disable(int stateid)
			{
				Set(stateid, false);
			}
			
			void SetColor(float nr, float ng, float nb, float na)
			{
				if (r != nr || g != ng || b != nb || a != na)
				{
					r=nr;g=ng;b=nb;a=na;
					glColor4f(r,g,b,a);
				}
			}
			
			void SetDepthMask(bool newdepthmask)
			{
				if (newdepthmask != depthmask)
				{
					depthmask = newdepthmask;
					glDepthMask(depthmask ? 1 : 0);
				}
			}
			
			void SetAlphaFunc(GLenum mode, float value)
			{
				if (mode != alphamode || value != alphavalue)
				{
					alphamode = mode;
					alphavalue = value;
					glAlphaFunc(mode, value);
				}
			}
			
			void SetBlendFunc(GLenum s, GLenum d)
			{
				if (blendsource != s || blenddest != d)
				{
					blendsource = s;
					blenddest = d;
					glBlendFunc(s, d);
				}
			}
			
			void SetCullFace(GLenum mode)
			{
				if (mode != cullmode)
				{
					cullmode = mode;
					glCullFace(cullmode);
				}
			}
	} glstate;
	
	///purely abstract base class
	class RENDER_INPUT
	{
		public:
			virtual void Render(GLSTATEMANAGER & glstate) = 0; ///< must be implemented by derived classes
	};
	
	class RENDER_INPUT_POSTPROCESS : public RENDER_INPUT
	{
		private:
			const FBTEXTURE_GL * source_texture;
			SHADER_GLSL * shader;
			
		public:
			RENDER_INPUT_POSTPROCESS() : source_texture(NULL), shader(NULL) {}
			
			void SetSourceTexture(FBTEXTURE_GL & newsource)
			{
				source_texture = &newsource;
			}
			
			void SetShader(SHADER_GLSL * newshader) {shader = newshader;}
			
			virtual void Render(GLSTATEMANAGER & glstate)
			{
				assert(source_texture);
				assert(shader);
				
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				
				shader->Enable();
				
				glMatrixMode(GL_PROJECTION);
				glPushMatrix();
				glLoadIdentity();
				glOrtho( 0, 1, 0, 1, -1, 1 );
				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				glLoadIdentity();
	
				glColor4f(1,1,1,1);
				glstate.Disable(GL_BLEND);
				glstate.Disable(GL_DEPTH_TEST);
				glstate.Enable(GL_TEXTURE_2D);
				
				glActiveTextureARB(GL_TEXTURE0_ARB);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
				glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
				source_texture->Activate();

				glBegin(GL_QUADS);
				glTexCoord2f(0.0f, 0.0f); glVertex3f( 0.0f,  0.0f,  0.0f);
				glTexCoord2f(1.0, 0.0f); glVertex3f( 1.0f,  0.0f,  0.0f);
				glTexCoord2f(1.0, 1.0); glVertex3f( 1.0f,  1.0f,  0.0f);
				glTexCoord2f(0.0f, 1.0); glVertex3f( 0.0f,  1.0f,  0.0f);
				glEnd();
	
				glMatrixMode(GL_PROJECTION);
				glPopMatrix();
				glMatrixMode(GL_MODELVIEW);
				glPopMatrix();

				glstate.Enable(GL_DEPTH_TEST);
				glstate.Disable(GL_TEXTURE_2D);
			}
	};
	
	class RENDER_INPUT_SCENE : public RENDER_INPUT
	{
		public:
			enum SHADER_TYPE
			{
				SHADER_SIMPLE,
    			SHADER_DISTANCEFIELD,
				SHADER_FULL,
				SHADER_FULLBLEND,
    			SHADER_SKYBOX,
				SHADER_NONE
			};
		
		private:
			reseatable_reference <std::vector <SCENEDRAW> > drawlist_static;
			reseatable_reference <std::vector <SCENEDRAW> > drawlist_dynamic;
			reseatable_reference <AABB_SPACE_PARTITIONING_NODE <SCENEDRAW*> > static_partitioning;
			std::vector <SCENEDRAW*> combined_drawlist_cache;
			bool last_transform_valid;
			MATRIX4 <float> last_transform;
			QUATERNION <float> cam_rotation; //used for the skybox effect
			MATHVECTOR <float, 3> cam_position;
			MATHVECTOR <float, 3> lightposition;
			MATHVECTOR <float, 3> orthomin;
			MATHVECTOR <float, 3> orthomax;
			float w, h;
			float camfov;
			float frustum[6][4]; //used for frustum culling
			float lod_far; //used for distance culling
			bool shaders;
			bool clearcolor, cleardepth;
			void SetActiveShader(const SHADER_TYPE & newshader);
			std::vector <SHADER_GLSL *> shadermap;
			SHADER_TYPE activeshader;
			reseatable_reference <TEXTURE_INTERFACE> reflection;
			reseatable_reference <TEXTURE_INTERFACE> ambient;
			bool orthomode;
			unsigned int fsaa;
			float contrast;
			bool use_static_partitioning;
			bool depth_mode_equal;
			
			void DrawList(GLSTATEMANAGER & glstate);
			bool FrustumCull(SCENEDRAW & tocull) const;
			void SelectAppropriateShader(SCENEDRAW & forme);
			void SelectFlags(SCENEDRAW & forme, GLSTATEMANAGER & glstate);
			void SelectTexturing(SCENEDRAW & forme, GLSTATEMANAGER & glstate);
			bool SelectTransformStart(SCENEDRAW & forme, GLSTATEMANAGER & glstate);
			void SelectTransformEnd(SCENEDRAW & forme, bool need_pop);
			void ExtractFrustum();
			unsigned int CombineDrawlists(); ///< returns the number of scenedraw elements that have already gone through culling
			
		public:
			RENDER_INPUT_SCENE();
			
			void SetDrawList(std::vector <SCENEDRAW> & dl_static, std::vector <SCENEDRAW> & dl_dynamic, AABB_SPACE_PARTITIONING_NODE <SCENEDRAW*> & static_speedup, bool useit)
			{
				drawlist_static = &dl_static;
				drawlist_dynamic = &dl_dynamic;
				static_partitioning = &static_speedup;
				use_static_partitioning = useit;
			}
			void DisableOrtho() {orthomode = false;}
			void SetOrtho(const MATHVECTOR <float, 3> & neworthomin, const MATHVECTOR <float, 3> & neworthomax) {orthomode = true; orthomin = neworthomin; orthomax = neworthomax;}
			void SetCameraInfo(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newrot, float newfov, float newlodfar, float neww, float newh);
			void SetSunDirection(const MATHVECTOR <float, 3> & newsun) {lightposition = newsun;}
			void SetFlags(bool newshaders) {shaders=newshaders;}
			void SetDefaultShader(SHADER_GLSL & newdefault) {shadermap.clear();shadermap.resize(SHADER_NONE, &newdefault);}
			void SetShader(SHADER_TYPE stype, SHADER_GLSL & newshader) {assert((unsigned int)stype < shadermap.size());shadermap[stype]=&newshader;}
			void SetClear(bool newclearcolor, bool newcleardepth) {clearcolor = newclearcolor;cleardepth = newcleardepth;}
			virtual void Render(GLSTATEMANAGER & glstate);
			void SetReflection ( TEXTURE_INTERFACE & value ) {reflection = value;}
			void SetFSAA ( unsigned int value ) {fsaa = value;}
			void SetAmbient ( TEXTURE_INTERFACE & value ) {ambient = value;}
			void SetContrast ( float value ) {contrast = value;}
			void SetDepthModeEqual ( bool value ) {depth_mode_equal = value;}
	};
	
	class RENDER_OUTPUT
	{
		private:
			FBTEXTURE_GL fbo;
			enum
			{
				RENDER_TO_FBO,
    				RENDER_TO_FRAMEBUFFER
			} target;
		
		public:
			RENDER_OUTPUT() : target(RENDER_TO_FRAMEBUFFER) {}
			
			///returns the FBO that the user should set up as necessary
			FBTEXTURE_GL & RenderToFBO()
			{
				target = RENDER_TO_FBO;
				return fbo;
			}
			
			void RenderToFramebuffer()
			{
				target = RENDER_TO_FRAMEBUFFER;
			}
			
			void Begin(std::ostream & error_output)
			{
				if (target == RENDER_TO_FBO)
					fbo.Begin(error_output);
			}
			
			void End(std::ostream & error_output)
			{
				if (target == RENDER_TO_FBO)
					fbo.End(error_output);
			}
	};
	
	int w, h;
	SDL_Surface * surface;
	bool initialized;
	bool using_shaders;
	GLint max_anisotropy;
	bool shadows;
	int shadow_distance;
	int shadow_quality;
	float closeshadow;
	enum {REFLECTION_DISABLED, REFLECTION_STATIC, REFLECTION_DYNAMIC} reflection_status;
	TEXTURE_GL static_reflection;
	TEXTURE_GL static_ambient;
	
	std::map <std::string, SHADER_GLSL> shadermap;
	std::map <std::string, SHADER_GLSL>::iterator activeshader;
	
	std::map < DRAWABLE_FILTER *, std::vector <SCENEDRAW> > dynamic_drawlist_map; //used for objects that move
	std::map < DRAWABLE_FILTER *, std::vector <SCENEDRAW> > static_drawlist_map; //used for objects that do not move
	std::map < DRAWABLE_FILTER *, AABB_SPACE_PARTITIONING_NODE <SCENEDRAW*> > static_object_partitioning; //used to speed up frustum checking for the static_drawlist_map

	std::list <DRAWABLE_FILTER *> filter_list;
	DRAWABLE_FILTER no2d_noblend;
	DRAWABLE_FILTER no2d_blend;
	DRAWABLE_FILTER only2d;
	DRAWABLE_FILTER skyboxes_noblend;
	DRAWABLE_FILTER skyboxes_blend;
	DRAWABLE_FILTER camtransfilter;
	
	//render pipeline info
	RENDER_INPUT_SCENE renderscene;
	//RENDER_OUTPUT scene_depthtexture;
	std::list <RENDER_OUTPUT> shadow_depthtexturelist;
	RENDER_OUTPUT final;
	RENDER_OUTPUT edgecontrastenhancement_depths;
	RENDER_OUTPUT full_scene_buffer;
	RENDER_OUTPUT bloom_buffer;
	RENDER_OUTPUT blur_buffer;
	
	float camfov;
	float view_distance;
	MATHVECTOR <float, 3> campos;
	QUATERNION <float> camorient;
	QUATERNION <float> lightdirection;
	
	unsigned int fsaa;
	int lighting; ///<lighting quality; see data/settings/options.config for definition of values
	bool bloom;
	float contrast;
	bool aticard;
	
	void ChangeDisplay(const int width, const int height, const int bpp, const int dbpp, const bool fullscreen, 
			   unsigned int antialiasing, std::ostream & info_output, std::ostream & error_output);
	void SetActiveShader(const std::string name);
	bool LoadShader(const std::string & shaderpath, const std::string & name, std::ostream & info_output, std::ostream & error_output, std::string variant="", std::string variant_defines="");
	void EnableShaders(const std::string & shaderpath, std::ostream & info_output, std::ostream & error_output);
	void DisableShaders();
	void DrawBox(const MATHVECTOR <float, 3> & corner1, const MATHVECTOR <float, 3> & corner2) const;
	void SetupCamera();
	void SendDrawlistToRenderScene(RENDER_INPUT_SCENE & renderscene, DRAWABLE_FILTER * filter_ptr);
	
	void Render(RENDER_INPUT * input, RENDER_OUTPUT & output, std::ostream & error_output);
public:
	GRAPHICS_SDLGL() : surface(NULL),initialized(false),using_shaders(false),max_anisotropy(0),shadows(false),
		       	closeshadow(5.0), reflection_status(REFLECTION_DISABLED),camfov(45),
			view_distance(10000),fsaa(1),lighting(0),bloom(false),contrast(1.0), aticard(false)
			{activeshader = shadermap.end();}
	~GRAPHICS_SDLGL() {}
	
	///reflection_type is 0 (low=OFF), 1 (medium=static), 2 (high=dynamic)
	void Init(const std::string shaderpath, const std::string & windowcaption,
				unsigned int resx, unsigned int resy, unsigned int bpp,
				unsigned int depthbpp, bool fullscreen, bool shaders,
				unsigned int antialiasing, bool enableshadows,
				int shadow_distance, int shadow_quality,
				int reflection_type,
				const std::string & static_reflectionmap_file,
				const std::string & static_ambientmap_file,
				int anisotropy, const std::string & texturesize,
				int lighting_quality, bool newbloom,
				std::ostream & info_output, std::ostream & error_output);
	void Deinit();
	void BeginScene(std::ostream & error_output);
	std::map < DRAWABLE_FILTER *, std::vector <SCENEDRAW> > & GetStaticDrawlistmap() {return static_drawlist_map;}
	std::map < DRAWABLE_FILTER *, std::vector <SCENEDRAW> > & GetDrawlistmap() {return dynamic_drawlist_map;}
	void OptimizeStaticDrawlistmap(); ///<should be called after filling the static drawlist map
	void ClearStaticDrawlistMap()
	{
		for (std::map <DRAWABLE_FILTER *, std::vector <SCENEDRAW> >::iterator i =
			GetStaticDrawlistmap().begin(); i != GetStaticDrawlistmap().end(); ++i)

			i->second.clear();

		OptimizeStaticDrawlistmap();
	}
	void SetupScene(float fov, float new_view_distance, const MATHVECTOR <float, 3> cam_position, const QUATERNION <float> & cam_rotation)
	{
		camfov = fov;
		campos = cam_position;
		camorient = cam_rotation;
		view_distance = new_view_distance;
	}
	void DrawScene(std::ostream & error_output);
	void EndScene(std::ostream & error_output);
	void Screenshot(std::string filename);
	int GetW() const {return w;}
	int GetH() const {return h;}
	float GetWHRatio() const {return (float)w/h;}
	int GetMaxAnisotropy() const {return max_anisotropy;}
	bool AntialiasingSupported() {return GLEW_ARB_multisample;}

	bool GetUsingShaders() const
	{
		return using_shaders;
	}
	
	bool ReloadShaders(const std::string & shaderpath, std::ostream & info_output, std::ostream & error_output);

	void SetCloseShadow ( float value )
	{
		closeshadow = value;
	}

	bool GetShadows() const
	{
		return shadows;
	}

	void SetSunDirection ( const QUATERNION< float >& value )
	{
		lightdirection = value;
	}

	void SetContrast ( float value )
	{
		contrast = value;
	}
	
	bool HaveATICard() {return aticard;}
};

#endif

