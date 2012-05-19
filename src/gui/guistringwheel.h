#ifndef _GUISTRINGWHEEL_H
#define _GUISTRINGWHEEL_H

#include "gui/guilabel.h"
#include "gui/guicontrol.h"
#include "sprite2d.h"

class GUIOPTION;

class GUISTRINGWHEEL : public GUICONTROL
{
public:
	GUISTRINGWHEEL();

	~GUISTRINGWHEEL();

	virtual void SetAlpha(SCENENODE & scene, float value);

	virtual void SetVisible(SCENENODE & scene, bool value);

	virtual void Update(SCENENODE & scene, float dt);

	virtual bool ProcessInput(
		SCENENODE & scene,
		float cursorx, float cursory,
		bool cursordown, bool cursorjustup);

	void SetupDrawable(
		SCENENODE & scene,
		std::tr1::shared_ptr<TEXTURE> bgtex,
		std::map<std::string, GUIOPTION> & optionmap,
		const std::string & setting,
		const FONT & font,
		float scalex, float scaley,
		float centerx, float centery,
		float w, float h, float z,
		std::ostream & error_output);

protected:
	GUILABEL m_label_value;
	GUILABEL m_label_left;
	GUILABEL m_label_right;
	std::string m_setting;
	std::string m_value;

	Slot1<const std::string &> set_value;
	void SetValue(const std::string & value);

	GUISTRINGWHEEL(const GUISTRINGWHEEL & other);
};

#endif // _GUISTRINGWHEEL_H
