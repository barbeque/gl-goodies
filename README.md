These are pieces of utility code that I've used in previous OpenGL projects but they haven't been updated in years.

Some useful items:
* IndexBuffer - Basic wrapper around index buffers
* VertexBuffer - More "type safe" vertex buffer, with basic range checking and state management than the default OpenGL one
* ObjLoader - Loads the Alias-Wavefront OBJ file format with some limitations. Uses IndexBuffer and VertexBuffer for storage.
* Vector - 3D math utility class for a vector. Few operations, mostly used by ObjLoader
