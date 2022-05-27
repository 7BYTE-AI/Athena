#pragma once

#include "Vector.h"


namespace Athena
{
	static constexpr size_t Size4 = 4;

	template <typename Ty>
	class Vector<Ty, Size4>
	{
	public:
		using iterator = VectorIterator<Ty, Size4>;
		using const_iterator = VectorConstIterator<Ty, Size4>;

	public:
		constexpr Vector() = default;

		constexpr Vector(Ty value)
			: x(value), y(value), z(value), t(value) {}

		constexpr Vector(Ty X, Ty Y, Ty Z, Ty T)
			: x(X), y(Y), z(Z), t(T) {}

		constexpr Vector(const Vector& other) = default;

		constexpr Vector<Ty, Size4>(const Vector<Ty, 3>& other)
			: Vector(other.x, other.y, other.z, static_cast<Ty>(1)) {}

		template <typename U>
		constexpr Vector<Ty, Size4>(const Vector<U, 3>& other)
		{
			static_assert(std::is_convertible<U, Ty>::value,
				"Vector initialization error: Vectors are not convertible");

			x = static_cast<Ty>(other.x);
			y = static_cast<Ty>(other.y);
			z = static_cast<Ty>(other.z);
			t = static_cast<Ty>(1);
		}

		template <typename U>
		constexpr Vector<Ty, Size4>(const Vector<U, Size4>& other)
		{
			static_assert(std::is_convertible<U, Ty>::value,
				"Vector initialization error: Vectors are not convertible");

			x = static_cast<Ty>(other.x);
			y = static_cast<Ty>(other.y);
			z = static_cast<Ty>(other.z);
			t = static_cast<Ty>(other.t);
		}

		constexpr Vector& operator=(const Vector& other) = default;

		template <typename U>
		constexpr Vector<Ty, Size4>& operator=(const Vector<U, Size4>& other)
		{
			static_assert(std::is_convertible<U, Ty>::value,
				"Vector assignment error: Vectors are not convertible");

			x = static_cast<Ty>(other.x);
			y = static_cast<Ty>(other.y);
			z = static_cast<Ty>(other.z);
			t = static_cast<Ty>(other.t);

			return *this;
		}

		constexpr void Fill(Ty value)
		{
			x = value;
			y = value;
			z = value;
			t = value;
		}

		constexpr const Ty& operator[](size_t idx) const
		{
			ATN_CORE_ASSERT(idx < Size4, "Vector subscript out of range");
			return *(&x + idx);
		}

		constexpr Ty& operator[](size_t idx)
		{
			ATN_CORE_ASSERT(idx < Size4, "Vector subscript out of range");
			return *(&x + idx);
		}

		constexpr size_t GetSize() const
		{
			return Size4;
		}

		constexpr Ty* Data()
		{
			return &x;
		}

		constexpr Ty* Data() const
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

		constexpr Vector& Apply(Ty (*func)(Ty))
		{
			x = func(x);
			y = func(y);
			z = func(z);
			t = func(t);
			return *this;
		}

		constexpr Ty GetSqrLength() const 
		{
			return x * x + y * y + z * z + t * t;
		}

		constexpr float GetLength() const 
		{
			return std::sqrt(static_cast<float>(GetSqrLength()));
		}

		constexpr Vector& Normalize()
		{
			float length = GetLength();
			return length == 0 ? *this : *this /= static_cast<Ty>(length);
		}

		constexpr Vector GetNormalized() const
		{
			float length = GetLength();
			return length == 0 ? Vector(*this) : Vector(*this) /= static_cast<Ty>(length);
		}

	public:
		constexpr Vector& operator+=(const Vector& other) 
		{
			x += other.x;
			y += other.y;
			z += other.z;
			t += other.t;
			return *this;
		}

		constexpr Vector& operator-=(const Vector& other) 
		{
			x -= other.x;
			y -= other.y;
			z -= other.z;
			t -= other.t;
			return *this;
		}

		constexpr Vector& operator+=(Ty scalar) 
		{
			x += scalar;
			y += scalar;
			z += scalar;
			t += scalar;
			return *this;
		}

		constexpr Vector& operator-=(Ty scalar) 
		{
			x -= scalar;
			y -= scalar;
			z -= scalar;
			t -= scalar;
			return *this;
		}

		constexpr Vector& operator*=(Ty scalar) 
		{
			x *= scalar;
			y *= scalar;
			z *= scalar;
			t *= scalar;
			return *this;
		}

		constexpr Vector& operator/=(Ty scalar)
		{
			ATN_CORE_ASSERT(scalar != 0, "Vector operation error: dividing by zero");
			x /= scalar;
			y /= scalar;
			z /= scalar;
			t /= scalar;
			return *this;
		}

		constexpr Vector operator+(const Vector& other) const 
		{
			return Vector(x + other.x, y + other.y, z + other.z, t + other.t);
		}

		constexpr Vector operator-(const Vector& other) const 
		{
			return Vector(x - other.x, y - other.y, z - other.z, t - other.t);
		}

		constexpr Vector operator+(Ty scalar) const 
		{
			return Vector(x + scalar, y + scalar, z + scalar, t + scalar);
		}

		constexpr Vector operator-(Ty scalar) const 
		{
			return Vector(x - scalar, y - scalar, z - scalar, t - scalar);
		}

		constexpr Vector operator*(Ty scalar) const 
		{
			return Vector(x * scalar, y * scalar, z * scalar, t * scalar);
		}

		constexpr Vector operator/(Ty scalar) const
		{
			ATN_CORE_ASSERT(scalar != 0, "Vector operation error: dividing by zero");
			return Vector(x / scalar, y / scalar, z / scalar, t / scalar);
		}

		constexpr Vector operator-() const 
		{
			return Vector(-x, -y, -z, -t);
		}

		constexpr bool operator==(const Vector& other) const 
		{
			return x == other.x && y == other.y && z == other.z && t == other.t;
		}

		constexpr bool operator!=(const Vector& other) const 
		{
			return !(*this == other);
		}

	public:
		Ty x, y, z, t;
	};


	template <typename Ty>
	constexpr Ty Dot(const Vector<Ty, Size4>& Left, const Vector<Ty, Size4>& Right) 
	{
		return Left.x * Right.x + Left.y * Right.y + Left.z * Right.z + Left.t * Right.t;
	}


	typedef Vector<float, Size4> Vector4;
	typedef Vector<unsigned int, Size4> Vector4u;
	typedef Vector<int, Size4> Vector4i;
	typedef Vector<double, Size4> Vector4d;
}