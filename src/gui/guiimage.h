#ifndef _GUIIMAGE_H
#define _GUIIMAGE_H

#include "gui/guiwidget.h"
#include "scenenode.h"
#include "vertexarray.h"

class TEXTURE;
class ContentManager;

class GUIIMAGE : public GUIWIDGET
{
public:
	GUIIMAGE();

	~GUIIMAGE();

	virtual void Update(SCENENODE & scene, float dt);

	void SetupDrawable(
		SCENENODE & scene,
		ContentManager & content,
		const std::string & imagepath,
		float x, float y, float w, float h, float z);

	Slot1<const std::string &> set_image;

private:
	ContentManager * m_content;
	std::string m_imagepath, m_imagename;
	keyed_container <DRAWABLE>::handle m_draw;
	VERTEXARRAY m_varray;

	void SetImage(const std::string & value);

	DRAWABLE & GetDrawable(SCENENODE & scene)
	{
		return scene.GetDrawlist().twodim.get(m_draw);
	}

	GUIIMAGE(const GUIIMAGE & other);
};

#endif
