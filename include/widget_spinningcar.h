#ifndef _WIDGET_SPINNINGCAR_H
#define _WIDGET_SPINNINGCAR_H

#include "widget.h"
#include "model_joe03.h"
#include "texture.h"
#include "scenegraph.h"
#include "mathvector.h"
#include "sprite2d.h"
#include "configfile.h"
#include "coordinatesystems.h"

#include <string>
#include <cassert>
#include <sstream>

class WIDGET_SPINNINGCAR : public WIDGET
{
private:
	std::string data;
	std::string tsize;
	MATHVECTOR <float, 2> center;
	MATHVECTOR <float, 3> carpos;
	int draworder;
	std::ostream * errptr;
	float rotation;
	std::string lastcarname;
	std::string carname;
	std::string lastcarpaint;
	bool wasvisible;
	
	SCENENODE * carnode;
	DRAWABLE * bodydraw;
	DRAWABLE * interiordraw;
	DRAWABLE * glassdraw;
	SCENENODE * bodynode;
	MODEL_JOE03 bodymodel;
	MODEL_JOE03 interiormodel;
	MODEL_JOE03 glassmodel;
	DRAWABLE * wheeldraw[4];
	SCENENODE * wheelnode[4];
	MODEL_JOE03 wheelmodelfront;
	MODEL_JOE03 wheelmodelrear;
	std::map <std::string, TEXTURE_GL> textures;
	
	void Unload()
	{
		//std::cout << "Unloading..." << std::endl;
		
		//unload the car's assets
		for (std::map <std::string, TEXTURE_GL>::iterator i = textures.begin(); i != textures.end(); ++i)
			i->second.Unload();
		textures.clear();
		
		bodymodel.Clear();
		interiormodel.Clear();
		glassmodel.Clear();
		wheelmodelfront.Clear();
		wheelmodelrear.Clear();
		
		if (carnode)
			carnode->Clear();
		bodydraw = NULL;
		interiordraw = NULL;
		glassdraw = NULL;
		bodynode = NULL;
		for (int i = 0; i < 4; i++)
		{
			wheeldraw[i] = NULL;
			wheelnode[i] = NULL;
		}
		//lastcarpaint.clear();
		//lastcarname.clear();
	}
	
	///load assets.  if the parentnode is NULL then the output_drawableptr isn't touched.  if the model or texture is already loaded, they do not get re-loaded
	bool LoadInto ( SCENENODE * parentnode, DRAWABLE * & output_drawableptr, const std::string & joefile,
			MODEL_JOE03 & output_model, const std::string & texfile, const std::string & texfilemisc1,
			int anisotropy, bool transparency, std::ostream & error_output )
	{
		if (!output_model.Loaded())
			if (!output_model.Load(joefile, error_output))
			{
				error_output << "Error loading model: " << joefile << std::endl;
				return false;
			}

		if (!texfile.empty())
		{
			TEXTUREINFO texinfo;
			texinfo.SetName(texfile);
			texinfo.SetMipMap(true);
			texinfo.SetAnisotropy(anisotropy);
			const std::string texture_size(tsize);
			if (!textures[texfile].Loaded())
				if (!textures[texfile].Load(texinfo, error_output, texture_size))
				{
					error_output << "Error loading texture: " << texfile << std::endl;
					textures.erase(texfile);
					return false;
				}
		}
		
		if (!texfilemisc1.empty())
		{
			TEXTUREINFO texinfo;
			texinfo.SetName(texfilemisc1);
			texinfo.SetMipMap(true);
			texinfo.SetAnisotropy(anisotropy);
			const std::string texture_size(tsize);
			if (!textures[texfilemisc1].Loaded())
				if (!textures[texfilemisc1].Load(texinfo, error_output, texture_size))
				{
					textures.erase(texfilemisc1);
				}
		}

		if (parentnode)
		{
			output_drawableptr = &parentnode->AddDrawable();
			//output_drawableptr->SetModel(&output_model);
			output_drawableptr->AddDrawList(output_model.GetListID());
			output_drawableptr->SetDiffuseMap(&textures[texfile]);
			if (textures.find(texfilemisc1) != textures.end())
				output_drawableptr->SetMiscMap1(&textures[texfilemisc1]);
			output_drawableptr->SetObjectCenter(output_model.GetCenter());
			output_drawableptr->SetCameraTransformEnable(false);
			output_drawableptr->SetPartialTransparency(transparency);
		}

		return true;
	}
	
	void Load(const std::string & carname, const std::string & paintstr)
	{
		Unload();
		
		//std::cout << "Loading car " << carname << ", " << paintstr << std::endl;
		
		std::string carpath = data+"/cars/"+carname+"/";
		
		std::stringstream loadlog;
		bodynode = &carnode->AddNode();
		if (!LoadInto(bodynode, bodydraw, carpath+"body.joe", bodymodel,
			 carpath+"/textures/body"+paintstr+".png", carpath+"/textures/body-misc1.png", 0, false, loadlog)) //TODO: anisotropy
			if (bodydraw)
				bodynode->Delete(bodydraw);
		if (!LoadInto(bodynode, interiordraw, carpath+"interior.joe", interiormodel,
			 carpath+"/textures/interior.png", carpath+"/textures/interior-misc1.png", 0, false, loadlog))
			if (interiordraw)
				bodynode->Delete(interiordraw);
		if (!LoadInto(bodynode, glassdraw, carpath+"glass.joe", glassmodel,
			 carpath+"/textures/glass.png", carpath+"/textures/glass-misc1.png", 0, true, loadlog))
			if (glassdraw)
				bodynode->Delete(glassdraw);
		
		QUATERNION <float> fixer;
		fixer.Rotate(-3.141593*0.5, 0, 0, 1);
		bodynode->GetTransform().SetRotation(fixer);
		
		MATHVECTOR <float, 3> bboxcenter(-bodymodel.GetAABB().GetCenter());
		fixer.RotateVector(bboxcenter);
		bodynode->GetTransform().SetTranslation(bboxcenter);
		
		//load the wheels
		DRAWABLE * junk(NULL);
		if (LoadInto(NULL, junk, carpath+"wheel_front.joe", wheelmodelfront,
			carpath+"/textures/wheel_front.png", carpath+"/textures/wheel_front-misc1.png", 0, false, loadlog) &&
				  LoadInto(NULL, junk, carpath+"wheel_rear.joe", wheelmodelrear,
				  carpath+"/textures/wheel_rear.png", carpath+"/textures/wheel_rear-misc1.png", 0, false, loadlog))
		{
			CONFIGFILE carconf;
			if (carconf.Load(carpath+carname+".car" ) )
			{
				int version(1);
				carconf.GetParam("version", version);
				
				for (int i = 0; i < 4; i++)
				{
					QUATERNION <float> myfixer = fixer;
					MODEL_JOE03 * model = &wheelmodelfront;
					std::string texturename = carpath+"/textures/wheel_front.png";
					std::string texturenamemisc1 = carpath+"/textures/wheel_front-misc1.png";
					if (i > 1)
					{
						model = &wheelmodelrear;
						texturename = carpath+"/textures/wheel_rear.png";
						texturenamemisc1 = carpath+"/textures/wheel_rear-misc1.png";
					}
					if (i == 1 || i == 3)
						myfixer = -fixer;
					std::string posstr;
					if (i == 0)
					{
						posstr = "FL";
					}
					else if (i == 1)
					{
						posstr = "FR";
					}
					else if (i == 2)
					{
						posstr = "RL";
					}
					else
					{
						posstr = "RR";
					}
					
					float position[3];
					MATHVECTOR <float, 3> tempvec;
					
					if (carconf.GetParam("wheel-"+posstr+".position", position))
					{
						if (version == 2)
							COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(position[0],position[1],position[2]);
						
						tempvec.Set(position[0],position[1], position[2]);
						
						wheelnode[i] = &carnode->AddNode();
						if (LoadInto(wheelnode[i], wheeldraw[i], "", *model, texturename, texturenamemisc1, 0, false, loadlog))
						{
							wheelnode[i]->GetTransform().SetRotation(myfixer);
							wheelnode[i]->GetTransform().SetTranslation(tempvec+bboxcenter);
						}
					}
				}
			}
		}
		
		//carnode->GetTransform().SetTranslation(MATHVECTOR <float, 3> (0,0,-20));
		//carnode->GetTransform().SetTranslation(MATHVECTOR <float, 3> (-center[0]*4.0,center[1]*4.0,-15)); //TODO: better positioning?
		MATHVECTOR <float, 3> cartrans = carpos;
		cartrans[0] += center[0];
		cartrans[1] += center[1];
		carnode->GetTransform().SetTranslation(cartrans);
		
		//set initial rotation
		Update(0);
		
		if (carnode)
		{
			carnode->SetChildVisibility(wasvisible);
		}
		
		//std::cerr << "Loading log: " << loadlog.str();
	}
	
public:
	WIDGET_SPINNINGCAR() : errptr(NULL),rotation(0),wasvisible(false),carnode(NULL),bodydraw(NULL),
		interiordraw(NULL),glassdraw(NULL),bodynode(NULL)
	{
		for (int i = 0; i < 4; i++)
		{
			wheeldraw[i] = NULL;
			wheelnode[i] = NULL;
		}
	}
	~WIDGET_SPINNINGCAR() {Unload();}
	virtual WIDGET * clone() const {return new WIDGET_SPINNINGCAR(*this);};
	
	void SetupDrawable(SCENENODE * scene, const std::string & texturesize, const std::string & datapath,
      			   float x, float y, const MATHVECTOR <float, 3> & newcarpos, std::ostream & error_output, int order=0)
	{
		data = datapath;
		tsize = texturesize;
		
		errptr = &error_output;
		
		center.Set(x,y);
		
		carpos = newcarpos;
		
		draworder = order;
		
		assert(!carnode);
		carnode = &scene->AddNode();
	}
	
	virtual void SetAlpha(float newalpha)
	{
		//set alpha
		if (bodydraw)
			bodydraw->SetColor(1,1,1,newalpha);
		if (interiordraw)
			interiordraw->SetColor(1,1,1,newalpha);
		if (glassdraw)
			glassdraw->SetColor(1,1,1,newalpha);
		for (int i = 0; i < 4; i++)
		{
			if (wheeldraw[i])
				wheeldraw[i]->SetColor(1,1,1,newalpha);
		}
	}
	
	virtual void SetVisible(bool newvis)
	{
		wasvisible = newvis;
		
		//std::cout << "newvis: " << newvis << std::endl;
		
		if (carnode)
		{
			carnode->SetChildVisibility(newvis);
		}
		
		if (newvis && !bodydraw)
		{
			if (!lastcarpaint.empty() && !carname.empty())
			{
				Load(carname, lastcarpaint);
			}
			else
			{
				//std::cout << "Not loading car on visibility change since no carname or carpaint are present:" << std::endl;
				//std::cout << carname << std::endl;
				//std::cout << lastcarpaint << std::endl;
			}
		}
		
		if (!newvis)
		{
			//std::cout << "Unloading spinning car due to visibility" << std::endl;
			Unload();
		}
	}
	
	virtual void HookMessage(const std::string & message)
	{
		assert(errptr);
		assert(carnode);
		
		//std::cout << "Message: " << message << std::endl;
		
		//if the message is all numbers and two digits, assume it's the car paint
		std::string paintstr;
		if (message.find_first_not_of("0123456789") == std::string::npos && message.length() == 2)
		{
			paintstr = message;
		}
		else
		{
			lastcarname = carname;
			carname = message;
			//the paint should come in as the second message; that's when we do the actual load
			return;
		}
		
		assert(!carname.empty());
		assert(!paintstr.empty());
		
		//if everything is exactly the same don't re-load.  if we're not visible, don't load.
		if (carname == lastcarname && paintstr == lastcarpaint)
		{
			//std::cout << "Not loading car because it's already loaded:" << std::endl;
			//std::cout << carname << "/" << lastcarname << ", " << paintstr << "/" << lastcarpaint << std::endl;
			return;
		}
		else
		{
			//std::cout << "Car/paint change:" << std::endl;
			//std::cout << carname << "/" << lastcarname << ", " << paintstr << "/" << lastcarpaint << std::endl;
		}
		
		lastcarpaint = paintstr;
		
		if (!wasvisible)
		{
			//std::cout << "Not visible, returning" << std::endl;
			return;
		}
		
		Load(carname, paintstr);
	}
	
	virtual void Update(float dt)
	{
		rotation += dt;
		QUATERNION <float> q;
		q.Rotate(3.141593*1.5,1,0,0);
		q.Rotate(rotation,0,1,0);
		q.Rotate(3.141593*0.1,1,0,0);
		carnode->GetTransform().SetRotation(q);
	}
};

#endif
