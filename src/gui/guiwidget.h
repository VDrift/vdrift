#ifndef _GUIWIDGET_H
#define _GUIWIDGET_H

#include "signalslot.h"
#include <string>

class SCENENODE;
class DRAWABLE;

class GUIWIDGET
{
public:
	/// base destructor
	virtual ~GUIWIDGET();

	/// update widget state
	virtual void Update(SCENENODE & scene, float dt);

	/// scale widget alpha [0, 1]
	void SetAlpha(SCENENODE & scene, float value);

	/// override visibility
	void SetVisible(SCENENODE & scene, bool value);

	/// properties
	void SetColor(float r, float g, float b);
	void SetAlpha(float value);
	void SetHue(float value);
	void SetSat(float value);
	void SetVal(float value);

	/// add support for typed signals
	Slot1<const std::string &> set_color;
	Slot1<const std::string &> set_alpha;
	Slot1<const std::string &> set_hue;
	Slot1<const std::string &> set_sat;
	Slot1<const std::string &> set_val;

protected:
	//static const float m_ease_factor;
	//float m_r1, m_g1, m_b1, m_a1;
	float m_r, m_g, m_b, m_a;
	bool m_visible;
	bool m_update;

	GUIWIDGET();
	virtual DRAWABLE & GetDrawable(SCENENODE & scene) = 0;
	void SetColor(const std::string & value);
	void SetAlpha(const std::string & value);
	void SetHue(const std::string & value);
	void SetSat(const std::string & value);
	void SetVal(const std::string & value);

};

#endif // _GUIWIDGET_H
