#ifndef _WIDGET_SLIDER_H
#define _WIDGET_SLIDER_H

#include "widget.h"
#include "signal.h"
#include "sprite2d.h"
#include "text_draw.h"
#include "mathvector.h"

class FONT;

class WIDGET_SLIDER : public WIDGET
{
public:
	WIDGET_SLIDER();

	~WIDGET_SLIDER();

	virtual void SetAlpha(SCENENODE & scene, float newalpha);

	virtual void SetVisible(SCENENODE & scene, bool newvis);

	virtual bool ProcessInput(
		SCENENODE & scene,
		std::map<std::string, GUIOPTION> & optionmap,
		float cursorx, float cursory,
		bool cursordown, bool cursorjustup);

	virtual void SetName(const std::string & newname);

	virtual std::string GetDescription() const;

	virtual void SetDescription(const std::string & newdesc);

	virtual void Update(SCENENODE & scene, float dt);

	void SetColor(SCENENODE & scene, float r, float g, float b);

	void SetSetting(const std::string & newsetting);

	void SetupDrawable(
		SCENENODE & scene,
		std::tr1::shared_ptr<TEXTURE> wedgetex,
		std::tr1::shared_ptr<TEXTURE> cursortex,
		const float x,
		const float y,
		const float nw,
		const float nh,
		const float newmin,
		const float newmax,
		const bool ispercentage,
		std::map<std::string, GUIOPTION> & optionmap,
		const std::string & newsetting,
  		const FONT & font,
  		const float fontscalex,
  		const float fontscaley,
  		std::ostream & error_output,
  		int draworder = 1);

private:
	TEXT_DRAWABLE text;
	SPRITE2D wedge;
	SPRITE2D cursor;
	std::string name;
	std::string description;
	std::string setting;
	MATHVECTOR <float, 2> corner1;
	MATHVECTOR <float, 2> corner2;
	float min, max, current;
	bool percentage;
	float w, h, texty;
	bool update;

	Slot1<const std::string &> set_value;
	void SetValue(const std::string & value);

	WIDGET_SLIDER(const WIDGET_SLIDER & other);

	void UpdateText(SCENENODE & scene);
};

#endif
