#ifndef _GUISLIDER_H
#define _GUISLIDER_H

#include "gui/guilabel.h"
#include "sprite2d.h"

class GUIOPTION;
class FONT;

class GUISLIDER : public GUIWIDGET
{
public:
	GUISLIDER();

	~GUISLIDER();

	virtual void SetAlpha(SCENENODE & scene, float value);

	virtual void SetVisible(SCENENODE & scene, bool value);

	virtual bool ProcessInput(
		SCENENODE & scene,
		float cursorx, float cursory,
		bool cursordown, bool cursorjustup);

	virtual std::string GetDescription() const;

	virtual void SetDescription(const std::string & newdesc);

	virtual void Update(SCENENODE & scene, float dt);

	void SetColor(SCENENODE & scene, float r, float g, float b);

	void SetupDrawable(
		SCENENODE & scene,
		std::tr1::shared_ptr<TEXTURE> bgtex,
		std::tr1::shared_ptr<TEXTURE> bartex,
		std::map<std::string, GUIOPTION> & optionmap,
		const std::string & setting,
  		const FONT & font,
  		float scalex, float scaley,
		float centerx, float centery,
		float w, float h, float z,
		float min, float max,
		bool percent, bool fill,
  		std::ostream & error_output);

	Signal0 signal_moveup;
	Signal0 signal_movedown;
	Signal0 signal_next;
	Signal0 signal_prev;

private:
	GUILABEL m_label_value;
	GUILABEL m_label_left;
	GUILABEL m_label_right;
	SPRITE2D m_slider;
	SPRITE2D m_bar;
	std::string m_name;
	std::string m_description;
	std::string m_setting;
	float m_min, m_max, m_current;
	float m_w, m_h;
	bool m_percent;
	bool m_fill;
	bool m_focus;

	Signal1<const std::string &> signal_value;
	Slot1<const std::string &> set_value;
	void SetValue(const std::string & value);

	GUISLIDER(const GUISLIDER & other);

	void UpdateText(SCENENODE & scene);
};

#endif
