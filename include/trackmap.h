#ifndef _TRACKMAP_H
#define _TRACKMAP_H

#include "mathvector.h"
#include "track.h"
#include "scenenode.h"

#include <ostream>
#include <list>
#include <string>

class SDL_Surface;

class TRACKMAP
{
public:
	TRACKMAP();
	
	~TRACKMAP();
	
	///w and h are the display device dimensions in pixels.  returns true if successful.
	bool BuildMap(
		const std::list <ROADSTRIP> & roads,
		int w,
		int h,
		const std::string & trackname,
		const std::string & texturepath,
		const std::string & texsize,
		ContentManager & content,
		std::ostream & error_output);
	
	void Unload();
	
	///update the map with provided information for map visibility, as well as a list of car positions and whether or not they're the player car
	void Update(bool mapvisible, const std::list <std::pair<MATHVECTOR <float, 3>, bool> > & carpositions);
	
	SCENENODE & GetNode()
	{
		return mapnode;
	}

private:
	//track map size in real world 
	MATHVECTOR <float, 2> mapsize;
	float map_w_min, map_w_max;
	float map_h_min, map_h_max;
	float scale;
	const int MAP_WIDTH;
	const int MAP_HEIGHT;

	//screen size
	MATHVECTOR <float, 2> screen;

	//position of the trackmap on screen
	MATHVECTOR <float, 2> position;

	//size of the trackmap on screen
	MATHVECTOR <float, 2> size;

	//size of the car dot on screen
	MATHVECTOR <float, 2> dot_size;

	SDL_Surface* surface;

	TexturePtr cardot0;
	TexturePtr cardot1;
	TexturePtr cardot0_focused;
	TexturePtr cardot1_focused;
	
	SCENENODE mapnode;
	keyed_container <DRAWABLE>::handle mapdraw;
	VERTEXARRAY mapverts;
	
	class CARDOT
	{
		public:
			void Init(
				SCENENODE & topnode, 
				TexturePtr tex, 
				const MATHVECTOR <float, 2> & corner1, 
				const MATHVECTOR <float, 2> & corner2);
			
			void Retexture(SCENENODE & topnode, TexturePtr newtex);
			
			void Reposition(const MATHVECTOR <float, 2> & corner1, const MATHVECTOR <float, 2> & corner2);
			
			void SetVisible(SCENENODE & topnode, bool visible);
			
			void DebugPrint(SCENENODE & topnode, std::ostream & out) const;
			
			keyed_container <DRAWABLE>::handle & GetDrawableHandle()
			{
				return dotdraw;
			}
		
		private:
			keyed_container <DRAWABLE>::handle dotdraw;
			VERTEXARRAY dotverts;

			DRAWABLE & GetDrawable(SCENENODE & topnode)
			{
				return topnode.GetDrawlist().twodim.get(dotdraw);
			}
			
			const DRAWABLE & GetDrawable(SCENENODE & topnode) const
			{
				return topnode.GetDrawlist().twodim.get(dotdraw);
			}
	};
	
	std::list <CARDOT> dotlist;
	
	///w and h are the display device dimensions in pixels
	void CalcPosition(int w, int h);
};

#endif
