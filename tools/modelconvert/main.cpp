#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <list>
#include <algorithm>

#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "model_joe03.h"
#include "model_obj.h"

using namespace std;

int main(int argc, char ** argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH);
  glutInitWindowSize(640, 480);
  glutInitWindowPosition(0, 0);
  int window = glutCreateWindow("Converter");
  
  list <string> args(argv, argv + argc);
  
  map <string, string> argmap;

  //generate an argument map
  for (list <string>::iterator i = args.begin(); i != args.end(); ++i)
  {
      if ((*i)[0] == '-')
      {
          argmap[*i] = "";
      }

      list <string>::iterator n = i;
      n++;
      if (n != args.end())
      {
          if ((*n)[0] != '-')
              argmap[*i] = *n;
      }
  }
  
  GLenum glew_err = glewInit();
  if ( glew_err != GLEW_OK )
  {
      cerr << "GLEW failed to initialize: " << glewGetErrorString ( glew_err ) << endl;
      assert(glew_err == GLEW_OK);
  }

  map <string, MODEL *> typemap;
  typemap["joe"] = new MODEL_JOE03;
  typemap["obj"] = new MODEL_OBJ;
  
  string infile = argmap["-in"];
  string outfile = argmap["-out"];
  
  if (infile.empty() || outfile.empty())
  {
	  cout << "Usage: -in <INPUTFILE> -out <OUTPUTFILE>" << endl << endl;
	  cout << "Input file formats supported: ova";
	  for (map <string, MODEL *>::iterator i = typemap.begin(); i != typemap.end(); ++i)
	  {
		  cout << ", " << i->first;
	  }
	  cout << endl;
	  cout << "Output file formats supported: ova";
	  for (map <string, MODEL *>::iterator i = typemap.begin(); i != typemap.end(); ++i)
	  {
		  if (i->second->CanSave())
			  cout << ", " << i->first;
	  }
	  cout << endl << endl;
	  return 0;
  }
  
  string inext = infile.substr(max(infile.size()*0,infile.size()-3));
  string outext = outfile.substr(max(outfile.size()*0,outfile.size()-3));

  MODEL * inmodel(NULL);  

  if (inext == "ova")
  {
	  inmodel = new MODEL_OBJ();
	  if (!inmodel->ReadFromFile(infile, cerr, false))
	  {
		  cerr << "Error loading " << infile << endl;
		  return 0;
	  }
  }
  else
  {
	  if (typemap.find(inext) == typemap.end())
	  {
		  cerr << "Don't know how to read " << inext << " files" << endl;
		  return 0;
	  }
	  
	  inmodel = typemap[inext];
	  if (!inmodel->Load(infile, cerr))
	  {
		  cerr << "Error loading " << infile << endl;
		  return 0;
	  }
  }
  
  if (outext == "ova")
  {
    if (!inmodel->WriteToFile(outfile))
    {
      cerr << "Error writing to ova file" << endl;
      return 0;
    }
  }
  else if (typemap.find(outext) != typemap.end())
  {
	  MODEL * outmodel = typemap[outext];
	  if (!outmodel->CanSave())
	  {
		  cerr << "File format " << outext << " doesn't support saving" << endl;
		  return 0;
	  }
	  if (inmodel != outmodel)
		  outmodel->SetVertexArray(inmodel->GetVertexArray());
	  if (!outmodel->Save(outfile, cerr))
	  {
		  cerr << "Error converting to " << outfile << endl;
		  return 0;
	  }
  }
  else
  {
    cerr << "Don't know how to save to " << outext << " format" << endl;
    return 0;
  }
  
  cout << "Converting from " << inext << " to " << outext << endl;

  return 0;
}
