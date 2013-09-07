/************************************************************************/
/*                                                                      */
/* This file is part of VDrift.                                         */
/*                                                                      */
/* VDrift is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* VDrift is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with VDrift.  If not, see <http://www.gnu.org/licenses/>.      */
/*                                                                      */
/************************************************************************/

#ifndef _RENDER_INPUT_SCENE_H
#define _RENDER_INPUT_SCENE_H

#include "render_input.h"
#include "mathvector.h"
#include "quaternion.h"
#include "matrix4.h"
#include "frustum.h"
#include "reseatable_reference.h"
#include <vector>

class SceneNode;
class Drawable;
class VertexArray;
class TextureInterface;
class Shader;

class RenderInputScene : public RenderInput
{
public:
	RenderInputScene();

	~RenderInputScene();

	void SetDrawLists(
		const std::vector <Drawable*> & dl_dynamic,
		const std::vector <Drawable*> & dl_static);

	void DisableOrtho();

	void SetOrtho(
		const Vec3 & neworthomin,
		const Vec3 & neworthomax);

	Frustum SetCameraInfo(
		const Vec3 & newpos,
		const Quat & newrot,
		float newfov, float newlodfar,
		float neww, float newh,
		bool restore_matrices = true);

	const Mat4 & GetProjMatrix() const;

	const Mat4 & GetViewMatrix() const;

	void SetSunDirection(const Vec3 & newsun);

	void SetFlags(bool newshaders);

	void SetDefaultShader(Shader & newdefault);

	void SetClear(bool newclearcolor, bool newcleardepth);

	virtual void Render(GraphicsState & glstate, std::ostream & error_output);

	void SetFSAA(unsigned int value);

	void SetContrast(float value);

	void SetDepthMode(int mode);

	void SetWriteDepth(bool write);

	void SetWriteColor(bool write);

	void SetWriteAlpha(bool write);

	std::pair <bool, bool> GetClear() const;

	void SetCarPaintHack(bool hack);

	void SetBlendMode(BlendMode::BLENDMODE mode);

private:
	reseatable_reference <const std::vector <Drawable*> > dynamic_drawlist_ptr;
	reseatable_reference <const std::vector <Drawable*> > static_drawlist_ptr;
	bool last_transform_valid;
	Mat4 last_transform;
	Quat cam_rotation; //used for the skybox effect
	Vec3 cam_position;
	Vec3 lightposition;
	Vec3 orthomin;
	Vec3 orthomax;
	float w, h;
	float camfov;
	Mat4 projMatrix;
	Mat4 viewMatrix;
	Frustum frustum; //used for frustum culling
	float lod_far; //used for distance culling
	bool shaders;
	bool clearcolor, cleardepth;
	reseatable_reference <Shader> shader;
	bool orthomode;
	unsigned int fsaa;
	float contrast;
	int depth_mode;
	bool writecolor;
	bool writealpha;
	bool writedepth;
	bool carpainthack;
	bool vlighting;
	BlendMode::BLENDMODE blendmode;

	void EnableCarPaint(GraphicsState & glstate);

	void DisableCarPaint(GraphicsState & glstate);

	void SetBlendMode(GraphicsState & glstate);

	void DrawList(GraphicsState & glstate, const std::vector <Drawable*> & drawlist, bool preculled);

	void DrawVertexArray(const VertexArray & va, float linesize) const;

	/// returns true if the object was culled and should not be drawn
	bool FrustumCull(const Drawable & d);

	void SetFlags(const Drawable & d, GraphicsState & glstate);

	void SetTextures(const Drawable & d, GraphicsState & glstate);

	void SetTransform(const Drawable & d, GraphicsState & glstate);
};

#endif // _RENDER_INPUT_SCENE_H
