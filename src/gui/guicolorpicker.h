#ifndef _GUICOLORPICKER_H
#define _GUICOLORPICKER_H

#include "gui/guiwidget.h"
#include "sprite2d.h"

class GUIOPTION;

class GUICOLORPICKER : public GUIWIDGET
{
public:
	GUICOLORPICKER();

	~GUICOLORPICKER();

	virtual void SetAlpha(SCENENODE & scene, float newalpha);

	virtual void SetVisible(SCENENODE & scene, bool newvis);

	virtual void SetDescription(const std::string & newdesc);

	virtual std::string GetDescription() const;

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

	Signal0 signal_action;
	Signal0 signal_moveup;
	Signal0 signal_movedown;
	Signal1<const std::string &> signal_value;

private:
	SPRITE2D sv_plane;
	SPRITE2D sv_bg;
	SPRITE2D h_bar;
	SPRITE2D sv_cursor;
	SPRITE2D h_cursor;
	std::string name;
	std::string description;
	std::string setting;
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

	Slot1<const std::string &> set_value;
	void SetValue(const std::string & value);

	void UpdatePosition();
	bool SetColor(SCENENODE & scene, float x, float y);
	GUICOLORPICKER(const GUICOLORPICKER & other);
};

#endif // _GUICOLORPICKER_H
