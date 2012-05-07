#ifndef _WIDGET_H
#define _WIDGET_H

#include "signalslot.h"
#include <map>
#include <string>
#include <ostream>

class GUIOPTION;
class SCENENODE;

class WIDGET
{
public:
	/// true if x, y within widget rectangle
	virtual bool InFocus(float x, float y) const;

	/// scale widget alpha [0, 1]
	virtual void SetAlpha(SCENENODE & scene, float value);

	/// override visibility
	virtual void SetVisible(SCENENODE & scene, bool value);

	/// true if the mouse is within the widget
	virtual bool ProcessInput(
		SCENENODE & scene,
		std::map<std::string, GUIOPTION> & optionmap,
		float cursorx, float cursory,
		bool cursordown, bool cursorjustup);

	/// action associated with the widget
	virtual std::string GetAction() const;

	virtual void SetName(const std::string & newname);

	virtual std::string GetDescription() const;

	virtual void SetDescription(const std::string & newdesc);

	/// write/read associated guioption
	virtual void UpdateOptions(
		SCENENODE & scene,
		bool save_to_options,
		std::map<std::string, GUIOPTION> & optionmap,
		std::ostream & error_output);

	virtual bool GetCancel() const;

	/// update widget state
	virtual void Update(SCENENODE & scene, float dt);

	/// base destructor
	virtual ~WIDGET();

protected:
	float m_xmin, m_ymin, m_xmax, m_ymax;
	float m_r, m_g, m_b, m_a;
	bool m_visible;
	bool m_update;

	WIDGET();
};

inline bool WIDGET::InFocus(float x, float y) const
{
	return x < m_xmax && x > m_xmin && y < m_ymax && y > m_ymin;
}

inline void WIDGET::SetAlpha(SCENENODE & scene, float value)
{
	//GetDrawable(scene).SetColor(m_r, m_g, m_b, m_a * value);
}

inline void WIDGET::SetVisible(SCENENODE & scene, bool value)
{
	//GetDrawable(scene).SetDrawEnable(m_visible & value);
}

inline bool WIDGET::ProcessInput(
	SCENENODE & scene,
	std::map<std::string, GUIOPTION> & optionmap,
	float cursorx, float cursory,
	bool cursordown, bool cursorjustup)
{
	return false;
}

inline std::string WIDGET::GetAction() const
{
	return "";
}

inline void WIDGET::SetName(const std::string & newname)
{
	// optional
}

inline std::string WIDGET::GetDescription() const
{
	return "";
}

inline void WIDGET::SetDescription(const std::string & newdesc)
{
	// optional
}

inline void WIDGET::UpdateOptions(
	SCENENODE & scene,
	bool save_to_options,
	std::map<std::string, GUIOPTION> & optionmap,
	std::ostream & error_output)
{
	// optional
}

inline bool WIDGET::GetCancel() const
{
	return false;
}

inline void WIDGET::Update(SCENENODE & scene, float dt)
{
	// optional
}

inline WIDGET::~WIDGET()
{
	// destructor
}

inline WIDGET::WIDGET() :
	m_xmin(0), m_ymin(0), m_xmax(0), m_ymax(0),
	m_r(1), m_g(1), m_b(1), m_a(0),
	m_visible(true),
	m_update(false)
{
	// constructor
}

#endif // _WIDGET_H
