#ifndef _WIDGET_SLIDER_H
#define _WIDGET_SLIDER_H

#include "widget_label.h"
#include "sprite2d.h"

class FONT;

class WIDGET_SLIDER : public WIDGET
{
public:
	WIDGET_SLIDER();

	~WIDGET_SLIDER();

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

private:
	WIDGET_LABEL m_label_value;
	WIDGET_LABEL m_label_left;
	WIDGET_LABEL m_label_right;
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

	WIDGET_SLIDER(const WIDGET_SLIDER & other);

	void UpdateText(SCENENODE & scene);
};

#endif
