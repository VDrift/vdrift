#ifndef _GUILABEL_H
#define _GUILABEL_H

#include "gui/guiwidget.h"
#include "scenenode.h"
#include "text_draw.h"

class TEXTURE;
class FONT;

class GUILABEL : public GUIWIDGET
{
public:
	GUILABEL();
	
	virtual ~GUILABEL();

	// align: -1 left, 0 center, +1 right
	void SetupDrawable(
		SCENENODE & scene,
		const FONT & font,
		int align,
		float scalex, float scaley,
		float centerx, float centery,
		float w, float h, float z);

	void SetText(const std::string & text);
	
	const std::string & GetText() const;

	Slot1<const std::string &> set_value;

private:
	keyed_container<DRAWABLE>::handle m_draw;
	TEXT_DRAW m_text_draw;
	std::string m_text;
	const FONT * m_font;
	float m_x, m_y, m_w, m_h;
	float m_scalex, m_scaley;
	int m_align;

	DRAWABLE & GetDrawable(SCENENODE & scene);
	GUILABEL(const GUILABEL & other);
};

#endif
