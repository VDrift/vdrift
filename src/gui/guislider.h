#ifndef _GUISLIDER_H
#define _GUISLIDER_H

#include "gui/guiwidget.h"
#include "sprite2d.h"

class GUISLIDER : public GUIWIDGET
{
public:
	GUISLIDER();

	~GUISLIDER();

	virtual void Update(SCENENODE & scene, float dt);

	void SetupDrawable(
		SCENENODE & scene,
		std::tr1::shared_ptr<TEXTURE> texture,
		float centerx, float centery,
		float w, float h, float z, bool fill,
  		std::ostream & error_output);

	Slot1<const std::string &> set_value;

private:
	SPRITE2D m_slider;
	float m_value, m_x, m_y, m_w, m_h;
	bool m_fill;

	void SetValue(const std::string & value);
	DRAWABLE & GetDrawable(SCENENODE & scene);
	GUISLIDER(const GUISLIDER & other);
};

#endif
