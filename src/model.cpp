#include "model.h"
#include "utils.h"
#include "vertexattribs.h"
using namespace VERTEX_ATTRIBS;

#include <fstream>
#include <sstream>

#define ERROR_CHECK OPENGL_UTILITY::CheckForOpenGLErrors(std::string(__PRETTY_FUNCTION__)+":"+__FILE__+":"+UTILS::tostr(__LINE__), error_output)

MODEL::MODEL() : 
	generatedlistid(false),
	generatedmetrics(false),
	generatedvao(false),
	vao(0),
	elementVbo(0),
	elementCount(0),
	radius(0),
	radiusxz(0)
{
	// ctor
}

MODEL::MODEL(const std::string & filepath, std::ostream & error_output) :
	generatedlistid(false),
	generatedmetrics(false),
	generatedvao(false),
	vao(0),
	elementVbo(0),
	elementCount(0),
	radius(0),
	radiusxz(0) 
{
	if (filepath.size() > 4 && filepath.substr(filepath.size()-4) == ".ova")
	{
		ReadFromFile(filepath, error_output, false);
	}
	else
	{
		Load(filepath, error_output, false);
	}
}

MODEL::~MODEL()
{
	Clear();
}

void MODEL::GenerateListID(std::ostream & error_output)
{
	if (HaveListID()) return;
	
	ClearListID();
	
	listid = glGenLists(1);
	glNewList (listid, GL_COMPILE);
	
	glBegin(GL_TRIANGLES);
	
	int true_faces = mesh.GetNumFaces() / 3;
	
	//cout << "generating list from " << true_faces << " faces" << endl;
	
	// iterate through all of the faces (polygons)
	for(int j = 0; j < true_faces; j++)
	{
		// iterate though each vertex in the face
		for(int whichVertex = 0; whichVertex < 3; whichVertex++)
		{
			// get the 3D location for this vertex
			///vert array bounds are not checked but it is assumed to be of size 3
			std::vector <float> vert = mesh.GetVertex(j, whichVertex);
			
			// get the 3D normal for this vertex
			///norm array bounds are not checked but it is assumed to be of size 3
			std::vector <float> norm = mesh.GetNormal(j, whichVertex);
			
			assert (mesh.GetTexCoordSets() > 0);
			
			if (mesh.GetTexCoordSets() > 0)
			{
				// get the 2D texture coordinates for this vertex
				///tex array bounds are not checked but it is assumed to be of size 2
				std::vector <float> tex = mesh.GetTextureCoordinate(j, whichVertex, 0);
				
				glMultiTexCoord2f(GL_TEXTURE0, tex[0], tex[1]);
			}

			glNormal3fv(&norm[0]);
			glVertex3fv(&vert[0]);
		}
	}

	glEnd();
	
	glEndList ();
	
	generatedlistid = true;
	
	OPENGL_UTILITY::CheckForOpenGLErrors("model list ID generation", error_output);
}

void MODEL::ClearVertexArrayObject()
{
	if (generatedvao)
	{
		glBindBuffer(GL_ARRAY_BUFFER,0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
		if (!vbos.empty())
		{
			glDeleteBuffers(vbos.size(), &vbos[0]);
		}
		if (elementVbo != 0)
		{
			glDeleteBuffers(1, &elementVbo);
			elementVbo = 0;
		}
		if (vao != 0)
		{
			glBindVertexArray(0);
			glDeleteVertexArrays(1,&vao);
			vao = 0;
		}
	}
	generatedlistid = false;
}

template <typename T>
GLuint GenerateBufferObject(T * data, unsigned int i, unsigned int vertexCount, unsigned int elementsPerVertex, std::ostream & error_output)
{
	GLuint vboHandle;
	glGenBuffers(1, &vboHandle);ERROR_CHECK;
	glBindBuffer(GL_ARRAY_BUFFER, vboHandle);ERROR_CHECK;
	glBufferData(GL_ARRAY_BUFFER, vertexCount*elementsPerVertex*sizeof(T), data, GL_STATIC_DRAW);ERROR_CHECK;
	glVertexAttribPointer(i, elementsPerVertex, GL_FLOAT, GL_FALSE, 0, 0);ERROR_CHECK;
	glEnableVertexAttribArray(i);ERROR_CHECK;
	
	return vboHandle;
}

void MODEL::GenerateVertexArrayObject(std::ostream & error_output)
{
	if (generatedvao)
		return;
	
	// generate vertex array object
	glGenVertexArrays(1, &vao);ERROR_CHECK;
	glBindVertexArray(vao);ERROR_CHECK;
	
	// buffer object for faces
	const int * faces;
	int facecount;
	mesh.GetFaces(faces, facecount);
	glGenBuffers(1, &elementVbo);ERROR_CHECK;
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementVbo);ERROR_CHECK;
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, facecount*sizeof(GLuint), faces, GL_STATIC_DRAW);ERROR_CHECK;
	elementCount = facecount;
	
	// calculate the number of vertices (vertcount is the size of the verts array)
	const float * verts;
	int vertcount;
	mesh.GetVertices(verts, vertcount);
	unsigned int vertexCount = vertcount/3;
	
	// generate buffer object for vertex positions
	vbos.push_back(GenerateBufferObject(verts, VERTEX_POSITION, vertexCount, 3, error_output));
	
	// generate buffer object for normals
	const float * norms;
	int normcount;
	mesh.GetNormals(norms, normcount);
	assert((unsigned int)normcount == vertexCount*3);
	vbos.push_back(GenerateBufferObject(norms, VERTEX_NORMAL, vertexCount, 3, error_output));
	
	// TODO: generate tangent and bitangent
	glDisableVertexAttribArray(VERTEX_TANGENT);
	glDisableVertexAttribArray(VERTEX_BITANGENT);
	
	glDisableVertexAttribArray(VERTEX_COLOR);

	// generate buffer object for texture coordinates
	const float * tc[1];
	int tccount[1];
	if (mesh.GetTexCoordSets() > 0)
	{
		// TODO: make this work for UV1 and UV2
		mesh.GetTexCoords(0, tc[0], tccount[0]);
		assert((unsigned int)tccount[0] == vertexCount*2);
		vbos.push_back(GenerateBufferObject(tc[0], VERTEX_UV0, vertexCount, 2, error_output));
	}
	else
		glDisableVertexAttribArray(VERTEX_UV0);
	
	glDisableVertexAttribArray(VERTEX_UV1);
	glDisableVertexAttribArray(VERTEX_UV2);
	
	// don't leave anything bound
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER,0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
	
	generatedvao = true;
}

bool MODEL::GetVertexArrayObject(GLuint & vao_out, unsigned int & elementCount_out) const
{
	if (!generatedvao)
		return false;
	
	vao_out = vao;
	elementCount_out = elementCount;
	
	return true;
}

void MODEL::GenerateMeshMetrics()
{
	float maxv[3] = {0, 0, 0};
	float minv[3] = {0, 0, 0};
	bool havevals[6];
	for ( int n = 0; n < 6; n++ )
		havevals[n] = false;

	const float * verts;
	int vnum;
	mesh.GetVertices(verts, vnum);
	vnum = vnum / 3;
	for ( int v = 0; v < vnum; v++ )
	{
		MATHVECTOR <float, 3> temp;

		temp.Set ( verts + v*3 );
		
		//cout << verts[v*3] << "," << verts[v*3+1] << "," << verts[v*3+2] << endl;
	
		//cache for bbox stuff
		for ( int n = 0; n < 3; n++ )
		{
			if (!havevals[n])
			{
				maxv[n] = temp[n];
				havevals[n] = true;
			}
			else if (temp[n] > maxv[n])
				maxv[n] = temp[n];
			
			if (!havevals[n+3])
			{
				minv[n] = temp[n];
				havevals[n+3] = true;
			}
			else if (temp[n] < minv[n])
				minv[n] = temp[n];
		}

		float r = temp.Magnitude();
		MATHVECTOR <float, 2> tempxz;
		tempxz.Set(temp[0], temp[2]);
		float rxz = tempxz.Magnitude();
		if ( r > radius )
			radius = r;
		if ( rxz > radiusxz )
			radiusxz = rxz;
	}

	bboxmin.Set(minv[0], minv[1], minv[2]);
	bboxmax.Set(maxv[0], maxv[1], maxv[2]);
	
	MATHVECTOR <float, 3> center;
	center = (bboxmin + bboxmax)*0.5;
	radius = (bboxmin - center).Magnitude();
	
	MATHVECTOR <float, 3> minv_noy = bboxmin;
	minv_noy[1] = 0;
	center[1] = 0;
	radiusxz = (minv_noy - center).Magnitude();
	
	generatedmetrics = true;
}

void MODEL::ClearListID()
{
	if (generatedlistid) glDeleteLists(listid, 1);
	generatedlistid = false;
}

bool MODEL::WriteToFile(const std::string & filepath)
{
	const std::string magic = "OGLVARRAYV01";
	std::ofstream fileout(filepath.c_str());
	if (!fileout) return false;
	
	fileout.write(magic.c_str(), magic.size());
	joeserialize::BinaryOutputSerializer s(fileout);
	return Serialize(s);
}

bool MODEL::ReadFromFile(const std::string & filepath, std::ostream & error_output, bool generatelistid)
{
	const bool verbose = false;
	
	if (verbose) std::cout << filepath << ": starting mesh read" << std::endl;
	
	std::ifstream filein(filepath.c_str(), std::ios_base::binary);
	if (!filein)
	{
		error_output << "Can't find file: " << filepath << std::endl;
		return false;
	}
	//else std::cout << "File " << filepath << " exists!" << std::endl;
	
	const std::string magic = "OGLVARRAYV01";
	const int magicsize = 12;//magic.size();
	char fmagic[magicsize+1];
	filein.read(fmagic, magic.size());
	if (!filein)
	{
		error_output << "File magic read error: " << filepath << std::endl;
		return false;
	}
	
	fmagic[magic.size()] = '\0';
	std::string fmagicstr = fmagic;
	if (magic != fmagic)
	{
		error_output << "File magic is incorrect: \"" << magic << "\" != \"" << fmagic << "\" in " << filepath << std::endl;
		return false;
	}
	
	if (verbose) std::cout << filepath << ": serializing" << std::endl;
	
	/*// read the entire file into the memfile stream
	std::streampos start = filein.tellg();
	filein.seekg(0,std::ios::end);
	std::streampos length = filein.tellg() - start;
	filein.seekg(start);
	std::vector <char> buffer(length);
	filein.read(&buffer[0],length);
	std::stringstream memfile;
	memfile.rdbuf()->pubsetbuf(&buffer[0],length);*/
	
	joeserialize::BinaryInputSerializer s(filein);
	//joeserialize::BinaryInputSerializer s(memfile);
	if (!Serialize(s))
	{
		error_output << "Serialization error: " << filepath << std::endl;
		Clear();
		return false;
	}
	
	if (verbose) std::cout << filepath << ": generating metrics" << std::endl;
	
	ClearListID();
	ClearMetrics();
	GenerateMeshMetrics();
	if (verbose) std::cout << filepath << ": generating list id" << std::endl;
	
	if (generatelistid) GenerateListID(error_output);
	
	if (verbose) std::cout << filepath << ": done" << std::endl;
	
	return true;
}

void MODEL::SetVertexArray(const VERTEXARRAY & newmesh)
{
	Clear();
	mesh = newmesh;
}

void MODEL::BuildFromVertexArray(const VERTEXARRAY & newmesh, std::ostream & error_output)
{
	SetVertexArray(newmesh);
	
	//generate metrics such as bounding box, etc
	GenerateMeshMetrics();
}

void MODEL::Translate(float x, float y, float z)
{
	mesh.Translate(x, y, z);
}

void MODEL::Rotate(float a, float x, float y, float z)
{
	mesh.Rotate(a, x, y, z);
}
	
void MODEL::Scale(float x, float y, float z)
{
	mesh.Scale(x,y,z);
}
