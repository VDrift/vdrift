#ifndef _WIDGET_BUTTON_H
#define _WIDGET_BUTTON_H

#include "widget_label.h"
#include "widget_image.h"

class WIDGET_BUTTON : public WIDGET_LABEL
{
public:
	WIDGET_BUTTON();

	~WIDGET_BUTTON();

	virtual void SetAlpha(SCENENODE & scene, float value);

	virtual std::string GetAction() const;

	virtual std::string GetDescription() const;

	virtual void SetDescription(const std::string & value);

	virtual bool GetCancel() const;

	virtual bool ProcessInput(
		SCENENODE & scene,
		float cursorx, float cursory,
		bool cursordown, bool cursorjustup);

	void SetCancel(bool value);

	void SetAction(const std::string & value);

	void SetEnabled(SCENENODE & scene, bool value);

private:
	std::string m_action;
	std::string m_active_action;
	std::string m_description;
	bool m_cancel;
	bool m_enabled;
	float m_alpha;
};

#endif
