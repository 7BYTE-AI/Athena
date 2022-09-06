#pragma once

#include "Athena/Math/Vector.h"
#include "Athena/Math/Impl/Types/Vector4float_impl.h"
#include "Limits.h"

#include <random>


namespace Athena::Math
{
	class Random
	{
	public:
		static void Init()
		{
			s_RandomEngine.seed(std::random_device()());
		}

		static float Float()
		{
			return (float)s_Distribution(s_RandomEngine) / (float)MaxValue<internal_type>();
		}

		static float Float(float min, float max)
		{
			return Lerp(min, max, Float());
		}

		static uint32 UInt32()
		{
			return (uint32)s_Distribution(s_RandomEngine);
		}

		static uint32 UInt32(uint32 min, uint32 max)
		{
			return min + ((uint32)s_Distribution(s_RandomEngine) % (max - min + 1));
		}

		static uint64 UInt64()
		{
			return s_Distribution(s_RandomEngine);
		}

		static uint64 UInt64(uint64 min, uint64 max)
		{
			return min + (s_Distribution(s_RandomEngine) % (max - min + 1));
		}

		static Vector<float, 2> Vector2()
		{
			return Vector<float, 2>(Float(), Float());
		}

		static Vector<float, 3> Vector3()
		{
			return Vector<float, 3>(Float(), Float(), Float());
		}

		static Vector<float, 4> Vector4()
		{
			return Vector<float, 4>(Float(), Float(), Float(), Float());
		}

		static Vector<float, 2> Vector2(float min, float max)
		{
			return Vector<float, 2>(Float(min, max), Float(min, max));
		}

		static Vector<float, 3> Vector3(float min, float max)
		{
			return Vector<float, 3>(Float(min, max), Float(min, max), Float(min, max));
		}

		static Vector<float, 4> Vector4(float min, float max)
		{
			return Vector<float, 4>(Float(min, max), Float(min, max), Float(min, max), Float(min, max));
		}

	private:
		using internal_type = uint64;

	private:
		static std::mt19937 s_RandomEngine;
		static std::uniform_int_distribution<internal_type> s_Distribution;
	};
}
