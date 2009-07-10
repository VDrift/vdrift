#ifndef _TRACK_H
#define _TRACK_H

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <list>
#include <memory>
#include <vector>

#include "model_joe03.h"
#include "texture.h"
#include "joepack.h"
#include "scenegraph.h"
#include "track_object.h"
#include "tracksurfacetype.h"
#include "mathvector.h"
#include "quaternion.h"
#include "collision_detection.h"
#include "bezier.h"
#include "aabb.h"
#include "aabb_space_partitioning.h"
#include "k1999.h"
#include "optional.h"

class ROADPATCH
{
	private:
		BEZIER patch;
		float track_curvature;
		MATHVECTOR <float, 3> racing_line;
		
		DRAWABLE * racingline_draw;
		VERTEXARRAY racingline_vertexarray;
		
	public:
		ROADPATCH() : track_curvature(0),racingline_draw(NULL) {}
		
		const BEZIER & GetPatch() const {return patch;}
		BEZIER & GetPatch() {return patch;}
		
		///return true if the ray starting at the given origin going in the given direction intersects this patch.
		/// output the contact point and normal to the given outtri and normal variables.
		bool Collide(const MATHVECTOR <float, 3> & origin, const MATHVECTOR <float, 3> & direction, float seglen, MATHVECTOR <float, 3> & outtri, MATHVECTOR <float, 3> & normal) const;

		float GetTrackCurvature() const
		{
			return track_curvature;
		}
	
		MATHVECTOR< float, 3 > GetRacingLine() const
		{
			return racing_line;
		}

		void SetTrackCurvature ( float value )
		{
			track_curvature = value;
		}
	
		void SetRacingLine ( const MATHVECTOR< float, 3 >& value )
		{
			racing_line = value;
			patch.racing_line = value;
			patch.have_racingline = true;
		}
		
		void AddRacinglineScenenode(SCENENODE * node, ROADPATCH * nextpatch, 
					    TEXTURE_GL & racingline_texture, std::ostream & error_output);
};

class ROADSTRIP
{
	private:
		std::list <ROADPATCH> patches;
		AABB_SPACE_PARTITIONING_NODE <ROADPATCH *> aabb_part;
		bool closed;
		
		void GenerateSpacePartitioning();
		
	public:
		ROADSTRIP() : closed(false) {}
		
		bool ReadFrom(std::istream & openfile, std::ostream & error_output);
		bool Collide(const MATHVECTOR <float, 3> & origin, const MATHVECTOR <float, 3> & direction, float seglen, MATHVECTOR <float, 3> & outtri, const BEZIER * & colpatch, MATHVECTOR <float, 3> & normal) const;
		void Reverse();
		const std::list <ROADPATCH> & GetPatchList() const {return patches;}
		std::list <ROADPATCH> & GetPatchList() {return patches;}
		
		void CreateRacingLine(SCENENODE * parentnode, 
				      TEXTURE_GL & racingline_texture, std::ostream & error_output)
		{
			for (std::list <ROADPATCH>::iterator i = patches.begin(); i != patches.end(); ++i)
			{
				std::list <ROADPATCH>::iterator n = i;
				n++;
				ROADPATCH * nextpatch(NULL);
				if (n != patches.end())
					nextpatch = &(*n);
				i->AddRacinglineScenenode(parentnode, nextpatch, racingline_texture, error_output);
			}
		}

		bool GetClosed() const
		{
			return closed;
		}
	
		///either returns a const BEZIER * to the roadpatch at the given (positive or negative) offset from the supplied bezier (looping around if necessary) or does not return a value if the bezier is not found in this roadstrip.
		optional <const BEZIER *> FindBezierAtOffset(const BEZIER * bezier, int offset=0) const;
};

class TRACK
{
	private:
		typedef TEXTURE_GL TEXTURE;
		
		std::string texture_size;
		std::map <std::string, MODEL_JOE03> model_library;
		std::map <std::string, TEXTURE> texture_library;
		std::list <TRACK_OBJECT> objects;
		bool vertical_tracking_skyboxes;
		// does the track use Surface types
		bool usesurfaces;
		std::vector <TRACKSURFACE> tracksurfaces;
		std::vector <std::pair <MATHVECTOR <float, 3>, QUATERNION <float> > > start_positions;
		enum
		{
			DIRECTION_FORWARD,
   			DIRECTION_REVERSE
		} direction;
		
		//road information
		std::list <ROADSTRIP> roads;
		
		//the sequence of beziers that a car needs to hit to do a lap
		std::vector <const BEZIER *> lapsequence;
		
		//racing line data
		SCENENODE * racingline_node;
		TEXTURE_GL racingline_texture;
		bool CreateRacingLines(SCENENODE * parentnode, 
				       const std::string & texturepath, const std::string & texsize, std::ostream & error_output)
		{
			assert(parentnode);
			if (!racingline_node)
			{
				racingline_node = &parentnode->AddNode();
			}
			
			if (!racingline_texture.Loaded())
			{
				TEXTUREINFO tex; 
				tex.SetName(texturepath + "/racingline.png");
				if (!racingline_texture.Load(tex, error_output, texsize))
					return false;
			}
			
			K1999 k1999data;
			for (std::list <ROADSTRIP>::iterator i = roads.begin(); i != roads.end(); ++i)
			{
				if (k1999data.LoadData(&(*i)))
				{
					k1999data.CalcRaceLine();
					k1999data.UpdateRoadStrip(&(*i));
				}
				//else std::cerr << "Didn't load racingline data" << std::endl;
				
				i->CreateRacingLine(racingline_node, racingline_texture, error_output);
			}
			
			return true;
		}
		
		bool LoadParameters(const std::string & trackpath, std::ostream & info_output, std::ostream & error_output);
		bool LoadSurfaces(const std::string & trackpath, std::ostream & info_output, std::ostream & error_output);
		bool LoadObjects(const std::string & trackpath, SCENENODE & sceneroot, int anisotropy, std::ostream & info_output, std::ostream & error_output);
		
		class OBJECTLOADER
		{
			private:
				///read from the file stream and put it in "output".
				/// return true if the get was successful, else false
				template <typename T>
				bool GetParam(std::ifstream & f, T & output)
				{
					if (!f.good())
						return false;
			
					std::string instr;
					f >> instr;
					if (instr.empty())
						return false;
			
					while (!instr.empty() && instr[0] == '#' && f.good())
					{
						f.ignore(1024, '\n');
						f >> instr;
					}
			
					if (!f.good() && !instr.empty() && instr[0] == '#')
						return false;
			
					std::stringstream sstr(instr);
					sstr >> output;
					return true;
				}
				
				const std::string & trackpath;
				std::string objectpath;
				SCENENODE & sceneroot;
				SCENENODE unoptimized_scene;
				std::ostream & info_output;
				std::ostream & error_output;
				
				JOEPACK pack;
				std::ifstream objectfile;
				
				bool error;
				int numobjects;
				bool packload;
				int anisotropy;
				bool cull;
				
				int params_per_object;
				const int expected_params;
				const int min_params;
				
				bool dynamicshadowsenabled;
				bool agressivecombine;
				
				void CalculateNumObjects();
			
			public:
				OBJECTLOADER(const std::string & ntrackpath, SCENENODE & nsceneroot, int nanisotropy, bool newdynamicshadowsenabled, std::ostream & ninfo_output,
					std::ostream & nerror_output, bool newcull, bool doagressivecombining) : trackpath(ntrackpath),
					sceneroot(nsceneroot), info_output(ninfo_output),
					error_output(nerror_output), error(false), numobjects(0),
					packload(false), anisotropy(nanisotropy), cull(newcull),
					params_per_object(17), expected_params(17), min_params(14),
					dynamicshadowsenabled(newdynamicshadowsenabled), agressivecombine(doagressivecombining) {}

				bool GetError() const
				{
					return error;
				}

				int GetNumObjects() const
				{
					return numobjects;
				}
				
				///returns false on error
				bool BeginObjectLoad();
				
				///returns a pair of bools: the first bool is true if there was an error, the second bool is true if an object was loaded
				std::pair <bool,bool> ContinueObjectLoad(std::map <std::string, MODEL_JOE03> & model_library,
									std::map <std::string, TEXTURE> & texture_library,
									std::list <TRACK_OBJECT> & objects,
	 								bool vertical_tracking_skyboxes, const std::string & texture_size);
									
				///optimize the drawables by grouping them
				void Optimize();
				bool GetSurfacesBool();
		};
		std::auto_ptr <OBJECTLOADER> objload;
		///returns false on error
		bool BeginObjectLoad(const std::string & trackpath, SCENENODE & sceneroot, int anisotropy, bool dynamicshadowsenabled, bool doagressivecombining, std::ostream & info_output, std::ostream & error_output);
		///returns a pair of bools: the first bool is true if there was an error, the second bool is true if an object was loaded
		std::pair <bool,bool> ContinueObjectLoad();
		bool LoadRoads(const std::string & trackpath, bool reverse, std::ostream & error_output);
		bool LoadLapSequence(const std::string & trackpath, bool reverse, std::ostream & info_output, std::ostream & error_output);
		void ClearRoads() {roads.clear();}
		void Reverse();
		
		///read from the file stream and put it in "output".
		/// return true if the get was successful, else false
		template <typename T>
		bool GetParam(std::ifstream & f, T & output)
		{
			if (!f.good())
				return false;
			
			std::string instr;
			f >> instr;
			if (instr.empty())
				return false;
			
			while (!instr.empty() && instr[0] == '#' && f.good())
			{
				f.ignore(1024, '\n');
				f >> instr;
			}
			
			if (!f.good() && !instr.empty() && instr[0] == '#')
				return false;
			
			std::stringstream sstr(instr);
			sstr >> output;
			return true;
		}
		
		bool loaded;
		bool cull;
	
	public:
		TRACK() : texture_size("large"), vertical_tracking_skyboxes(false), usesurfaces(false), racingline_node(NULL), loaded(false), cull(false) {}
		~TRACK() {Clear();}
		
		void Clear();
		///returns true if successful.  loads the entire track with this one function call.
		bool Load(const std::string & trackpath, const std::string & effects_texturepath, SCENENODE & sceneroot, bool reverse, int anisotropy, const std::string & texsize, std::ostream & info_output, std::ostream & error_output);
		///returns true if successful.  only begins loading the track; the track won't be loaded until more calls to ContinueDeferredLoad().  use Loaded() to see if loading is complete yet.
		bool DeferredLoad(const std::string & trackpath, const std::string & effects_texturepath, SCENENODE & sceneroot, bool reverse, int anisotropy, const std::string & texsize, bool shadows, bool doagressivecombining, std::ostream & info_output, std::ostream & error_output);
		///returns true if successful
		bool ContinueDeferredLoad();
		int DeferredLoadTotalObjects() {assert(objload.get());return objload->GetNumObjects();}
		std::pair <MATHVECTOR <float, 3>, QUATERNION <float> > GetStart(unsigned int index);
		int GetNumStartPositions() {return start_positions.size();}
		bool Loaded() const {return loaded;}
		void GetCollisionObjectsTo(std::list <COLLISION_OBJECT *> & outputlist)
		{
			outputlist.clear();
			for (std::list <TRACK_OBJECT>::iterator i = objects.begin(); i != objects.end(); ++i)
			{
				if (i->GetCollisionObject())
					outputlist.push_back(i->GetCollisionObject());
			}
		}
		bool CollideRoads(const MATHVECTOR <float, 3> & origin, const MATHVECTOR <float, 3> & direction, float seglen, MATHVECTOR <float, 3> & outtri, const BEZIER * & colpatch, MATHVECTOR <float, 3> & normal) const;
		const std::list <ROADSTRIP> & GetRoadList() const {return roads;}
		unsigned int GetSectors() const {return lapsequence.size();}
		const BEZIER * GetLapSequence(unsigned int sector)
		{
			assert (sector < lapsequence.size());
			return lapsequence[sector];
		}
		void SetRacingLineVisibility(bool newvis)
		{
			if(racingline_node)
				racingline_node->SetChildVisibility(newvis);
		}
		void Unload()
		{
			racingline_node = NULL;
			Clear();
		}
		bool IsReversed() const
		{
			return direction == DIRECTION_REVERSE;
		}
		bool UseSurfaceTypes()
		{
			return usesurfaces;
		}
		TRACKSURFACE GetTrackSurface(int index) const
		{
			return tracksurfaces[index];
		}
};

#endif
