#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	enum class RenderCommandBufferUsage
	{
		PRESENT = 1,
		IMMEDIATE = 2
	};

	struct RenderCommandBufferCreateInfo
	{
		String Name;
		RenderCommandBufferUsage Usage;
	};

	class ATHENA_API RenderCommandBuffer
	{
	public:
		static Ref<RenderCommandBuffer> Create(const RenderCommandBufferCreateInfo& info);
		virtual ~RenderCommandBuffer() = default;

		virtual void Begin() = 0;
		virtual void End() = 0;

		virtual void Submit() = 0;

	protected:
		RenderCommandBufferCreateInfo m_Info;
	};
}
