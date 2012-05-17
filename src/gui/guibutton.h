#ifndef _GUIBUTTON_H
#define _GUIBUTTON_H

#include "gui/guilabel.h"
#include "gui/guiimage.h"

class GUIBUTTON : public GUILABEL
{
public:
	GUIBUTTON();

	~GUIBUTTON();

	virtual void SetAlpha(SCENENODE & scene, float value);

	virtual std::string GetDescription() const;

	virtual void SetDescription(const std::string & value);

	virtual bool ProcessInput(
		SCENENODE & scene,
		float cursorx, float cursory,
		bool cursordown, bool cursorjustup);

	void SetCancel(bool value);

	void SetAction(const std::string & value);

	void SetEnabled(SCENENODE & scene, bool value);

	Signal0 signal_moveup;
	Signal0 signal_movedown;
	Signal0 signal_action;

private:
	std::string m_description;
	bool m_enabled;
	float m_alpha;
};

#endif
