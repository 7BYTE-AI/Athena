#pragma once

#include "Vector.h"


namespace Athena
{
#define Size4 4

	template <typename T>
	class Vector<T, Size4>
	{
	public:
		using iterator = VectorIterator<T, Size4>;
		using const_iterator = VectorConstIterator<T, Size4>;

	// Constructors
	public:
		constexpr Vector() = default;

		constexpr Vector(T value)
			: x(value), 
			  y(value), 
			  z(value), 
			  w(value) {}


		constexpr Vector(T _x, T _y, T _z, T _w)
			: x(_x), 
			  y(_y), 
			  z(_z), 
			  w(_w) {}


		template<typename X, typename Y, typename Z, typename W>
		constexpr Vector(X _x, Y _y, Z _z, W _w)
			: x(static_cast<T>(_x)),
			  y(static_cast<T>(_y)),
			  z(static_cast<T>(_z)),
			  w(static_cast<T>(_w)) {}


		constexpr Vector(const Vector& other) = default;
		constexpr Vector& operator=(const Vector& other) = default;

		template <typename U>
		constexpr Vector<T, Size4>(const Vector<U, Size4>& other)
			: x(static_cast<T>(other.x)),
			  y(static_cast<T>(other.y)),
			  z(static_cast<T>(other.z)),
			  w(static_cast<T>(other.w)) {}


		template <typename U>
		constexpr Vector<T, Size4>& operator=(const Vector<U, Size4>& other)
		{
			x = static_cast<T>(other.x);
			y = static_cast<T>(other.y);
			z = static_cast<T>(other.z);
			w = static_cast<T>(other.w);

			return *this;
		}


		template <typename U>
		constexpr Vector<T, Size4>(const Vector<U, 2>& _xy)
			: x(static_cast<T>(_xy.x)),
			  y(static_cast<T>(_xy.y)),
			  z(static_cast<T>(1)),
			  w(static_cast<T>(1)) {}
			  

		template <typename U>
		constexpr Vector<T, Size4>& operator=(const Vector<U, 2>& _xy)
		{
			x = static_cast<T>(_xy.x);
			y = static_cast<T>(_xy.y);
			z = static_cast<T>(1);
			w = static_cast<T>(1);

			return *this;
		}


		template <typename X, typename Y, typename Z>
		constexpr Vector<T, Size4>(const Vector<X, 2>& _xy, Y _z, Z _w)
			: x(static_cast<T>(_xy.x)),
			  y(static_cast<T>(_xy.y)),
			  z(static_cast<T>(_z)),
		      w(static_cast<T>(_w)) {}


		template <typename X, typename Y, typename Z>
		constexpr Vector<T, Size4>(X _x, Y _y, const Vector<Z, 2>& _zw)
			: x(static_cast<T>(_x)),
			  y(static_cast<T>(_y)),
			  z(static_cast<T>(_zw.x)),
		      w(static_cast<T>(_zw.y)) {}


		template <typename X, typename Y, typename Z>
		constexpr Vector<T, Size4>(X _x, const Vector<Y, 2>& _yz, Z _w)
			: x(static_cast<T>(_x)),
			  y(static_cast<T>(_yz.x)),
			  z(static_cast<T>(_yz.y)),
			  w(static_cast<T>(_w)) {}


		template <typename U>
		constexpr Vector<T, Size4>(const Vector<U, 3>& other)
			: x(static_cast<T>(other.x)),
			  y(static_cast<T>(other.y)),
			  z(static_cast<T>(other.z)),
			  w(static_cast<T>(1)) {}


		template <typename U>
		constexpr Vector<T, Size4>& operator=(const Vector<U, 3>& other)
		{
			x = static_cast<T>(other.x);
			y = static_cast<T>(other.y);
			z = static_cast<T>(other.z);
			w = static_cast<T>(1);

			return *this;
		}

		template <typename X, typename Y>
		constexpr Vector<T, Size4>(const Vector<X, 3>& _xyz, Y _w)
			: x(static_cast<T>(_xyz.x)),
			  y(static_cast<T>(_xyz.y)),
			  z(static_cast<T>(_xyz.z)),
			  w(static_cast<T>(_w)) {}


		template <typename X, typename Y>
		constexpr Vector<T, Size4>(X _x, const Vector<Y, 3>& _yzw)
			: x(static_cast<T>(_x)),
			  y(static_cast<T>(_yzw.x)),
			  z(static_cast<T>(_yzw.y)),
		      w(static_cast<T>(_yzw.z)) {}

	// Public Methods
	public:
		constexpr SIZE_T Size() const
		{
			return Size4;
		}

		constexpr T* Data()
		{
			return &x;
		}

		constexpr const T* Data() const
		{
			return &x;
		}

		constexpr iterator begin()
		{
			return iterator(&x, 0);
		}

		constexpr iterator end()
		{
			return iterator(&x, Size4);
		}

		constexpr const_iterator cbegin() const
		{
			return const_iterator(&x, 0);
		}

		constexpr const_iterator cend() const
		{
			return const_iterator(&x, Size4);
		}

		constexpr void Fill(T value)
		{
			x = value;
			y = value;
			z = value;
			w = value;
		}

		constexpr Vector& Apply(T (*func)(T))
		{
			x = func(x);
			y = func(y);
			z = func(z);
			w = func(w);
			return *this;
		}

		constexpr T SqrLength() const 
		{
			return x * x + y * y + z * z + w * w;
		}

		constexpr float Length() const 
		{
			return std::sqrt(static_cast<float>(SqrLength()));
		}

		constexpr Vector& Normalize()
		{
			float length = Length();
			return length == 0 ? *this : *this /= static_cast<T>(length);
		}

		constexpr Vector GetNormalized() const
		{
			float length = Length();
			return length == 0 ? Vector(*this) : Vector(*this) /= static_cast<T>(length);
		}

	// Operators
	public:
		constexpr const T& operator[](SIZE_T idx) const
		{
			return *(&x + idx);
		}

		constexpr T& operator[](SIZE_T idx)
		{
			return *(&x + idx);
		}

		constexpr Vector& operator+=(const Vector& other) 
		{
			x += other.x;
			y += other.y;
			z += other.z;
			w += other.w;
			return *this;
		}

		constexpr Vector& operator-=(const Vector& other) 
		{
			x -= other.x;
			y -= other.y;
			z -= other.z;
			w -= other.w;
			return *this;
		}

		constexpr Vector& operator*=(const Vector& other)
		{
			x *= other.x;
			y *= other.y;
			z *= other.z;
			w *= other.w;
			return *this;
		}

		constexpr Vector& operator/=(const Vector& other)
		{
			x /= other.x;
			y /= other.y;
			z /= other.z;
			w /= other.w;
			return *this;
		}

		constexpr Vector& operator+=(T scalar) 
		{
			x += scalar;
			y += scalar;
			z += scalar;
			w += scalar;
			return *this;
		}

		constexpr Vector& operator-=(T scalar) 
		{
			x -= scalar;
			y -= scalar;
			z -= scalar;
			w -= scalar;
			return *this;
		}

		constexpr Vector& operator*=(T scalar) 
		{
			x *= scalar;
			y *= scalar;
			z *= scalar;
			w *= scalar;
			return *this;
		}

		constexpr Vector& operator/=(T scalar)
		{
			x /= scalar;
			y /= scalar;
			z /= scalar;
			w /= scalar;
			return *this;
		}

		constexpr Vector operator+(const Vector& other) const 
		{
			return Vector(x + other.x, y + other.y, z + other.z, w + other.w);
		}

		constexpr Vector operator-(const Vector& other) const 
		{
			return Vector(x - other.x, y - other.y, z - other.z, w - other.w);
		}

		constexpr Vector operator*(const Vector& other) const
		{
			return Vector(x * other.x, y * other.y, z * other.z, w * other.w);
		}

		constexpr Vector operator/(const Vector& other) const
		{
			return Vector(x / other.x, y / other.y, z / other.z, w / other.w);
		}

		constexpr Vector operator+(T scalar) const 
		{
			return Vector(x + scalar, y + scalar, z + scalar, w + scalar);
		}

		constexpr Vector operator-(T scalar) const 
		{
			return Vector(x - scalar, y - scalar, z - scalar, w - scalar);
		}

		constexpr Vector operator*(T scalar) const 
		{
			return Vector(x * scalar, y * scalar, z * scalar, w * scalar);
		}

		constexpr Vector operator/(T scalar) const
		{
			return Vector(x / scalar, y / scalar, z / scalar, w / scalar);
		}

		constexpr Vector operator-() const 
		{
			return Vector(-x, -y, -z, -w);
		}

		constexpr bool operator==(const Vector& other) const 
		{
			return x == other.x && y == other.y && z == other.z && w == other.w;
		}

		constexpr bool operator!=(const Vector& other) const 
		{
			return !(*this == other);
		}

	public:
		T x, y, z, w;
	};

#undef Size4
}
