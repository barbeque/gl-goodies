#ifndef _VERTEXBUFFER_H_
#define _VERTEXBUFFER_H_

#include <vector>
#include "IndexBuffer.h"
#include "GLee.h"

#ifndef __APPLE__
#include <GL/gl.h>
#else
#include <OpenGL/gl.h>
#endif

#include <cassert>

/// An enumeration representing various fixed vertex formats
enum VertexFormat {
	/// 2D vertex data only
	Vertex2 = 0,
	/// 3D vertex data only
	Vertex3 = 1,
	/// 3D vertex data, 2D texture coordinate, 3D normal
	Vertex3Texture2Normal3 = 2,
	/// 3D vertex data, 2D texture coordinate, 3D normal, RGBA colour
	Vertex3Texture2Normal3Colour4 = 3,
	/// 3D vertex data, 3D normal, RGBA Colour
	Vertex3Normal3Colour4 = 4
};

/**
	\brief A class representing a native vertex buffer representation.
	Vertex buffers are significantly faster than immediate mode.
*/
class VertexBuffer {
public:
	/// Whether or not the vertex buffer is supported in hardware.
	static bool IsSupported() {
		return (_GLEE_ARB_vertex_buffer_object != 0); 
	}
public:
	/// Indexed const fetch for an individual vertex component.
	float operator[](size_t index) const {
		assert(index >= 0 && index < this->size);
		return this->rawStorage[index];
	}
	/// Indexed fetch for an individual vertex component.
	float& operator[](size_t index) {
		assert(index >= 0 && index < this->size);
		return this->rawStorage[index];
	}
	/// Indexed fetch
	float Get(size_t index) {
		assert(index >= 0 && index < this->size);
		return this->rawStorage[index];
	}
	/// Writes "our" vertex buffer to the GPU. Do this after changing this instance.
	void Commit() const {
		this->Bind();
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, this->size * sizeof(float), this->rawStorage, GL_STATIC_DRAW_ARB);
	}
	/// Sets a specific vertex component's value
	void Set(size_t index, float value) {
		assert(index >= 0 && index < this->size);
		this->rawStorage[index] = value;
	}
public:
	/// Load the vertex buffer from a vector of vertex components
	void Read(const std::vector<float>& vertexData) {
		// Can't put in more vertices than the buffer holds.
		assert(vertexData.size() <= this->size);
		// Load 'em
		for(size_t i = 0; i < vertexData.size(); i++) {
			this->rawStorage[i] = vertexData[i];
		}
	}
public:
	/// Draw the vertex buffer as a certain kind of primitive.
	void Draw(GLenum primitiveType = GL_TRIANGLES) {
		glPushAttrib(GL_ALL_ATTRIB_BITS); // slow

		// Bind the vertex buffer to prepare it for being read by the GPU
		Bind();
				
		glEnableClientState(GL_VERTEX_ARRAY);
		// Prepare the client states we need (and set this->componentsPerVertex)
		SetUpStreams();
		
		// Set up the "striping" style to tell the GPU how to expect the data
		SetUpPointers(this->format);
		
		// Draw the array
		glDrawArrays(primitiveType, 0, this->size / this->componentsPerVertex);

		glDisableClientState(GL_VERTEX_ARRAY);
		
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		
		glPopAttrib(); // Dog slow.
	}
	
	/**
		\brief Draw the vertex buffer using an index buffer to control which vertices are drawn.
		\param indices			The index buffer to use. Ensure it is committed to the GPU.
		\param primitiveType	The OpenGL geometric primitive type to render these vertices as.
	*/
	void DrawIndexed(IndexBuffer& indices, GLenum primitiveType = GL_TRIANGLES) {	
		glPushAttrib(GL_ALL_ATTRIB_BITS);
		glEnableClientState(GL_VERTEX_ARRAY);
		
		this->SetUpStreams();
		
		indices.Bind();
		this->Bind();
		this->SetUpPointers(this->format);
		indices.DrawAll(primitiveType);
		
		glDisableClientState(GL_VERTEX_ARRAY);
		glPopAttrib();
	}
	
	/**
		\brief 	Draw the vertex buffer using a range of indices from the index buffer to control which
				vertices are drawn.
		\param indices			The index buffer to use. Ensure it is committed to the GPU.
		\param startIndex		The vertex in the index buffer to start rendering from.
		\param vertexCount		The number of vertices from the index buffer to render.
		\param primitiveType	The OpenGL geometric primitive type to render these vertices as.
	*/
	void DrawIndexed(IndexBuffer& indices, unsigned int startIndex, unsigned int vertexCount,
	 				 GLenum primitiveType = GL_TRIANGLES) {	
		glPushAttrib(GL_ALL_ATTRIB_BITS);
		glEnableClientState(GL_VERTEX_ARRAY);
		
		this->SetUpStreams();
		
		indices.Bind();
		this->Bind();
		this->SetUpPointers(this->format);
		indices.DrawRange(primitiveType, startIndex, vertexCount);
		
		glDisableClientState(GL_VERTEX_ARRAY);
		glPopAttrib();
	}
public:
	/// Get how "big" each vertex is in terms of components.
	int GetVertexStride() const {
		switch(this->format) {
			case Vertex2:
				return 2;
			case Vertex3:
				return 3;
			case Vertex3Texture2Normal3:
				return 8;
			case Vertex3Texture2Normal3Colour4:
				return 12;
			default:
				return 0; // Unknown format
		}
	}

	/// Get the number of vertices in the vertex buffer
	size_t GetVertexCount() const {
		return this->size / this->GetVertexStride();
	}
public:
	VertexBuffer(unsigned int size, VertexFormat format) {
		assert(size > 0);
		
		// Set our parameters
		this->size = size;
		this->format = format;
		
		// Create & initialize local shadow storage
		this->rawStorage = new float[this->size];
		for(size_t i = 0; i < this->size; i++) {
			this->rawStorage[i] = 0;
		}

		// Ask OpenGL to create a new vertex buffer handle for us
		glGenBuffersARB(1, &this->handle);
		assert(this->handle != 0); // GL screwed us.
		// Write the "blank" vertex buffer out to GPU memory, causing it to allocate
		// the proper space for us.
		this->Commit();
		
		this->componentsPerVertex = 1;
	}

	~VertexBuffer() {
		// Delete the vertex buffer from GPU-side.
		glDeleteBuffersARB(1, &this->handle);
		// Toss our shadow array
		delete[] this->rawStorage;
	}
private:
	void Bind() const {
		// Bind the vertex buffer for drawing on the GPU
		// If you're not rendering the right data, it may be because you forgot to Commit.
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, this->handle);
	}

	void SetUpPointers(VertexFormat format) {
		switch(format) {
			case Vertex2:
				glVertexPointer(2, GL_FLOAT, 0, 0);
				this->componentsPerVertex = 2;
				break;
			case Vertex3:
				glVertexPointer(3, GL_FLOAT, 0, 0);
				this->componentsPerVertex = 3;
				break;
			case Vertex3Texture2Normal3:
				glVertexPointer(3, GL_FLOAT,	8 * sizeof(float),	0);
				glTexCoordPointer(2, GL_FLOAT,	8 * sizeof(float),	(void*)(3 * sizeof(float)));
				glNormalPointer(GL_FLOAT,		8 * sizeof(float),	(void*)(5 * sizeof(float)));
				this->componentsPerVertex = 8;
				break;
			case Vertex3Texture2Normal3Colour4:
				glVertexPointer(3, GL_FLOAT,	12 * sizeof(float),	0);
				glTexCoordPointer(2, GL_FLOAT,	12 * sizeof(float),	(void*)(3 * sizeof(float)));
				glNormalPointer(GL_FLOAT,		12 * sizeof(float),	(void*)(5 * sizeof(float)));
				glColorPointer(4, GL_FLOAT,		12 * sizeof(float),	(void*)(8 * sizeof(float)));
				this->componentsPerVertex = 12;
				break;
			case Vertex3Normal3Colour4:
				glVertexPointer(3, GL_FLOAT,	10 * sizeof(float),0);
				glNormalPointer(GL_FLOAT,		10 * sizeof(float), (void*)(3 * sizeof(float)));
				glColorPointer(4, GL_FLOAT,		10 * sizeof(float), (void*)(6 * sizeof(float)));
				break;
			default:
				assert(false); // Unknown vertex type
				break;
		}
	}

	inline void SetUpStreams() const {
		if(this->format == Vertex3Texture2Normal3 || this->format == Vertex3Texture2Normal3Colour4) {
			// Enable the texture client state, since we need textures.
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		}
		if(this->format == Vertex3Texture2Normal3 || this->format == Vertex3Texture2Normal3Colour4 || this->format == Vertex3Normal3Colour4) {	
			// Enable the normal client state too.
			glEnableClientState(GL_NORMAL_ARRAY);
		}
		if(this->format == Vertex3Texture2Normal3Colour4 ||	this->format == Vertex3Normal3Colour4) {
			// We'll need colour too
			glEnableClientState(GL_COLOR_ARRAY);
		}
	}
private:
	VertexFormat format;
	float* rawStorage;
	GLuint handle;
	/// Size (in components)
	unsigned int size;
	/// The number of components per vertex
	unsigned int componentsPerVertex;
};

#endif
