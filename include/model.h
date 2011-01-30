#ifndef _MODEL_H
#define _MODEL_H

#include "opengl_utility.h"
#include "vertexarray.h"
#include "mathvector.h"
#include "aabb.h"
#include "joeserialize.h"
#include "macros.h"

#include <string>
#include <iostream>
#include <cassert>
#include <vector>

///loading data into the mesh vertexarray is implemented by derived classes
class MODEL
{
friend class joeserialize::Serializer;
public:
	MODEL();
	
	MODEL(const std::string & filepath, std::ostream & error_output);
	
	virtual ~MODEL();
	
	virtual bool Load(const std::string & strFileName, std::ostream & error_output, bool genlist) {return false;}
	
	virtual bool CanSave() const {return false;}  ///< returns true if the model format is capable of saving to a file
	
	virtual bool Save(const std::string & strFileName, std::ostream & error_output) const {return false;} ///< optional capability
	
	bool Serialize(joeserialize::Serializer & s)
	{
		_SERIALIZE_(s,mesh);
		return true;
	}
	
	bool WriteToFile(const std::string & filepath);
	
	bool ReadFromFile(const std::string & filepath, std::ostream & error_output, bool generatelistid=true);
	
	void GenerateListID(std::ostream & error_output);
	
	void GenerateVertexArrayObject(std::ostream & error_output);
	bool HaveVertexArrayObject() const {return generatedvao;}
	void ClearVertexArrayObject();
	
	/// returns true if we have a vertex array object and stores the VAO handle and element count in the provided
	/// arguments; returns false if we have no vertex array object
	bool GetVertexArrayObject(GLuint & vao_out, unsigned int & elementCount_out);
	
	void GenerateMeshMetrics();
	
	void ClearMeshData() {mesh.Clear();}
	
	int GetListID() const {RequireListID(); return listid;}
	
	//"metrics"
	float GetRadius() const {RequireMetrics();return radius + 0.5f;}
	
	float GetRadiusXZ() const {RequireMetrics();return radiusxz;}
	
	MATHVECTOR <float, 3> GetCenter() {return (bboxmax + bboxmin) * 0.5;}
	
	bool HaveMeshData() const {return (mesh.GetNumFaces() > 0);}
	
	bool HaveMeshMetrics() const {return generatedmetrics;}
	
	bool HaveListID() const {return generatedlistid;}
	
	void Clear() {ClearMeshData();ClearListID();ClearVertexArrayObject();ClearMetrics();}
	
	const VERTEXARRAY & GetVertexArray() const {return mesh;}
	
	void SetVertexArray(const VERTEXARRAY & newmesh);
	
	void BuildFromVertexArray(const VERTEXARRAY & newmesh, std::ostream & error_output);
	
	bool Loaded() {return (mesh.GetNumFaces() > 0);}

	void Translate(float x, float y, float z);

	void Rotate(float a, float x, float y, float z);
	
	void Scale(float x, float y, float z);
	
	AABB <float> GetAABB() const
	{
		AABB <float> output;
		output.SetFromCorners(bboxmin, bboxmax);
		return output;
	}

protected:
	///to be filled by the derived classes
	VERTEXARRAY mesh;
	
private:
	bool generatedlistid;
	bool generatedmetrics;
	int listid;
	
	bool generatedvao; // vao means vertex array object
	GLuint vao;
	std::vector <GLuint> vbos;
	GLuint elementVbo;
	unsigned int elementCount;
	
	//metrics
	float radius;
	float radiusxz;
	MATHVECTOR <float, 3> bboxmin;
	MATHVECTOR <float, 3> bboxmax;
	
	void RequireMetrics() const
	{
		//Mesh metrics need to be generated before they can be queried
		assert(generatedmetrics);
	}
	
	void RequireListID() const
	{
		//Mesh id needs to be generated
		assert(generatedlistid);
	}
	
	void ClearListID();
	
	void ClearMetrics() {generatedmetrics = false;}
};

#endif


