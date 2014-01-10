#include "ObjLoader.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <limits>
#include <iostream>
#include <cstdlib>
#include <cassert>

//--------------------------------------------------------------------------

/**
 \brief Calculate a vertex normal.
 \param vertexIndex	The zero-indexed index of the vertex normal to use.
 \param vertices		The vertex pool.
 \param triangles	The triangle pool.
 */
Vector3 CalculateVertexNormal(unsigned int vertexIndex, const std::vector<ObjVertex>& vertices, std::vector<ObjTriangle>& triangles) {
	// First, find the triangles that use this vertex (slooowwwww)
	Vector3 aggregateFaceNormal;
	unsigned int adjacentFaces = 0;
	for(unsigned int triangle = 0; triangle < triangles.size(); triangle++) {
		if(triangles[triangle].GetVertexIndex(0) - 1 == vertexIndex || triangles[triangle].GetVertexIndex(1) - 1 == vertexIndex || triangles[triangle].GetVertexIndex(2) - 1 == vertexIndex) {
			// Triangle uses this vertex
			aggregateFaceNormal += triangles[triangle].GetFaceNormal(vertices);
			++adjacentFaces;
		}
	}
	
	return (aggregateFaceNormal / adjacentFaces).normalize();
}

//--------------------------------------------------------------------------

inline bool RayTriangleCollision(Vector3 rayOrigin, Vector3 rayDirection, Vector3* triangleVertices, float& t) {
	// Distance will be returned as a t
	Vector3 e1, e2, h, s, q;
	float a, f, u, v;
	
	e1 = triangleVertices[1] - triangleVertices[0];
	e2 = triangleVertices[2] - triangleVertices[0];
	h = cross(rayDirection, e2);
	a = e1.dot(h);
	
	// Parallel check
	if(a > -0.00001f && a < 0.00001f) {
		return false;
	}
	
	f = 1.0f / a;
	s = rayOrigin - triangleVertices[0];
	
	u = f * s.dot(h);
	
	// Barycentric range check
	if(u < 0.0f || u > 1.0f) {
		return false;
	}
	
	q = cross(s, e1);
	
	v = f * rayDirection.dot(q);
	if(v < 0.0f || u + v > 1.0f) {
		return false;
	}
	
	t = f * e2.dot(q);
	if(t > 0.00001f) {
		// Hit!
		return true;
	}
	
	return false;
}

void TriangleMeshInternalDepth::Calculate(std::vector<ObjTriangle>& triangles, std::vector<ObjVertex>& vertices) {
	this->distances.clear();
	
	std::cout << "Generating vertex depth information." << std::endl;
	
	for(size_t i = 0; i < vertices.size(); i++) {
		ObjVertex& thisVertex = vertices[i];
		float minimumDepth = std::numeric_limits<float>::max();
		Vector3 vertexPosition = Vector3(thisVertex.x, thisVertex.y, thisVertex.z);
		// Make a new vector for the vertex normal that is INVERSE!!!
		Vector3 vertexNormalRay = Vector3(thisVertex.normalX, thisVertex.normalY, thisVertex.normalZ).normalize() * -1;
		
		for(size_t t = 0; t < triangles.size(); t++) {
			ObjTriangle& thisTriangle = triangles[t];
			float depth = 0.0f;
			
			Vector3 triangleVertices[3];
			for(size_t v = 0; v < 3; v++) {
				// Could try caching this, but who cares
				ObjVertex& referencedVertex = vertices[thisTriangle.GetVertexIndex(v) - 1];
				triangleVertices[v] = Vector3(referencedVertex.x, referencedVertex.y, referencedVertex.z);
			}
			
			if(RayTriangleCollision(vertexPosition, vertexNormalRay, triangleVertices, depth)) {
				if(depth < minimumDepth) {
					minimumDepth = depth;
				}
			}
		}
		
		// Assign the minimum depth
		this->distances.push_back(minimumDepth);
	}
	
	assert(this->distances.size() == vertices.size()); // Make sure they're all there!
}

void TriangleMeshInternalDepth::WriteDepthAsTextureCoordinates(VertexBuffer* vertices, const std::vector<ObjTriangle>& triangles) const {
	assert(this->distances.size() > 0); // make sure they were calculated to start with
	
	unsigned int vertexSize = vertices->GetVertexStride();
	
	for(unsigned int t = 0; t < triangles.size(); t++) {
		const ObjTriangle& thisTriangle = triangles[t];
		unsigned int baseAddress = (t * vertexSize * 3); // 3 verts/tri
		
		for(unsigned int v = 0; v < 3; v++) {
			// Look up the depth that this vertex of the triangle uses from our table
			unsigned int vertexIndex = (thisTriangle.GetVertexIndex(v) - 1);
			float thisVertexDepth = this->distances[vertexIndex];
			unsigned int vertexOffset = vertexSize * v;
			unsigned int texCoordUOffset = 3; // 4th object in the vertex (Vertex3Texture2Normal3)
			
			// Write it into the VBO
			(*vertices)[baseAddress + vertexOffset + texCoordUOffset] = thisVertexDepth;
		}
	}
	
	// Now that we're done, commit the vertices to GPU
	vertices->Commit();
}

float TriangleMeshInternalDepth::GetVertexInternalDistance(size_t vertexIndex) const {
	return this->distances[vertexIndex];
}

// --------------------------------------------------------------

// Tokenize a group of format x/[y]/[z]
std::vector<std::string> tokenizeGroup(const std::string& str) {
	std::vector<std::string> grouped(3);
	std::string currentString = "";
	
	int group = 0;
	for(unsigned int i = 0; i < str.length(); i++) {
		if(str[i] != '/') {
			currentString += str[i]; // append it
		}
		else {
			// Next group!
			grouped[group] = currentString;
			currentString = "";
			group++;
		}
	}
	
	// We ran out of text
	if(str.length() > 0) {
		if(grouped[group].size() == 0) {
			// We didn't get a chance to commit it
			grouped[group] = currentString;
		}
	}
	
	return grouped;
}

std::vector<std::string> SplitString(const std::string& str, char splitCharacter) {
	std::vector<std::string> output;
	std::string currentString;
	
	for(size_t i = 0; i < str.length(); i++) {
		if(str[i] != splitCharacter) {
			// Keep going
			currentString += str[i];
		}
		else {
			// Make a split
			output.push_back(currentString);
			currentString = "";
		}
	}
	
	// Add in the last chunk
	if(currentString.length() > 0) {
		output.push_back(currentString);
	}
	
	return output;
}

// --------------------------------------------------------------

MeshGeometry ObjLoader::LoadMesh(const std::string& path) const {
	
	// Try opening the file for starters.
	std::ifstream input(path.c_str());
	std::string buffer;
	
	unsigned int numberOfTextureCoordinates = 0;
	unsigned int numberOfNormals = 0;
	unsigned int numberOfTriangles = 0;
	unsigned int numberOfVertices = 0;
	
	// Create the geometry cache object
	MeshGeometry output;
	output.vertices = NULL;
	output.indices = NULL;
	
	if(!input.is_open()) {
		// Load failed (file not found)
		std::cerr << "Could not open OBJ file \"" + path + "\"!" << std::endl;
	}
	else {
		// First we'll scan to figure out how many vertices,
		// texture coordinates, normals and triangles there are
		// in the file.
		while(!input.eof()) {
			// Read each line
			std::getline(input, buffer);
			
			if(buffer.substr(0, 2) == "vn") {
				// Vertex normal
				numberOfNormals++;
			}
			else if(buffer.substr(0, 2) == "vt") {
				// Vertex texture coordinate
				numberOfTextureCoordinates++;
			}
			else if(buffer.substr(0, 1) == "v") {
				// Vertex
				numberOfVertices++;
			}
			else if(buffer.substr(0, 1) == "f") {
				// Face
				
				// Not necessarily a triangle, so now we gotta count it.
				std::vector<std::string> faceParts = SplitString(buffer, ' ');
				
				numberOfTriangles++;
				if(faceParts.size() > 4) {
					// We have to make another triangle for this face. It's a quad.
					numberOfTriangles++;
				}
			}
		}
		
		// Instantiate the arrays
		std::vector<ObjVertex> vertices(numberOfVertices);
		std::vector<ObjNormal> normals(std::max(1u, numberOfNormals)); // if no normals, just predefine some
		std::vector<ObjTextureCoordinate> textureCoordinates(std::max(1u, numberOfTextureCoordinates));
		std::vector<ObjTriangle> triangles(numberOfTriangles);
		
		// Reset the read pointer, otherwise it will just keep shouting eof.		
		input.clear();
		input.seekg(0, std::ios::beg);
		
		if(!input.is_open()) {
			std::cerr << "Could not reopen OBJ file for second stage load" << std::endl;
		}
		else {
			// Keep reading through line by line and break down the format now			
			int normal = 0;
			int textureCoordinate = 0;
			int vertex = 0;
			int triangle = 0;
			
			while(!input.eof()) {
				// Fetch the line
				std::getline(input, buffer);
				// Create a stringstream for fast searching
				std::istringstream line(buffer);
				
				if(buffer.substr(0, 2) == "vn") {
					std::string temp, f1, f2, f3;
					// Parse out some floats and the parameters
					// Format vn nx ny nz
					line >> temp >> f1 >> f2 >> f3;
					float x = (float)atof(f1.c_str());
					float y = (float)atof(f2.c_str());
					float z = (float)atof(f3.c_str());
					normals[normal].x = x;
					normals[normal].y = y;
					normals[normal].z = z;
					normal++;
				}
				else if(buffer.substr(0, 2) == "vt") {
					// format: vt u v
					std::string temp, f1, f2;
					line >> temp >> f1 >> f2;
					float u = (float)atof(f1.c_str());
					float v = (float)atof(f2.c_str());
					textureCoordinates[textureCoordinate].u = u;
					textureCoordinates[textureCoordinate].v = v;
					textureCoordinate++;
				}
				else if(buffer.substr(0, 1) == "v") {
					// format: v x y z
					std::string temp, f1, f2, f3;
					line >> temp >> f1 >> f2 >> f3;
					float x = (float)atof(f1.c_str());
					float y = (float)atof(f2.c_str());
					float z = (float)atof(f3.c_str());
					vertices[vertex].x = x;
					vertices[vertex].y = y;
					vertices[vertex].z = z;
					vertex++;
				}
				else if(buffer.substr(0, 1) == "f") {
					// Format: vertexIndex1/[textureIndex1]/[normalIndex1] vertexIndex2/[textureIndex2]/[normalIndex2] vertexIndex3/[textureIndex3]/[normalIndex3]
					
					std::vector<std::string> vertices = SplitString(buffer, ' ');
					
					// Now parse them all.
					std::vector<std::string> v1Parts = tokenizeGroup(vertices[1]);
					std::vector<std::string> v2Parts = tokenizeGroup(vertices[2]);
					std::vector<std::string> v3Parts = tokenizeGroup(vertices[3]);
					
					// Load the vertex indexes
					triangles[triangle].SetVertexIndex(0, (v1Parts[0].length() > 0) ? atoi(v1Parts[0].c_str()) : 1);
					triangles[triangle].SetVertexIndex(1, (v2Parts[0].length() > 0) ? atoi(v2Parts[0].c_str()) : 1);
					triangles[triangle].SetVertexIndex(2, (v3Parts[0].length() > 0) ? atoi(v3Parts[0].c_str()) : 1);
					
					// Load the texture coordinates	(if present)
					triangles[triangle].SetTextureCoordinateIndex(0, (v1Parts[1].length() > 0) ? atoi(v1Parts[1].c_str()) : 1);
					triangles[triangle].SetTextureCoordinateIndex(1, (v2Parts[1].length() > 1) ? atoi(v2Parts[1].c_str()) : 1);
					triangles[triangle].SetTextureCoordinateIndex(2, (v3Parts[1].length() > 1) ? atoi(v3Parts[1].c_str()) : 1);
					
					// Load the normals (if present)
					triangles[triangle].SetNormalIndex(0, (v1Parts[2].length() > 0) ? atoi(v1Parts[2].c_str()) : 1);
					triangles[triangle].SetNormalIndex(1, (v2Parts[2].length() > 0) ? atoi(v2Parts[2].c_str()) : 1);
					triangles[triangle].SetNormalIndex(2, (v3Parts[2].length() > 0) ? atoi(v3Parts[2].c_str()) : 1);
					
					triangle++;
					
					if(vertices.size() > 4) {
						// Making a quad.
						// Make another triangle
						std::vector<std::string> v4Parts = tokenizeGroup(vertices[4]);
						
						// Use the previous two vertices
						triangles[triangle].SetVertexIndex(0, (v1Parts[0].length() > 0) ? atoi(v1Parts[0].c_str()) : 1);
						triangles[triangle].SetVertexIndex(1, (v3Parts[0].length() > 0) ? atoi(v3Parts[0].c_str()) : 1);
						triangles[triangle].SetVertexIndex(2, (v4Parts[0].length() > 0) ? atoi(v4Parts[0].c_str()) : 1);
						
						triangles[triangle].SetTextureCoordinateIndex(0, (v1Parts[1].length() > 1) ? atoi(v1Parts[1].c_str()) : 1);
						triangles[triangle].SetTextureCoordinateIndex(1, (v3Parts[1].length() > 1) ? atoi(v3Parts[1].c_str()) : 1);
						triangles[triangle].SetTextureCoordinateIndex(2, (v4Parts[1].length() > 1) ? atoi(v4Parts[1].c_str()) : 1);
						
						triangles[triangle].SetNormalIndex(0, (v1Parts[2].length() > 0) ? atoi(v1Parts[2].c_str()) : 1);
						triangles[triangle].SetNormalIndex(1, (v3Parts[2].length() > 0) ? atoi(v3Parts[2].c_str()) : 1);
						triangles[triangle].SetNormalIndex(2, (v4Parts[2].length() > 0) ? atoi(v4Parts[2].c_str()) : 1);
						
						++triangle;
					}
				}
			}
			
			// We're done, close it out
			input.close();
			
			
			// ATTEMPTING TO CENTER MODEL

			float minX = std::numeric_limits<float>::max();
			float maxX = std::numeric_limits<float>::min();
			float minY = std::numeric_limits<float>::max();
			float maxY = std::numeric_limits<float>::min();
			float minZ = std::numeric_limits<float>::max();
			float maxZ = std::numeric_limits<float>::min();
			
			float xSum = 0.0f;
			float ySum = 0.0f;
			float zSum = 0.0f;
			
			/*for(unsigned int i = 0; i < triangles.size(); i++) {
				for(unsigned int v = 0; v < 3; v++) {
					minX = std::min(minX, vertices[triangles[i].GetVertexIndex(v) - 1].x);
					maxX = std::max(maxX, vertices[triangles[i].GetVertexIndex(v) - 1].x);
					minY = std::min(minY, vertices[triangles[i].GetVertexIndex(v) - 1].y);
					maxY = std::max(maxY, vertices[triangles[i].GetVertexIndex(v) - 1].y);
					minZ = std::min(minZ, vertices[triangles[i].GetVertexIndex(v) - 1].z);
					maxZ = std::max(maxZ, vertices[triangles[i].GetVertexIndex(v) - 1].z);
					
					xSum += vertices[triangles[i].GetVertexIndex(v) - 1].x;
					ySum += vertices[triangles[i].GetVertexIndex(v) - 1].y;
					zSum += vertices[triangles[i].GetVertexIndex(v) - 1].z;
				}
			}*/
			
			for(unsigned int v = 0; v < vertices.size(); v++) {
				minX = std::min(minX, vertices[v].x);
				maxX = std::max(maxX, vertices[v].x);
				minY = std::min(minY, vertices[v].y);
				maxY = std::max(maxY, vertices[v].y);
				minZ = std::min(minZ, vertices[v].z);
				maxZ = std::max(maxZ, vertices[v].z);
				
				xSum += vertices[v].x;
				ySum += vertices[v].y;
				zSum += vertices[v].z;
			}
			
			float aveX = xSum / vertices.size();//(minX + maxX) / 2.0f;
			float aveY = ySum / vertices.size();//(minY + maxY) / 2.0f;
			float aveZ = zSum / vertices.size();//(minZ + maxZ) / 2.0f;
			std::cout << "Maximum dimensions: [" << minX << "," << maxX << "] [" << minY << "," << maxY << "] [" << minZ << "," << maxZ << "]" << std::endl;
			std::cout << "Average: [" << aveX << "," << aveY << "," << aveZ << "]" << std::endl;
			
			for(unsigned int v = 0; v < vertices.size(); v++) {
				vertices[v].x -= aveX;
				vertices[v].y -= aveY;
				vertices[v].z -= aveZ;
			}
			
			/*for (unsigned int i = 0;i < triangles.size();i++) {
				for (unsigned int v=0;v<3;v++) {
					vertices[triangles[i].GetVertexIndex(v) - 1].x -= aveX;
					vertices[triangles[i].GetVertexIndex(v) - 1].y -= aveY;
					vertices[triangles[i].GetVertexIndex(v) - 1].z -= aveZ;
				}
			}*/
			
			// END ATTEMPTING TO CENTER MODEL
			
			// Figure out the scales so we can rope this thing down
			float width = std::numeric_limits<float>::min();
			float height = std::numeric_limits<float>::min();
			float depth = std::numeric_limits<float>::min();
			 
			for(unsigned int i = 0; i < triangles.size(); i++) {
				for(unsigned int v = 0; v < 3; v++) {
					width = std::max(width, std::fabs(vertices[triangles[i].GetVertexIndex(v) - 1].x));
					height = std::max(height, std::fabs(vertices[triangles[i].GetVertexIndex(v) - 1].y));
					float z = std::fabs(vertices[triangles[i].GetVertexIndex(v) - 1].z);
					depth = std::max(depth, z);
				}
			 }
			
			std::cout << "New dimensions: [" << width << "," << height << "," << depth << "]" << std::endl;
			
			// Build the vertex buffer.
			VertexBuffer* vb = new VertexBuffer(numberOfTriangles * 3 * 8, Vertex3Texture2Normal3); // 8 for Vertex3Texture2Normal3
			IndexBuffer* ib = new IndexBuffer(numberOfTriangles * 3);
			
			// Clamp the size of the model so that the biggest axis is normalized to 1.0f world units
			float scale = std::max(0.5f, std::max(width, std::max(height, depth)));

			std::cout << "Calculating normals" << std::endl;

			struct TemporaryTriangle {
				float x;
				float y;
				float z;

				float u;
				float v;

				float normalX;
				float normalY;
				float normalZ;
			};

			// Rebuild all the vertex normals
			for(unsigned int i = 0; i < vertices.size(); i++) {
				Vector3 vertexNormal = CalculateVertexNormal(i, vertices, triangles);
				vertices[i].normalX = vertexNormal[0];
				vertices[i].normalY = vertexNormal[1];
				vertices[i].normalZ = vertexNormal[2];
			}
			
			// Scale the vertices
			for(unsigned int v = 0; v < vertices.size(); v++) {
				float adjustedScale = 1.0f / (scale * 2.0f);
				
				vertices[v].x *= adjustedScale;
				vertices[v].y *= adjustedScale;
				vertices[v].z *= adjustedScale;
			}
			
			// Now that all the vertices are set up, calculate the vertex depths before we
			// write the whole thing into a vertex buffer
			
			// Compute vertex depths (assuming that the vertices already have their normals calculated)
			output.internalDepthInformation.Calculate(triangles, vertices);
			
			// Final preparation and then write the triangles into the vertex buffer.
			for(unsigned int i = 0; i < triangles.size(); i++) {
				for(unsigned int v = 0; v < 3; v++) {
					TemporaryTriangle triangle;

					// Vertex (3)
					
					triangle.x = vertices[triangles[i].GetVertexIndex(v) - 1].x;
					triangle.y = vertices[triangles[i].GetVertexIndex(v) - 1].y;
					triangle.z = vertices[triangles[i].GetVertexIndex(v) - 1].z;
					
					// Some quickie assertions to make sure we're sane.
					assert(!(triangle.x != triangle.x)); // nan check
					assert(!(triangle.y != triangle.y));
					assert(!(triangle.z != triangle.z));
					assert(triangle.x <= 1.0f);
					assert(triangle.x >= -1.0f);
					assert(triangle.y <= 1.0f);
					assert(triangle.y >= -1.0f);
					assert(triangle.z <= 1.0f);
					assert(triangle.z >= -1.0f);
					
					// Texture (2)
					triangle.u = textureCoordinates[triangles[i].GetTextureCoordinateIndex(v) - 1].u;
					triangle.v = textureCoordinates[triangles[i].GetTextureCoordinateIndex(v) - 1].v;
					
					// Normal (3)
					unsigned int normalIndex = triangles[i].GetNormalIndex(v) - 1;
					assert(normalIndex < normals.size());

					// We calculated the vertex normals already, so just use 'em
					triangle.normalX = vertices[triangles[i].GetVertexIndex(v) - 1].normalX;
					triangle.normalY = vertices[triangles[i].GetVertexIndex(v) - 1].normalY;
					triangle.normalZ = vertices[triangles[i].GetVertexIndex(v) - 1].normalZ;

					// Load the triangle in
					vb->Set(i * 24 + (v * 8) + 0, triangle.x);
					vb->Set(i * 24 + (v * 8) + 1, triangle.y);
					vb->Set(i * 24 + (v * 8) + 2, triangle.z);

					vb->Set(i * 24 + (v * 8) + 3, triangle.u);
					vb->Set(i * 24 + (v * 8) + 4, triangle.v);
					
					vb->Set(i * 24 + (v * 8) + 5, triangle.normalX);
					vb->Set(i * 24 + (v * 8) + 6, triangle.normalY);
					vb->Set(i * 24 + (v * 8) + 7, triangle.normalZ);
				}
				
				// Make the index buffer now
				(*ib)[i * 3] = triangles[i].GetVertexIndex(0) - 1;
				(*ib)[i * 3 + 1] = triangles[i].GetVertexIndex(1)- 1;
				(*ib)[i * 3 + 2] = triangles[i].GetVertexIndex(2) - 1;
			}
			
			// Commit the buffers
			vb->Commit();
			ib->Commit();			
			
			// Set the output properly.
			output.vertices = vb;
			output.indices = ib;
			output.scale = scale;
			
			// Encode the depth information into the vertex buffer, overwriting texture coordinates!
			output.internalDepthInformation.WriteDepthAsTextureCoordinates(output.vertices, triangles);
		}
	}
	
	return output;
}

// --------------------------------------------------------------
