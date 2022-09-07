#pragma once

#include "Athena/Math/SIMD/Platform.h"


#ifdef ATN_SSE_2

namespace Athena::Math
{
	inline Vector<float, 4> operator*(const Vector<float, 4>& vec, const Matrix<float, 4, 4>& mat)
	{
		__m128 vecX = _mm_shuffle_ps(vec._data, vec._data, _MM_SHUFFLE(0, 0, 0, 0));
		__m128 vecY = _mm_shuffle_ps(vec._data, vec._data, _MM_SHUFFLE(1, 1, 1, 1));
		__m128 vecZ = _mm_shuffle_ps(vec._data, vec._data, _MM_SHUFFLE(2, 2, 2, 2));
		__m128 vecW = _mm_shuffle_ps(vec._data, vec._data, _MM_SHUFFLE(3, 3, 3, 3));

		__m128 result = _mm_mul_ps(vecX, mat[0]._data);
		result = _mm_add_ps(result, _mm_mul_ps(vecY, mat[1]._data));
		result = _mm_add_ps(result, _mm_mul_ps(vecZ, mat[2]._data));
		result = _mm_add_ps(result, _mm_mul_ps(vecW, mat[3]._data));

		return Vector<float, 4>(result);
	}

	inline Matrix<float, 4, 4> operator*(const Matrix<float, 4, 4>& Left, const Matrix<float, 4, 4>& Right)
	{
		Matrix<float, 4, 4> out;
		out[0] = Left[0] * Right;
		out[1] = Left[1] * Right;
		out[2] = Left[2] * Right;
		out[3] = Left[3] * Right;
		return out;
	}

	inline Matrix<float, 4, 4> Transpose(const Matrix<float, 4, 4>& mat)
	{
		Matrix<float, 4, 4> out;
		__m128 tmp0 = _mm_shuffle_ps(mat[0]._data, mat[1]._data, 0x44);
		__m128 tmp2 = _mm_shuffle_ps(mat[0]._data, mat[1]._data, 0xEE);
		__m128 tmp1 = _mm_shuffle_ps(mat[2]._data, mat[3]._data, 0x44);
		__m128 tmp3 = _mm_shuffle_ps(mat[2]._data, mat[3]._data, 0xEE);

		out[0]._data = _mm_shuffle_ps(tmp0, tmp1, 0x88);
		out[1]._data = _mm_shuffle_ps(tmp0, tmp1, 0xDD);
		out[2]._data = _mm_shuffle_ps(tmp2, tmp3, 0x88);
		out[3]._data = _mm_shuffle_ps(tmp2, tmp3, 0xDD);

		return out;
	}

	inline Matrix<float, 4, 4> AffineInverse(const Matrix<float, 4, 4>& mat)
	{
		Matrix<float, 4, 4> out;

		__m128 tmp1 = _mm_shuffle_ps(mat[0]._data, mat[1]._data, _MM_SHUFFLE(1, 0, 1, 0));
		__m128 tmp2 = _mm_shuffle_ps(mat[0]._data, mat[1]._data, _MM_SHUFFLE(3, 2, 3, 2));
		out[0]._data = _mm_shuffle_ps(tmp1, mat[2]._data, _MM_SHUFFLE(3, 0, 2, 0));
		out[1]._data = _mm_shuffle_ps(tmp1, mat[2]._data, _MM_SHUFFLE(3, 1, 3, 1));
		out[2]._data = _mm_shuffle_ps(tmp2, mat[2]._data, _MM_SHUFFLE(3, 2, 2, 0));

		__m128 tmp = _mm_shuffle_ps(mat[3]._data, mat[3]._data, _MM_SHUFFLE(0, 0, 0, 0));
		__m128 row3 = _mm_mul_ps(out[0]._data, tmp);

		tmp = _mm_shuffle_ps(mat[3]._data, mat[3]._data, _MM_SHUFFLE(1, 1, 1, 1));
		row3 = _mm_add_ps(row3, _mm_mul_ps(out[1]._data, tmp));

		tmp = _mm_shuffle_ps(mat[3]._data, mat[3]._data, _MM_SHUFFLE(2, 2, 2, 2));
		row3 = _mm_add_ps(row3, _mm_mul_ps(out[2]._data, tmp));

		out[3]._data = _mm_sub_ps(_mm_set_ps(1, 0, 0, 0), row3);

		return out;
	}
}

#endif // ATN_SSE_2
