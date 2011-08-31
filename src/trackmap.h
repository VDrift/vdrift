#ifndef _TRACKMAP_H
#define _TRACKMAP_H

#include "mathvector.h"
#include "scenenode.h"
#include "roadstrip.h"

#include <list>
#include <string>
#include <iostream>

class ContentManager;
class TEXTURE;
struct SDL_Surface;

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
		ContentManager & content,
		std::ostream & error_output);

	void Unload();

	///update the map with provided information for map visibility, as well as a list of car positions and whether or not they're the player car
	void Update(bool mapvisible, const std::list <std::pair<MATHVECTOR <float, 3>, bool> > & carpositions);

	SCENENODE & GetNode() {return mapnode;}

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

	SCENENODE mapnode;
	keyed_container <DRAWABLE>::handle mapdraw;
	VERTEXARRAY mapverts;

	// car dot textures
	std::tr1::shared_ptr<TEXTURE> cardot0;
	std::tr1::shared_ptr<TEXTURE> cardot1;
	std::tr1::shared_ptr<TEXTURE> cardot0_focused;
	std::tr1::shared_ptr<TEXTURE> cardot1_focused;

	class CARDOT
	{
		public:
			void Init(
				SCENENODE & topnode,
				std::tr1::shared_ptr<TEXTURE> tex,
				const MATHVECTOR <float, 2> & corner1,
				const MATHVECTOR <float, 2> & corner2)
			{
				dotdraw = topnode.GetDrawlist().twodim.insert(DRAWABLE());
				DRAWABLE & drawref = GetDrawable(topnode);
				drawref.SetVertArray(&dotverts);
				drawref.SetCull(false, false);
				drawref.SetColor(1,1,1,0.7);
				drawref.SetDrawOrder(0.1);
				Retexture(topnode, tex);
				Reposition(corner1, corner2);
			}
			void Retexture(SCENENODE & topnode, std::tr1::shared_ptr<TEXTURE> newtex)
			{
				assert(newtex.get());
				GetDrawable(topnode).SetDiffuseMap(newtex);
			}
			void Reposition(const MATHVECTOR <float, 2> & corner1, const MATHVECTOR <float, 2> & corner2)
			{
				dotverts.SetToBillboard(corner1[0], corner1[1], corner2[0], corner2[1]);
			}
			void SetVisible(SCENENODE & topnode, bool visible)
			{
				GetDrawable(topnode).SetDrawEnable(visible);
			}
			void DebugPrint(SCENENODE & topnode, std::ostream & out) const
			{
				const DRAWABLE & drawref = GetDrawable(topnode);
				out << &drawref << ": enable=" << drawref.GetDrawEnable() << ", tex=" << drawref.GetDiffuseMap() << ", verts=" << drawref.GetVertArray() << std::endl;
			}
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
};

#endif
