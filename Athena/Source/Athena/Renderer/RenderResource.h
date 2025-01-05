#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	enum class RenderResourceType
	{
		Texture2D = 1,
		TextureCube,
		TextureView2D,
		TextureViewCube,
		UniformBuffer,
		StorageBuffer,
	};

	class ATHENA_API RenderResource
	{
	public:
		virtual ~RenderResource() = default;

		virtual RenderResourceType GetResourceType() const = 0;
		virtual const String& GetName() const = 0;
	};
}
