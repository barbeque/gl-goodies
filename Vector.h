#ifndef _591_VECTOR_H_
#define _591_VECTOR_H_

#include <cmath>
#include <cstddef>
#include <cassert>
#include <iostream>

template<size_t N, class T>
class Vector {
	public:
	/// Raw data
	T m_Data[N];
	/// Basic constructor
	Vector<N,T>() {
		for(size_t i = 0; i < N; i++) {
			m_Data[i] = 0;
		}
	}
	/// 2-vector cons
	Vector<N,T>(float a, float b) {
		for(size_t i = 0; i < N; i++) {
			m_Data[i] = 0;
		}

		assert(N >= 2);
		m_Data[0] = a;
		m_Data[1] = b;
	}
	/// 3-Vector cons
	Vector<N,T>(float a, float b, float c) {
		for(size_t i = 0; i < N; i++) {
			m_Data[i] = 0;
		}
		
		assert(N >= 3);
		m_Data[0] = a;
		m_Data[1] = b;
		m_Data[2] = c;
	}
	/// 3-Vector cons
	Vector<N,T>(const Vector<2,T>& v, float c) {
		for(size_t i = 0; i < N; i++) {
			m_Data[i] = 0;
		}
		
		assert(N >= 3); // can't do a template specialization here for some reason.
		m_Data[0] = v[0];
		m_Data[1] = v[1];
		m_Data[2] = c;
	}
	/// 4-vector cons
	Vector<N,T>(float a, float b, float c, float d) {
		for(size_t i = 0; i < N; i++) {
			m_Data[i] = 0;
		}
		
		assert(N >= 4);
		m_Data[0] = a;
		m_Data[1] = b;
		m_Data[2] = c;
		m_Data[3] = d;
	}
	/// Copy cons
	Vector<N,T>(const Vector<N,T>& v) {
		for(size_t i = 0; i < N; i++) {
			m_Data[i] = v[i];
		}
	}
	/// Set operator
	Vector<N,T>& operator=(const Vector<N,T>& V) {
		for(size_t i = 0; i < N; i++) {
			m_Data[i] = V[i];
		}
		return *this;
	}
	/// Accessor
	inline T& operator[](size_t i) { 
		return m_Data[i];
	}
	/// Const accessor
	inline const T& operator[](size_t i) const {
		return m_Data[i];
	}
	// Operators (unary)
	Vector<N,T>& operator+=(const Vector<N,T>& p);
	Vector<N,T>& operator-=(const Vector<N,T>& p) {
		for(size_t i = 0; i < N; i++) {
			m_Data[i] -= p[i];
		}
		return *this;
	}
	Vector<N,T>& operator*=(T f) {
		for(size_t i = 0; i < N; i++) {
			m_Data[i] *= f;
		}
		return *this;
	}
	Vector<N,T>& operator/=(T f) {
		for(size_t i = 0; i < N; i++) {
			m_Data[i] /= f;
		}
		return *this;
	}
	/// Negation operator
	Vector<N,T> operator-() const {
		Vector<N, T> output;
		for(size_t i = 0; i < N; i++) {
			output.m_Data[i] = m_Data[i] * -1;
		}
		return output;
	}

	// Operators (binary)
	Vector<N,T> operator+(const Vector<N,T>&p) const;
	Vector<N,T> operator-(const Vector<N,T>& v) const {
		Vector<N,T> ret = *this;
		ret -= v;
		return ret;
	}
	Vector<N,T> operator*(double v) const {
		Vector<N,T> ret = *this;
		ret *= v;
		return ret;
	}
	Vector<N,T> operator/(double v) const {
		Vector<N,T> ret = *this;
		ret /= v;
		return ret;
	}

	/// Get the dot product with another vector
	double dot(const Vector<N,T>&) const;
	/// Get the length/magnitude of the vector (For later normalization)
	double length() const;
	/// Get the normalized form of this vector
	Vector<N,T> normalize() const {
		Vector<N,T> ret;
		double len = length();
		for(size_t i = 0; i < N; i++) {
			double here = m_Data[i];
			here /= len;
			ret[i] = here;
		}
		return ret;
	}
	
	/**
		\brief Project this vector along another.
		\param projectAlong	The vector to project this one along.
		\return				The projected form of this vector.
	*/
	Vector<N,T> project(const Vector<N,T>& projectAlong) const {
		Vector<N, T> output;
		
		float bLength = projectAlong.length() * projectAlong.length();
		if(bLength == 0.0f) {
			// Uh oh! Can't divide by zero.
			return (*this);
		}
		
		output = (this->dot(projectAlong) / bLength) * projectAlong;
		
		return output;
	}
};

/// Vector-vector addition/equals
template<size_t N, class T>
inline Vector<N,T>& Vector<N,T>::operator+=(const Vector<N,T>& v) {
	for(size_t i = 0; i < N; i++) {
		m_Data[i] += v[i];
	}
	return *this;
}

/// Vector-vector addition
template<size_t N, class T>
inline Vector<N,T> Vector<N,T>::operator+(const Vector<N,T>& v) const {
	Vector<N,T> ret = *this;
	ret += v;
	return ret;
}

/// Length of vector
template<size_t N, class T>
inline double Vector<N,T>::length() const {
	double ret = 0.0;
	for(size_t i = 0; i < N; i++) {
		ret += (m_Data[i] * m_Data[i]);
	}
	return sqrt(ret);
}
	
/// Cross product for 2-vector
template<class T>
inline float cross(const Vector<2,T>& v1, const Vector<2,T>& v2) {
	/*
	 (x * v2.y) - (y * v2.x)
	*/
	return (v1[0] * v2[1]) - (v1[2] * v2[0]);
}

/// Cross product
template<class T>
inline Vector<3,T> cross(const Vector<3,T>& v1, const Vector<3,T>& v2) {
	Vector<3,T> ret;
	/*
		Cx = AyBz - AzBy
		Cy = AzBx - AxBz
		Cz = AxBy - AyBx
	*/
	ret[0] = (v1[1] * v2[2]) - (v1[2] * v2[1]);
	ret[1] = (v1[2] * v2[0]) - (v1[0] * v2[2]);
	ret[2] = (v1[0] * v2[1]) - (v1[1] * v2[0]);
	return ret;
}

/// Cross product for 4-vector, ignoring the w-component.
template<class T>
inline Vector<4,T> cross(const Vector<4,T>& v1, const Vector<4,T>& v2) {
	Vector<4,T> ret;
	ret[0] = (v1[1] * v2[2]) - (v1[2] * v2[1]);
	ret[1] = (v1[2] * v2[0]) - (v1[0] * v2[2]);
	ret[2] = (v1[0] * v2[1]) - (v1[1] * v2[0]);
	ret[3] = 0;
	return ret;
}

/// Scale with l-value double
template<size_t N, class T>
inline Vector<N,T> operator*(double s, const Vector<N,T>& v) {
	Vector<N,T> ret;
	for(size_t i = 0; i < N; i++) {
		ret[i] = s * v[i];
	}
	return ret;
}

template<size_t N, class T>
inline double Vector<N,T>::dot(const Vector<N,T>& p) const {
	// U dot V = U1V1 + U2V2 + U3V3 + ... + UnVn
	double ret = 0;
	for(size_t i = 0; i < N; i++) {
		ret += m_Data[i] * p[i];
	}
	return ret;
}
	
/**
 \brief Gives you the angle between two vectors, around a basis vector.
 \param basis	The basis vector.
 \param v1		The first vector.
 \param v2		The second vector.
 \return		The angle between the two vectors, in radians.
 */
inline float calculateVectorAngle(const Vector<2, float>& basis, const Vector<2, float>& v1, const Vector<2, float>& v2) {
	Vector<2, float> a = (v1 - basis);
	Vector<2, float> b = (v2 - basis);
	return atan2(
				 (float)(a[0] * b[1]) - (a[1] * b[0]),
				 (float)a.dot(b)
				 );
}

template<size_t N, class T>
std::ostream& operator<<(std::ostream& os, const Vector<N,T>& m) {
	os << std::string("< ");
	for(int i = 0; i < (int)N; i++) {
		os << m[i] << " ";
	}
	os << std::string(">\n");
	return os;
}

typedef Vector<2,int> Vector2i;
typedef Vector<2,float> Vector2;
typedef Vector<3,int> Vector3i;
typedef Vector<3,float> Vector3;
typedef Vector<2,double> Vector2d;
typedef Vector<3,double> Vector3d;
typedef Vector<4,int> Vector4i;
typedef Vector<4,float> Vector4;
typedef Vector<4,double> Vector4d;

#endif

