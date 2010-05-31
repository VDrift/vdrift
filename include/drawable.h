#ifndef _DRAWABLE_H
#define _DRAWABLE_H

#include <vector>
#ifdef _MSC_VER
#include <memory>
#else
#include <tr1/memory>
#endif

#include "matrix4.h"
#include "mathvector.h"

class TEXTURE;
typedef std::tr1::shared_ptr<TEXTURE> TEXTUREPTR;
class VERTEXARRAY;

class DRAWABLE
{
public:
	DRAWABLE() {Init();}
	virtual ~DRAWABLE() {};
	
	bool operator < (const DRAWABLE & other) const {return draw_order < other.draw_order;}
	
	bool IsDrawList() const {return !list_ids.empty();}
	
	const std::vector <int> & GetDrawLists() const {return list_ids;}
	void AddDrawList(int value) {list_ids.push_back(value);}
	
	const TEXTURE * GetDiffuseMap() const {return diffuse_map.get();}
	void SetDiffuseMap(TEXTUREPTR value) {diffuse_map = value;}
	
	const TEXTURE * GetMiscMap1() const {return misc_map1.get();}
	void SetMiscMap1(TEXTUREPTR value) {misc_map1 = value;}
	
	const VERTEXARRAY * GetVertArray() const {return vert_array;}
	void SetVertArray(const VERTEXARRAY* value) {vert_array = value;}
	
	const std::vector < MATHVECTOR <float, 3> > & GetLine() const {return lineverts;}
	void AddLinePoint(const MATHVECTOR <float, 3> & value) {lineverts.push_back(value);}
	void ClearLine() {lineverts.clear();}
	
	float GetLinesize() const {return linesize;}
	void SetLinesize(float value) {linesize = value;}
	
	const MATRIX4 <float> & GetTransform() {return transform;}
	void SetTransform(const MATRIX4 <float> & value) {transform = value;}
	
	/// used for bounding sphere frustum culling
	const MATHVECTOR <float, 3> & GetObjectCenter() const {return objcenter;}
	void SetObjectCenter(const MATHVECTOR <float, 3> & value) {objcenter = value;}
	
	/// for bounding sphere frustum culling
	float GetRadius() const {return radius;}
	void SetRadius(float value) {radius = value;}
	
	void GetColor(float &nr, float &ng, float &nb, float &na) const {nr = r; ng = g; nb = b; na = a;}
	void SetColor(float nr, float ng, float nb, float na) {r = nr; g = ng; b = nb; a = na;}
	void SetColor(float nr, float ng, float nb) {r = nr; g = ng; b = nb;}
	void SetAlpha(float na) {a = na;}
	
	float GetDrawOrder() const {return draw_order;}
	void SetDrawOrder(float value) {draw_order = value;}
	void SetDrawOrderFont() {SetDrawOrder(5);}
	void SetDrawOrderCursor() {SetDrawOrder(10);}
	void SetDrawOrderGUIBackground() {SetDrawOrder(2);}
	void SetDrawOrderGUIForeground() {SetDrawOrder(4);}
	
	bool GetDecal() const {return decal;}
	void SetDecal(bool newdecal) {decal = newdecal;}
	
	bool GetDrawEnable() const {return drawenabled;}
	void SetDrawEnable(bool value) {drawenabled = value;}
	
	bool GetCull() const {return cull;}
	bool GetCullFront() const {return cull_front;}
	void SetCull(bool newcull, bool newcullfront) {cull = newcull; cull_front = newcullfront;}
	
	bool GetSkybox() const {return skybox;}
	void SetSkybox(bool value) {skybox = value;}
	
	/// used for so called "vertical tracking skyboxes"
	bool GetVerticalTrack() const {return vertical_track;}
	void SetVerticalTrack(bool value) {vertical_track = value;}

	/// true for normal objects; if false camera transform and orientation aren't applied when the object is rendered
	bool GetCameraTransformEnable() const {return cameratransform;}
	void SetCameraTransformEnable(bool value) {cameratransform = value;}
	
private:
	TEXTUREPTR diffuse_map;
	TEXTUREPTR misc_map1;
	std::vector <int> list_ids;
	const VERTEXARRAY * vert_array;
	std::vector <MATHVECTOR <float, 3> > lineverts;
	float linesize;
	MATRIX4 <float> transform;
	MATHVECTOR <float, 3> objcenter;
	float radius;
	float r, g, b, a;
	float draw_order;
	bool decal;
	bool drawenabled;
	bool cull;
	bool cull_front;
	bool skybox;
	bool vertical_track;
	bool cameratransform;
	
	void Init()
	{
		vert_array = NULL;
		linesize = 1.0;
		objcenter.Set(0.0);
		radius = 0.0;
		r = g = b = 1.0;
		a = 1.0;
		draw_order = 0;
		decal = false;
		drawenabled = true;
		cull = false;
		cull_front = false;
		skybox = false;
		vertical_track = false;
		cameratransform = true;
	}
};

#endif // _DRAWABLE_H
