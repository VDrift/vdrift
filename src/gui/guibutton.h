#ifndef _GUIBUTTON_H
#define _GUIBUTTON_H

#include "gui/guicontrol.h"
#include "gui/guilabel.h"

class GUIBUTTON : public GUICONTROL
{
public:
	GUIBUTTON();

	~GUIBUTTON();

	virtual void SetAlpha(SCENENODE & scene, float value);

	virtual void SetVisible(SCENENODE & scene, bool value);

	virtual bool ProcessInput(
		SCENENODE & scene,
		float cursorx, float cursory,
		bool cursordown, bool cursorjustup);

	void SetEnabled(SCENENODE & scene, bool value);

	void SetText(const std::string & value);

	// align: -1 left, 0 center, +1 right
	void SetupDrawable(
		SCENENODE & scene,
		const FONT & font,
		int align,
		float scalex, float scaley,
		float centerx, float centery,
		float w, float h, float z,
		float r, float g, float b);

private:
	GUILABEL m_label;
	bool m_enabled;
	float m_alpha;
};

#endif
