#pragma once

#include "Athena/Math/TypesImpl/Matrix.h"
#include "Athena/Math/TypesImpl/Vector3.h"
#include "Athena/Math/TypesImpl/Vector4.h"

#include "Athena/Math/Common.h"
#include "Athena/Math/Trigonometric.h"

#include <sstream>


namespace Athena::Math
{
	template <typename T>
	class Quaternion;

	template <typename T>
	constexpr T Dot(const Quaternion<T>& left, const Quaternion<T>& right);


	template <typename T>
	class Quaternion
	{
// -------------Constructors-------------------------------------
	public:
		constexpr Quaternion() = default;

		template<typename W, typename X, typename Y, typename Z>
		constexpr Quaternion(W _w, X _x, Y _y, Z _z)
			: w(static_cast<T>(_w)), 
		      x(static_cast<T>(_x)), 
			  y(static_cast<T>(_y)), 
			  z(static_cast<T>(_z)) {}

		template <typename Q>
		constexpr Quaternion(const Vector<Q, 3>& eulerAngles)
		{
			Vector<Q, 3> c = Math::Cos(eulerAngles * Q(0.5f));
			Vector<Q, 3> s = Math::Sin(eulerAngles * Q(0.5f));

			w = c.x * c.y * c.z + s.x * s.y * s.z;
			x = s.x * c.y * c.z - c.x * s.y * s.z;
			y = c.x * s.y * c.z + s.x * c.y * s.z;
			z = c.x * c.y * s.z - s.x * s.y * c.z;
		}

		template <typename Q>
		constexpr Quaternion(Q pitch, Q yaw, Q roll)
			: Quaternion(Vector<Q, 3>(pitch, yaw, roll))
		{

		}

		template <typename Q>
		constexpr Quaternion(const Vector<Q, 4>& vec4)
			: w(static_cast<T>(vec4.x)),
			  x(static_cast<T>(vec4.y)),
			  y(static_cast<T>(vec4.z)),
			  z(static_cast<T>(vec4.w)) {}


		template <typename Q>
		constexpr Quaternion(const Matrix<Q, 4, 4>& mat4)
		{
			T fourXSquaredMinus1 = mat4[0][0] - mat4[1][1] - mat4[2][2];
			T fourYSquaredMinus1 = mat4[1][1] - mat4[0][0] - mat4[2][2];
			T fourZSquaredMinus1 = mat4[2][2] - mat4[0][0] - mat4[1][1];
			T fourWSquaredMinus1 = mat4[0][0] + mat4[1][1] + mat4[2][2];

			int biggestIndex = 0;
			T fourBiggestSquaredMinus1 = fourWSquaredMinus1;
			if (fourXSquaredMinus1 > fourBiggestSquaredMinus1)
			{
				fourBiggestSquaredMinus1 = fourXSquaredMinus1;
				biggestIndex = 1;
			}
			if (fourYSquaredMinus1 > fourBiggestSquaredMinus1)
			{
				fourBiggestSquaredMinus1 = fourYSquaredMinus1;
				biggestIndex = 2;
			}
			if (fourZSquaredMinus1 > fourBiggestSquaredMinus1)
			{
				fourBiggestSquaredMinus1 = fourZSquaredMinus1;
				biggestIndex = 3;
			}

			T biggestVal = Math::Sqrt(fourBiggestSquaredMinus1 + static_cast<T>(1)) * static_cast<T>(0.5);
			T mult = static_cast<T>(0.25) / biggestVal;

			switch (biggestIndex)
			{
			case 0:
				w = biggestVal;
				x = (mat4[1][2] - mat4[2][1]) * mult;
				y = (mat4[2][0] - mat4[0][2]) * mult;
				z = (mat4[0][1] - mat4[1][0]) * mult;
				break;
			case 1:
				w = (mat4[1][2] - mat4[2][1]) * mult;
				x = biggestVal;
				y = (mat4[0][1] + mat4[1][0]) * mult;
				z = (mat4[2][0] + mat4[0][2]) * mult;
				break;
			case 2:
				w = (mat4[2][0] - mat4[0][2]) * mult;
				x = (mat4[0][1] + mat4[1][0]) * mult;
				y = biggestVal;
				z = (mat4[1][2] + mat4[2][1]) * mult;
				break;
			case 3:
				w = (mat4[0][1] - mat4[1][0]) * mult;
				x = (mat4[2][0] + mat4[0][2]) * mult;
				y = (mat4[1][2] + mat4[2][1]) * mult;
				z = biggestVal;
				break;
			}
		}

// -------------Public Methods-------------------------------------
	public:
		constexpr uint32 Size() const
		{
			return 4;
		}

		constexpr T* Data()
		{
			return &w;
		}

		constexpr const T* Data() const
		{
			return &w;
		}

		constexpr T SqrLength() const
		{
			return Math::Dot(*this, *this);
		}

		constexpr float Length() const
		{
			return Math::Sqrt(SqrLength());
		}

		constexpr Quaternion& Normalize()
		{
			float length = Length();
			return length == 0 ? *this : *this /= static_cast<T>(length);
		}

		constexpr Quaternion GetNormalized() const
		{
			float length = Length();
			return length == 0 ? Quaternion(*this) : Quaternion(*this) /= static_cast<T>(length);
		}
		
		constexpr Quaternion& Inverse()
		{
			T sqrLength = SqrLength();
			x = -x;
			y = -y;
			z = -z;
			return *this /= sqrLength;
		}

		constexpr Quaternion GetInversed() const
		{
			Quaternion conj = GetConjugated();
			T sqrLength = SqrLength();
			return conj / sqrLength;
		}

		constexpr Quaternion& Conjugate()
		{
			x = -x;
			y = -y;
			z = -z;
			return *this;
		}

		constexpr Quaternion GetConjugated() const
		{
			return Quaternion(w, -x, -y, -z);
		}

		inline Vector3 AsEulerAngles() const
		{
			float sqw = w * w;
			float sqx = x * x;
			float sqy = y * y;
			float sqz = z * z;

			float roll = Math::Atan2(T(2.f) * (y * z + x * w), (-sqx - sqy + sqz + sqw));
			float pitch = Math::Asin(T(-2.f) * (x * z - y * w));
			float yaw = Math::Atan2(T(2.f) * (x * y + z * w), (sqx - sqy - sqz + sqw));

			return { roll, pitch, yaw };
		}

		inline Matrix<T, 4, 4> AsMatrix() const
		{
			Matrix<T, 4, 4> result;
			T qxx(x * x);
			T qyy(y * y);
			T qzz(z * z);
			T qxz(x * z);
			T qxy(x * y);
			T qyz(y * z);
			T qwx(w * x);
			T qwy(w * y);
			T qwz(w * z);

			result[0][0] = T(1) - T(2) * (qyy + qzz);
			result[0][1] = T(2) * (qxy + qwz);
			result[0][2] = T(2) * (qxz - qwy);
			result[0][3] = T(0);

			result[1][0] = T(2) * (qxy - qwz);
			result[1][1] = T(1) - T(2) * (qxx + qzz);
			result[1][2] = T(2) * (qyz + qwx);
			result[1][3] = T(0);

			result[2][0] = T(2) * (qxz + qwy);
			result[2][1] = T(2) * (qyz - qwx);
			result[2][2] = T(1) - T(2) * (qxx + qyy);
			result[2][3] = T(0);

			result[3] = Vector<T, 4>(0, 0, 0, 1);

			return result;
		}

// -------------Operators-------------------------------------
	public:
		constexpr const T& operator[](uint32 idx) const
		{
			ATN_CORE_ASSERT(idx < 4, "Quaternion subscript out of range");
			return *(&w + idx);
		}

		constexpr T& operator[](uint32 idx)
		{
			ATN_CORE_ASSERT(idx < 4, "Quaternion subscript out of range");
			return *(&w + idx);
		}

		constexpr Quaternion& operator+=(const Quaternion& other)
		{
			w += other.w;
			x += other.x;
			y += other.y;
			z += other.z;
			return *this;
		}

		constexpr Quaternion& operator-=(const Quaternion& other)
		{
			w -= other.w;
			x -= other.x;
			y -= other.y;
			z -= other.z;
			return *this;
		}

		constexpr Quaternion& operator*=(const Quaternion& other)
		{
			Quaternion<T> p(*this);

			w = p.w * other.w - p.x * other.x - p.y * other.y - p.z * other.z;
			x = p.w * other.x + p.x * other.w + p.y * other.z - p.z * other.y;
			y = p.w * other.y + p.y * other.w + p.z * other.x - p.x * other.z;
			z = p.w * other.z + p.z * other.w + p.x * other.y - p.y * other.x;
			return *this;
		}

		constexpr Quaternion& operator*=(T scalar)
		{
			w *= scalar;
			x *= scalar;
			y *= scalar;
			z *= scalar;
			return *this;
		}

		constexpr Quaternion& operator/=(T scalar)
		{
			w /= scalar;
			x /= scalar;
			y /= scalar;
			z /= scalar;
			return *this;
		}

		constexpr Vector<T, 3> operator*(const Vector<T, 3>& vec3) const
		{
			Vector<T, 3> quatVector(x, y, z);
			Vector<T, 3> uv = Math::Cross(quatVector, vec3);
			Vector<T, 3> uuv = Math::Cross(quatVector, uv);

			return vec3 + ((uv * w) + uuv) * static_cast<T>(2);
		}

		constexpr Quaternion operator+(const Quaternion& other) const
		{
			return Quaternion(w + other.w, x + other.x, y + other.y, z + other.z);
		}

		constexpr Quaternion operator-(const Quaternion& other) const
		{
			return Quaternion(w - other.w, x - other.x, y - other.y, z - other.z);
		}

		constexpr Quaternion operator*(const Quaternion& other) const
		{
			return Quaternion(*this) *= other;
		}

		constexpr Quaternion operator*(T scalar) const
		{
			return Quaternion(w * scalar, x * scalar, y * scalar, z * scalar);
		}

		constexpr Quaternion operator/(T scalar) const
		{
			return Quaternion(w / scalar, x / scalar, y / scalar, z / scalar);
		}

		constexpr bool operator==(const Quaternion& other) const
		{
			return w == other.w && x == other.x && y == other.y && z == other.z;
		}

		constexpr bool operator!=(const Quaternion& other) const
		{
			return w != other.w || x != other.x || y != other.y || z != other.z;
		}

// -------------Static Methods-------------------------------------
	public:
		static constexpr Quaternion Identity()
		{
			return Quaternion(1, 0, 0, 0);
		}

	public:
		T w, x, y, z;
	};



// -------------Relative Functions-------------------------------------

	template <typename T>
	constexpr Quaternion<T> operator*(T scalar, const Quaternion<T>& quat)
	{
		return quat * scalar;
	}

	template <typename T>
	constexpr Vector<T, 3> operator*(const Vector<T, 3> vec, const Quaternion<T>& quat)
	{
		return quat.GetInversed() * vec;
	}

	template <typename T>
	constexpr T Dot(const Quaternion<T>& left, const Quaternion<T>& right)
	{
		return left.w * right.w + left.x * right.x + left.y * right.y + left.z * right.z;
	}
}

namespace Athena
{
	template <>
	inline String ToString<Math::Quaternion<float>>(const Math::Quaternion<float>& quat)
	{
		std::stringstream stream;
		stream << "Quaternion(" << quat.w << ", " << quat.x << ", " << quat.y << ", " << quat.z << ")";

		return stream.str();
	}
}
