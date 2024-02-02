#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	enum class ShaderResourceType
	{
		UniformBuffer = 1,
		StorageBuffer = 2,
		Texture2D = 3
	};

	class ATHENA_API ShaderResource: public RefCounted
	{
	public:
		virtual ~ShaderResource() = default;
		virtual ShaderResourceType GetResourceType() = 0;
	};
}
