#ifndef _GUISTRINGWHEEL2_H
#define _GUISTRINGWHEEL2_H

#include "gui/guilabel.h"
#include "sprite2d.h"

class GUISTRINGWHEEL2 : public GUIWIDGET
{
public:
	GUISTRINGWHEEL2();

	~GUISTRINGWHEEL2();

	virtual void SetAlpha(SCENENODE & scene, float value);

	virtual void SetVisible(SCENENODE & scene, bool value);

	virtual std::string GetDescription() const;

	virtual void SetDescription(const std::string & value);

	virtual bool ProcessInput(
		SCENENODE & scene,
		float cursorx, float cursory,
		bool cursordown, bool cursorjustup);

	virtual void Update(SCENENODE & scene, float dt);

	void SetupDrawable(
		SCENENODE & scene,
		std::tr1::shared_ptr<TEXTURE> bgtex,
		std::map<std::string, GUIOPTION> & optionmap,
		const std::string & setting1,
		const std::string & setting2,
		const FONT & font,
		float scalex, float scaley,
		float centerx, float centery,
		float w, float h, float z,
		std::ostream & error_output);

private:
	GUILABEL m_label_value;
	GUILABEL m_label_left;
	GUILABEL m_label_right;
	std::string m_setting1, m_setting2;
	std::string m_value1, m_value2;
	std::string m_description;
	bool m_focus;

	Signal0 next_value1, prev_value1;
	Signal0 next_value2, prev_value2;
	Slot1<const std::string &> set_value1;
	Slot1<const std::string &> set_value2;
	void SetValue1(const std::string & value);
	void SetValue2(const std::string & value);

	GUISTRINGWHEEL2(const GUISTRINGWHEEL2 & other);
};

#endif
