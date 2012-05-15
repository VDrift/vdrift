#ifndef _WIDGET_COLORPICKER_H
#define _WIDGET_COLORPICKER_H

#include "widget.h"
#include "sprite2d.h"

class WIDGET_COLORPICKER : public WIDGET
{
public:
	WIDGET_COLORPICKER();

	~WIDGET_COLORPICKER();

	virtual void SetAlpha(SCENENODE & scene, float newalpha);

	virtual void SetVisible(SCENENODE & scene, bool newvis);

	virtual void SetName(const std::string & newname);

	virtual void SetDescription(const std::string & newdesc);

	virtual std::string GetDescription() const;

	virtual std::string GetAction() const;

	virtual bool ProcessInput(
		SCENENODE & scene,
		float cursorx, float cursory,
		bool cursordown, bool cursorjustup);

	virtual void Update(SCENENODE & scene, float dt);

	void SetupDrawable(
		SCENENODE & scene,
		std::tr1::shared_ptr<TEXTURE> cursortex,
		std::tr1::shared_ptr<TEXTURE> htex,
		std::tr1::shared_ptr<TEXTURE> svtex,
		std::tr1::shared_ptr<TEXTURE> bgtex,
		float x, float y, float w, float h,
		std::map<std::string, GUIOPTION> & optionmap,
		const std::string & setting,
		std::ostream & error_output,
		int draworder);

	void SetAction(const std::string & newaction);

private:
	SPRITE2D sv_plane;
	SPRITE2D sv_bg;
	SPRITE2D h_bar;
	SPRITE2D sv_cursor;
	SPRITE2D h_cursor;
	std::string name;
	std::string description;
	std::string setting;
	std::string action;
	std::string active_action;
	MATHVECTOR <float, 2> sv_min;
	MATHVECTOR <float, 2> sv_max;
	MATHVECTOR <float, 2> h_min;
	MATHVECTOR <float, 2> h_max;
	MATHVECTOR <float, 2> h_pos;
	MATHVECTOR <float, 2> sv_pos;
	MATHVECTOR <float, 3> hsv;
	unsigned rgb;
	bool h_select;
	bool sv_select;
	float size2;
	bool update;

	Signal1<const std::string &> signal_value;
	Slot1<const std::string &> set_value;
	void SetValue(const std::string & value);

	void UpdatePosition();
	bool SetColor(SCENENODE & scene, float x, float y);
	WIDGET_COLORPICKER(const WIDGET_COLORPICKER & other);
};

#endif // _WIDGET_COLORPICKER_H
