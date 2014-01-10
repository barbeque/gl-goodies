#ifndef _585_OBJLOADER_H_
#define _585_OBJLOADER_H_

#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include <string>
#include "Vector.h"
//#include <Vector>

//--------------------------------------------------------------------------

/// A 3D vertex in the OBJ file format
class ObjVertex { 
public:
	ObjVertex() { x = y = z = normalX = normalY = normalZ = 0.0f; }
public:
	float x;
	float y;
	float z;
	
	float normalX;
	float normalY;
	float normalZ;
};

/// A 3D normal in the OBJ file format
class ObjNormal { 
public:
	ObjNormal() { x = y = z = 0.0f; }
	float x;
	float y;
	float z;
};

/// A 2D texture coordinate in the OBJ file format
class ObjTextureCoordinate {
public:
	ObjTextureCoordinate() { u = v = 0.0f; }
	float u;
	float v;
};

/// A triangle in the OBJ file format
class ObjTriangle {
public:
	ObjTriangle() {
		for(int i = 0; i < 3; i++) {
			vertexIndices[i] = normalIndices[i] = textureCoordinateIndices[i] = 0;
		}
		this->isFaceNormalComputedYet = false;
	}
	
	unsigned int GetVertexIndex(int vertex) const {
		assert(vertex >= 0 && vertex < 3);
		unsigned int vertexIndex = vertexIndices[vertex];
		assert(vertexIndex > 0); // they start from 1
		return vertexIndex;
	}
	
	unsigned int GetNormalIndex(int vertex) const {
		assert(vertex >= 0 && vertex < 3);
		unsigned int normalIndex = normalIndices[vertex];
		assert(normalIndex > 0); // super paranoid mode
		return normalIndex;
	}
	
	unsigned int GetTextureCoordinateIndex(int vertex) const {
		assert(vertex >= 0 && vertex < 3);
		unsigned int textureCoordinateIndex = textureCoordinateIndices[vertex];
		assert(textureCoordinateIndex > 0);
		return textureCoordinateIndex;
	}
	
	void SetVertexIndex(int vertex, unsigned int index) {
		assert(vertex >= 0 && vertex < 3);
		assert(index > 0);
		vertexIndices[vertex] = index;
	}
	
	void SetNormalIndex(int vertex, unsigned int index) {
		assert(vertex >= 0 && vertex < 3);
		assert(index > 0);
		normalIndices[vertex] = index;
	}
	
	void SetTextureCoordinateIndex(int vertex, unsigned int index) {
		assert(vertex >= 0 && vertex < 3);
		assert(index > 0);
		textureCoordinateIndices[vertex] = index;
	}
	
	Vector3 GetFaceNormal(const std::vector<ObjVertex>& vertices) {
		if(this->isFaceNormalComputedYet) {
			// Pull it out of the cache
			return this->faceNormal;
		}
		else {
			Vector3 triangleVertices[3];
			for(size_t i = 0; i < 3; i++) {
				float x = vertices[this->vertexIndices[i] - 1].x;
				float y = vertices[this->vertexIndices[i] - 1].y;
				float z = vertices[this->vertexIndices[i] - 1].z;
				triangleVertices[i] = Vector3(x, y, z);
			}
			
			// Okay off we go, AB x AC
			Vector3 ab = triangleVertices[1] - triangleVertices[0];
			Vector3 ac = triangleVertices[2] - triangleVertices[0];
			
			this->faceNormal = cross(ab, ac);
			this->isFaceNormalComputedYet = true; // Warn us so we just get it from cache next
			
			return this->faceNormal;
		}
	}
	
private:
	unsigned int vertexIndices[3];
	unsigned int normalIndices[3];
	unsigned int textureCoordinateIndices[3];
	bool isFaceNormalComputedYet;
	Vector3 faceNormal;
};

//--------------------------------------------------------------------------

class TriangleMeshInternalDepth {
public:
	/// Compute the internal depth storage from the vertices
	void Calculate(std::vector<ObjTriangle>& triangles, std::vector<ObjVertex>& vertices);
	/// Write the calculated depth to a vertex buffer of triangles
	void WriteDepthAsTextureCoordinates(VertexBuffer* vertices, const std::vector<ObjTriangle>& triangles) const;
	/**
	 \brief Retrieve the internal distance of a given vertex.
	 \param triangleIndex	A zero-indexed triangle index.
	 \return					The distance, in scaled units.
	 */
	float GetVertexInternalDistance(size_t vertexIndex) const;
private:
	std::vector<float> distances;
};

struct MeshGeometry {
	VertexBuffer* vertices;
	IndexBuffer* indices;
	float scale;
	TriangleMeshInternalDepth internalDepthInformation;
};

/// A mesh loader for the Alias/Wavefront OBJ file format.
class ObjLoader {
public:
	virtual ~ObjLoader() { }
public:
	virtual MeshGeometry LoadMesh(const std::string& path) const;
};

#endif
