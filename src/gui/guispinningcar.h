#ifndef _WIDGET_SPINNINGCAR_H
#define _WIDGET_SPINNINGCAR_H

#include "widget.h"
#include "scenenode.h"
#include "mathvector.h"

class ContentManager;
class PATHMANAGER;
class CAR;

class WIDGET_SPINNINGCAR : public WIDGET
{
public:
	WIDGET_SPINNINGCAR();

	~WIDGET_SPINNINGCAR();

	virtual void SetAlpha(SCENENODE & scene, float newalpha);

	virtual void SetVisible(SCENENODE & scene, bool newvis);

	virtual void Update(SCENENODE & scene, float dt);

	void SetupDrawable(
		SCENENODE & scene,
		ContentManager & content,
		const PATHMANAGER & pathmanager,
		std::map<std::string, GUIOPTION> & optionmap,
		const float x, const float y,
		const MATHVECTOR <float, 3> & newcarpos,
		const std::string & option,
		std::ostream & error_output,
		int order = 0);

private:
	std::string datarw;
	std::string dataro;
	std::string dataparts;
	std::string tsize;
	std::string carname;
	std::string carpaint;
	unsigned carcolor;
	MATHVECTOR <float, 2> center;
	MATHVECTOR <float, 3> carpos;
	const PATHMANAGER * pathptr;
	ContentManager * contentptr;
	std::ostream * errptr;
	float rotation;
	int draworder;
	bool wasvisible;
	bool updatecolor;
	bool updatecar;

	keyed_container <SCENENODE>::handle carnode;
	CAR * car;

	// widget slots
	Slot1<const std::string &> set_car;
	Slot1<const std::string &> set_paint;
	Slot1<const std::string &> set_color;
	void SetCar(const std::string & name);
	void SetPaint(const std::string & paint);
	void SetColor(const std::string & color);

	WIDGET_SPINNINGCAR(const WIDGET_SPINNINGCAR & other);

	SCENENODE & GetCarNode(SCENENODE & parent);

	void SetColor(SCENENODE & scene, float r, float g, float b);

	void Unload(SCENENODE & parent);

	void Load(SCENENODE & parent);

	void Rotate(SCENENODE & scene, float delta);

	bool Valid() const {return carnode.valid();}
};

#endif // _WIDGET_SPINNINGCAR_H
