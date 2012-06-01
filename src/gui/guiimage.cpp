#include "gui/guiimage.h"
#include "contentmanager.h"

GUIIMAGE::GUIIMAGE()
{
	set_image.call.bind<GUIIMAGE, &GUIIMAGE::SetImage>(this);
}

GUIIMAGE::~GUIIMAGE()
{
	// dtor
}

void GUIIMAGE::Update(SCENENODE & scene, float dt)
{
	if (m_update)
	{
		assert(m_content);
		TEXTUREINFO texinfo;
		texinfo.mipmap = false;
		texinfo.repeatu = false;
		texinfo.repeatv = false;
		std::tr1::shared_ptr<TEXTURE> texture;
		if (m_content->load(m_imagepath, m_imagename, texinfo, texture))
			GetDrawable(scene).SetDiffuseMap(texture);
		
		GUIWIDGET::Update(scene, dt);
	}
}

void GUIIMAGE::SetupDrawable(
	SCENENODE & scene,
	ContentManager & content,
	const std::string & imagepath,
	float x, float y, float w, float h, float z)
{
	m_content = &content;
	m_imagepath = imagepath;
	m_varray.SetToBillboard(x - w * 0.5f, y - h * 0.5f, x + w * 0.5f, y + h * 0.5f);

	m_draw = scene.GetDrawlist().twodim.insert(DRAWABLE());
	DRAWABLE & drawref = GetDrawable(scene);
	drawref.SetVertArray(&m_varray);
	drawref.SetCull(false, false);
	drawref.SetDrawOrder(z);
}

void GUIIMAGE::SetImage(const std::string & value)
{
	if (m_imagename != value)
	{
		m_imagename = value;
		m_update = true;
	}
}
