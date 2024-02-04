#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/RenderPass.h"

#include <vulkan/vulkan.h>

namespace Athena
{
	class VulkanRenderPass : public RenderPass
	{
	public:
		VulkanRenderPass(const RenderPassCreateInfo& info);
		~VulkanRenderPass();

		virtual void Begin(const Ref<RenderCommandBuffer>& commandBuffer) override;
		virtual void End(const Ref<RenderCommandBuffer>& commandBuffer) override;

		VkRenderPass GetVulkanRenderPass() const { return m_VulkanRenderPass; };

	private:
		VkRenderPass m_VulkanRenderPass = VK_NULL_HANDLE;
	};
}
