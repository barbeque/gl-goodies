#ifndef _585_INDEXBUFFER_H_
#define _585_INDEXBUFFER_H_

#include "GLee.h"
#include <vector>
#include <cassert>

/// The C type of the indices in the index buffer. Smaller types save GPU and main memory.
typedef unsigned short IndexType;
/// The IndexType as OpenGL understands it.
#define GL_INDEX_TYPE GL_UNSIGNED_SHORT

class IndexBuffer {
public:
	/// Constant index operator, for fetching a single index
	IndexType operator[](size_t index) const {
		assert(index >= 0 && index < this->size);
		return this->rawStorage[index];
	}
	/// Non-constant operator, for writing or reading a single index
	IndexType& operator[](size_t index) {
		assert(index >= 0 && index < this->size);
		return this->rawStorage[index];
	}
	/// Write the index buffer to the GPU, allocating the space we need
	void Commit() const {
		this->Bind();
		glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, this->size * sizeof(IndexType), rawStorage, GL_STATIC_DRAW_ARB);
	}
	/// Bind the index buffer to the GPU state, preparing it for rendering
	void Bind() const {
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, this->handle);
	}
	/**
	 	\brief Draw all elements of the bound vertex buffer using this (bound) index buffer
		\param primitiveType	The GL primitive type to draw
	*/
	void DrawAll(GLenum primitiveType = GL_TRIANGLES) const {
		glDrawElements(primitiveType, this->size, GL_INDEX_TYPE, NULL);
	}
	/**
		\brief Draw a selected set of elements on the bound vertex buffer using the (bound) index buffer
		\param primitiveType	The GL primitive type to draw
		\param startIndex		The index in the index buffer to start drawing at
		\param vertexCount		The number of indices in the index buffer to use (i.e. the number of vertices drawn)
	*/
	void DrawRange(GLenum primitiveType, unsigned int startIndex, unsigned int vertexCount) const {
		glDrawRangeElements(primitiveType, startIndex, startIndex + vertexCount, vertexCount, GL_INDEX_TYPE, NULL);
	}
	/**
		\brief Instantiate the index buffer.
		\param size	The number of indices to be stored in the buffer.
	*/
	IndexBuffer(unsigned int size) {
		assert(size > 0);
		
		this->handle = 0;
		
		this->size = size;
		// Allocate shadow storage
		this->rawStorage = new IndexType[this->size];
		for(size_t i = 0; i < this->size; i++) {
			this->rawStorage[i] = 0;
		}
		glGenBuffersARB(1, &this->handle);
	}
	/// Destroy the index buffer, its shadow array, its handle and its storage on GPU
	~IndexBuffer() {
		if(this->handle != 0) {
			glDeleteBuffersARB(1, &this->handle);
		}
		delete[] this->rawStorage;
	}
public:
	/**
		\brief Set the index buffer from a vector of indices. Will not commit.
		\param data	The vector of indices.
	*/
	void SetData(const std::vector<IndexType>& data) {
		assert(data.size() <= this->size);
		for(unsigned int i = 0; i < data.size(); i++) {
			this->rawStorage[i] = data[i];
		}
	}
	unsigned int getSize() {
		return size;
	}
private:
	GLuint handle;
	unsigned int size;
	IndexType* rawStorage;
};

#endif
