#ifndef _TRACKMAP_H
#define _TRACKMAP_H

#include "mathvector.h"
#include "texture.h"
#include "track.h"
#include "scenegraph.h"

#include <ostream>
#include <list>
#include <string>
#include <iostream>
#include <map>

#include <SDL/SDL.h>

class TRACKMAP
{
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
	TEXTURE_GL track_map;
	TEXTURE_GL cardot0;
	TEXTURE_GL cardot1;
	TEXTURE_GL cardot0_focused;
	TEXTURE_GL cardot1_focused;
	
	SCENENODE * mapnode;
	DRAWABLE * mapdraw;
	VERTEXARRAY mapverts;
	
	class CARDOT
	{
		private:
			DRAWABLE * dotdraw;
			VERTEXARRAY dotverts;
			
		public:
			CARDOT() : dotdraw(NULL) {}
			CARDOT(SCENENODE * topnode, TEXTURE_GL & tex, const MATHVECTOR <float, 2> & corner1, const MATHVECTOR <float, 2> & corner2)
			{
				dotdraw = &topnode->AddDrawable();
				assert(dotdraw);
				dotdraw->SetVertArray(&dotverts);
				dotdraw->SetLit(false);
				dotdraw->Set2D(true);
				dotdraw->SetCull(false, false);
				dotdraw->SetColor(1,1,1,0.7);
				dotdraw->SetDrawOrder(0.1);
				Retexture(tex);
				Reposition(corner1, corner2);
			}
			CARDOT(const CARDOT & other)
			{
				dotdraw = other.dotdraw;
				dotverts = other.dotverts;
				if (dotdraw)
					dotdraw->SetVertArray(&dotverts);
			}
			~CARDOT()
			{
				dotdraw = NULL;
			}
			void Retexture(TEXTURE_GL & newtex)
			{
				assert(dotdraw);
				assert(newtex.Loaded());
				dotdraw->SetDiffuseMap(&newtex);
			}
			void Reposition(const MATHVECTOR <float, 2> & corner1, const MATHVECTOR <float, 2> & corner2)
			{
				dotverts.SetToBillboard(corner1[0], corner1[1], corner2[0], corner2[1]);
			}
			void SetVisible(bool visible)
			{
				assert(dotdraw);
				dotdraw->SetDrawEnable(visible);
				//dotdraw->SetVertArray(&dotverts);
			}
			void DebugPrint(std::ostream & out)
			{
				assert(dotdraw);
				out << dotdraw << ": enable=" << dotdraw->GetDrawEnable() << ", tex=" << dotdraw->GetDiffuseMap() << ", verts=" << dotdraw->GetVertArray() << std::endl;
			}
			DRAWABLE * GetDrawable() {return dotdraw;}
	};
	
	std::list <CARDOT> dotlist;
	
	///w and h are the display device dimensions in pixels
	void CalcPosition(int w, int h);

public:
	TRACKMAP();
	~TRACKMAP();
	///w and h are the display device dimensions in pixels.  returns true if successful.
	bool BuildMap(SCENENODE * parentnode, const std::list <ROADSTRIP> & roads, int w, int h, const std::string & texturepath, const std::string & texsize, std::ostream & error_output);
	void Unload();
	///update the map with provided information for map visibility, as well as a list of car positions and whether or not they're the player car
	void Update(bool mapvisible, const std::list <std::pair<MATHVECTOR <float, 3>, bool> > & carpositions);
	//void DrawMap();
	//void DrawCarDot(int car_index, Vamos_Geometry::Three_Vector position, bool focused);
};

#endif
